#include "adsblib.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Library Version
 * ============================================================ */

#define ADSBLIB_STR(x)   ADSBLIB_STR_(x)
#define ADSBLIB_STR_(x)  #x

const char *adsb_version_string(void)
{
    return ADSBLIB_STR(ADSBLIB_VERSION_MAJOR) "."
           ADSBLIB_STR(ADSBLIB_VERSION_MINOR) "."
           ADSBLIB_STR(ADSBLIB_VERSION_PATCH);
}

/* ============================================================
 * Internal Constants
 * ============================================================ */

#define ADSB_DF17 17U
#define ADSB_CA   5U

#define ADSB_CPR_BITS 17U
#define ADSB_CPR_MOD  (1U << ADSB_CPR_BITS)

#define ADSB_NZ 15.0

/* ============================================================
 * Internal Helpers
 * ============================================================ */

static double adsb_pi(void)
{
    return acos(-1.0);
}

static bool is_valid_icao(uint32_t icao)
{
    return (icao <= ADSB_ICAO_MAX);
}

static bool is_valid_latitude(double latitude_deg)
{
    return isfinite(latitude_deg) && (latitude_deg >= -90.0) && (latitude_deg <= 90.0);
}

static bool is_valid_longitude(double longitude_deg)
{
    return isfinite(longitude_deg) && (longitude_deg >= -180.0) && (longitude_deg <= 180.0);
}

static bool is_valid_altitude(int32_t altitude_ft)
{
    return (altitude_ft >= -1000) && (altitude_ft <= 60000);
}

static bool is_valid_speed(double speed_kt)
{
    return isfinite(speed_kt) && (speed_kt >= 0.0);
}

static bool is_valid_track(double track_deg)
{
    return isfinite(track_deg) && (track_deg >= 0.0) && (track_deg < 360.0);
}

static bool is_valid_vertical_rate(int32_t vr_fpm)
{
    return (vr_fpm >= -32640) && (vr_fpm <= 32640);
}

static bool is_valid_emergency_state(adsb_emergency_state_t state)
{
    return (state >= ADSB_EMERGENCY_NONE) &&
           (state <= ADSB_EMERGENCY_DOWNED_AIRCRAFT);
}

static bool is_valid_mode_a_code(uint16_t mode_a_code)
{
    return mode_a_code <= 07777U;
}

static bool is_valid_altitude_source(adsb_altitude_source_t source)
{
    return (source == ADSB_ALTITUDE_SOURCE_MCP) ||
           (source == ADSB_ALTITUDE_SOURCE_FMS);
}

static enc_status_t encode_target_altitude(int32_t altitude_ft, uint32_t *altitude_field)
{
    long raw;

    if (altitude_field == NULL)
    {
        return ENC_INVALID_SELECTED_ALTITUDE;
    }
    if (altitude_ft == -1)
    {
        *altitude_field = 0U;
        return ENC_OK;
    }
    if ((altitude_ft < 0) || (altitude_ft > 65472))
    {
        return ENC_INVALID_SELECTED_ALTITUDE;
    }

    raw = lround((double)altitude_ft / 32.0) + 1L;
    if ((raw < 1L) || (raw > 2047L))
    {
        return ENC_INVALID_SELECTED_ALTITUDE;
    }

    *altitude_field = (uint32_t)raw;
    return ENC_OK;
}

static enc_status_t encode_barometric_pressure(double pressure_mbar, uint32_t *pressure_field)
{
    long raw;

    if (pressure_field == NULL)
    {
        return ENC_INVALID_BARO_PRESSURE;
    }
    if (pressure_mbar == -1.0)
    {
        *pressure_field = 0U;
        return ENC_OK;
    }
    if (!isfinite(pressure_mbar) || (pressure_mbar < 800.0) || (pressure_mbar > 1208.0))
    {
        return ENC_INVALID_BARO_PRESSURE;
    }

    raw = lround((pressure_mbar - 800.0) / 0.8) + 1L;
    if ((raw < 1L) || (raw > 511L))
    {
        return ENC_INVALID_BARO_PRESSURE;
    }

    *pressure_field = (uint32_t)raw;
    return ENC_OK;
}

static uint32_t encode_mode_a_code(uint16_t mode_a_code)
{
    uint32_t a = (mode_a_code >> 9U) & 0x7U;
    uint32_t b = (mode_a_code >> 6U) & 0x7U;
    uint32_t c = (mode_a_code >> 3U) & 0x7U;
    uint32_t d = mode_a_code & 0x7U;

    /* Gillham bit order: C1 A1 C2 A2 C4 A4 X B1 D1 B2 D2 B4 D4. */
    return ((a & 4U) << 5U) |
           ((a & 2U) << 8U) |
           ((a & 1U) << 11U) |
           ((b & 4U) >> 1U) |
           ((b & 2U) << 2U) |
           ((b & 1U) << 5U) |
           ((c & 4U) << 6U) |
           ((c & 2U) << 9U) |
           ((c & 1U) << 12U) |
           ((d & 4U) >> 2U) |
           ((d & 2U) >> 1U) |
           ((d & 1U) << 4U);
}

/*
 * Encode a ground speed (kt) into the 7-bit surface-position movement
 * field. The field is non-linear: 6 bins of increasing step size, plus
 * code 1 for "stopped" and code 124 for the open-ended "175 kt or more".
 * Table per ICAO Annex 10 / DO-260B (movement field encoding).
 */
static uint32_t encode_movement(double speed_kt)
{
    static const uint32_t bin_lb_code[6] = { 2U, 9U, 13U, 39U, 94U, 109U };
    static const double bin_lb_kt[6]     = { 0.125, 1.0, 2.0, 15.0, 70.0, 100.0 };
    static const double bin_step_kt[6]   = { 0.125, 0.25, 0.5, 1.0, 2.0, 5.0 };
    int i;

    if (speed_kt < 0.125)
    {
        return 1U;  /* Aircraft stopped */
    }

    if (speed_kt >= 175.0)
    {
        return 124U;  /* 175 kt or more (open-ended) */
    }

    for (i = 5; i >= 0; --i)
    {
        if (speed_kt >= bin_lb_kt[i])
        {
            return bin_lb_code[i] + (uint32_t)floor((speed_kt - bin_lb_kt[i]) / bin_step_kt[i]);
        }
    }

    return 1U;  /* Unreachable: speed_kt >= 0.125 is handled by the loop above. */
}

static double pos_mod(double value, double modulus)
{
    double r;

    if (modulus <= 0.0)
    {
        return 0.0;
    }

    r = fmod(value, modulus);
    if (r < 0.0)
    {
        r += modulus;
    }

    return r;
}

static void frame_set_bits(uint8_t frame[ADSB_FRAME_BYTES], uint32_t start_bit, uint32_t num_bits, uint32_t value)
{
    uint32_t i;

    for (i = 0U; i < num_bits; ++i)
    {
        uint32_t dst_bit = start_bit + i;
        uint32_t dst_byte = dst_bit / 8U;
        uint32_t dst_bit_in_byte = 7U - (dst_bit % 8U);
        uint32_t src_bit = num_bits - 1U - i;
        uint8_t bit_value = (uint8_t)((value >> src_bit) & 0x1U);

        if (bit_value != 0U)
        {
            frame[dst_byte] = (uint8_t)(frame[dst_byte] | (uint8_t)(1U << dst_bit_in_byte));
        }
        else
        {
            frame[dst_byte] = (uint8_t)(frame[dst_byte] & (uint8_t)~(uint8_t)(1U << dst_bit_in_byte));
        }
    }
}

static size_t callsign_len8(const char callsign[ADSB_CALLSIGN_LEN + 1])
{
    size_t i;

    for (i = 0U; i < (ADSB_CALLSIGN_LEN + 1U); ++i)
    {
        if (callsign[i] == '\0')
        {
            return i;
        }
    }

    return ADSB_CALLSIGN_LEN + 1U;
}

static int callsign_char_to_6bit(char c)
{
    if ((c >= 'A') && (c <= 'Z'))
    {
        return (int)(c - 'A') + 1;
    }

    if ((c >= '0') && (c <= '9'))
    {
        return (int)(c - '0') + 48;
    }

    if (c == ' ')
    {
        return 32;
    }

    return -1;
}

static enc_status_t encode_callsign(const char in_callsign[ADSB_CALLSIGN_LEN + 1], uint8_t out_codes[ADSB_CALLSIGN_LEN])
{
    size_t len;
    size_t i;

    if (in_callsign == NULL)
    {
        return ENC_INVALID_ARGUMENT;
    }

    len = callsign_len8(in_callsign);
    if (len > ADSB_CALLSIGN_LEN)
    {
        return ENC_INVALID_CALLSIGN;
    }

    for (i = 0U; i < ADSB_CALLSIGN_LEN; ++i)
    {
        char c = (i < len) ? in_callsign[i] : ' ';
        int code = callsign_char_to_6bit(c);

        if (code < 0)
        {
            return ENC_INVALID_CALLSIGN;
        }

        out_codes[i] = (uint8_t)code;
    }

    return ENC_OK;
}

static enc_status_t encode_altitude_12bit(int32_t altitude_ft, uint32_t *alt_code)
{
    int32_t n;
    uint32_t high;
    uint32_t low;

    if ((alt_code == NULL) || !is_valid_altitude(altitude_ft))
    {
        return ENC_INVALID_ALTITUDE;
    }

    /*
     * Q=1 uses 25 ft increments. This is the common ADS-B airborne format.
     * Encodable range with this representation is -1000..50175 ft.
     */
    if (altitude_ft > 50175)
    {
        return ENC_INVALID_ALTITUDE;
    }

    n = (altitude_ft + 1000) / 25;
    if ((n < 0) || (n > 2047))
    {
        return ENC_INVALID_ALTITUDE;
    }

    high = (uint32_t)((n >> 4) & 0x7FU);
    low = (uint32_t)(n & 0x0FU);

    *alt_code = (high << 5) | (1U << 4) | low;
    return ENC_OK;
}

/* ============================================================
 * Public Utility Functions
 * ============================================================ */

void adsb_frame_clear(uint8_t frame[ADSB_FRAME_BYTES])
{
    if (frame != NULL)
    {
        (void)memset(frame, 0, ADSB_FRAME_BYTES);
    }
}

void adsb_frame_to_hex(const uint8_t frame[ADSB_FRAME_BYTES], char hex_string[29])
{
    static const char hex_lut[] = "0123456789ABCDEF";
    uint32_t i;

    if ((frame == NULL) || (hex_string == NULL))
    {
        return;
    }

    for (i = 0U; i < ADSB_FRAME_BYTES; ++i)
    {
        hex_string[(2U * i)] = hex_lut[(frame[i] >> 4) & 0x0FU];
        hex_string[(2U * i) + 1U] = hex_lut[frame[i] & 0x0FU];
    }

    hex_string[28] = '\0';
}

uint32_t adsb_crc24(const uint8_t *data, uint32_t num_bits)
{
    uint32_t i;
    uint32_t crc = 0U;
    const uint32_t poly = 0xFFF409U;

    if (data == NULL)
    {
        return 0U;
    }

    for (i = 0U; i < num_bits; ++i)
    {
        uint32_t byte_idx = i / 8U;
        uint32_t bit_idx = 7U - (i % 8U);
        uint32_t bit = (data[byte_idx] >> bit_idx) & 0x1U;
        uint32_t top = (crc >> 23U) & 0x1U;

        crc = (crc << 1U) & 0xFFFFFFU;
        if ((top ^ bit) != 0U)
        {
            crc ^= poly;
        }
    }

    return crc & 0xFFFFFFU;
}

void adsb_apply_crc(uint8_t frame[ADSB_FRAME_BYTES])
{
    uint32_t crc;

    if (frame == NULL)
    {
        return;
    }

    crc = adsb_crc24(frame, 88U);

    frame[11] = (uint8_t)((crc >> 16U) & 0xFFU);
    frame[12] = (uint8_t)((crc >> 8U) & 0xFFU);
    frame[13] = (uint8_t)(crc & 0xFFU);
}

bool adsb_verify_crc(const uint8_t frame[ADSB_FRAME_BYTES])
{
    uint32_t expected_crc;
    uint32_t frame_crc;

    if (frame == NULL)
    {
        return false;
    }

    expected_crc = adsb_crc24(frame, 88U);
    frame_crc = ((uint32_t)frame[11] << 16U) |
                ((uint32_t)frame[12] << 8U) |
                (uint32_t)frame[13];

    return (expected_crc == frame_crc);
}

/* ============================================================
 * CPR Functions
 * ============================================================ */

int adsb_cpr_nl(double latitude_deg)
{
    double lat;
    double a;
    double b;
    double num;
    double den;
    double angle;

    if (!isfinite(latitude_deg))
    {
        return 1;
    }

    lat = fabs(latitude_deg);

    if (lat < 1e-12)
    {
        return 59;
    }

    if (lat >= 87.0)
    {
        return 1;
    }

    a = 1.0 - cos(adsb_pi() / (2.0 * ADSB_NZ));
    b = cos((adsb_pi() / 180.0) * lat);
    b *= b;

    if (b <= 0.0)
    {
        return 1;
    }

    angle = 1.0 - (a / b);
    if (angle < -1.0)
    {
        angle = -1.0;
    }
    if (angle > 1.0)
    {
        angle = 1.0;
    }

    num = 2.0 * adsb_pi();
    den = acos(angle);

    if (den <= 0.0)
    {
        return 1;
    }

    return (int)floor(num / den);
}

uint32_t adsb_cpr_encode_latitude(double latitude_deg, cpr_format_t format)
{
    double dlat;
    double yz;

    if (!is_valid_latitude(latitude_deg))
    {
        return 0U;
    }

    if (format == CPR_EVEN)
    {
        dlat = 360.0 / (4.0 * ADSB_NZ);
    }
    else
    {
        dlat = 360.0 / (4.0 * ADSB_NZ - 1.0);
    }

    yz = floor(((double)ADSB_CPR_MOD * pos_mod(latitude_deg, dlat) / dlat) + 0.5);

    return ((uint32_t)yz) & 0x1FFFFU;
}

uint32_t adsb_cpr_encode_longitude(double latitude_deg, double longitude_deg, cpr_format_t format)
{
    int nl;
    int ni;
    double dlon;
    double xz;

    if (!is_valid_latitude(latitude_deg) || !is_valid_longitude(longitude_deg))
    {
        return 0U;
    }

    nl = adsb_cpr_nl(latitude_deg);
    if (format == CPR_EVEN)
    {
        ni = nl;
    }
    else
    {
        ni = nl - 1;
    }

    if (ni < 1)
    {
        ni = 1;
    }

    dlon = 360.0 / (double)ni;
    xz = floor(((double)ADSB_CPR_MOD * pos_mod(longitude_deg, dlon) / dlon) + 0.5);

    return ((uint32_t)xz) & 0x1FFFFU;
}

/* ============================================================
 * Public Encoding Functions
 * ============================================================ */

enc_status_t adsb_encode_identification(const adsb_identification_t *msg, uint8_t frame[ADSB_FRAME_BYTES])
{
    uint8_t callsign_codes[ADSB_CALLSIGN_LEN];
    enc_status_t st;
    uint32_t i;

    if ((msg == NULL) || (frame == NULL))
    {
        return ENC_INVALID_ARGUMENT;
    }

    if (!is_valid_icao(msg->icao))
    {
        return ENC_INVALID_ICAO;
    }

    st = encode_callsign(msg->callsign, callsign_codes);
    if (st != ENC_OK)
    {
        return st;
    }

    adsb_frame_clear(frame);

    frame_set_bits(frame, 0U, 5U, ADSB_DF17);
    frame_set_bits(frame, 5U, 3U, ADSB_CA);
    frame_set_bits(frame, 8U, 24U, msg->icao & ADSB_ICAO_MAX);

    frame_set_bits(frame, 32U, 5U, 4U);  /* Type code 4: aircraft identification */
    frame_set_bits(frame, 37U, 3U, 0U);  /* Emitter category unknown */

    for (i = 0U; i < ADSB_CALLSIGN_LEN; ++i)
    {
        frame_set_bits(frame, 40U + (6U * i), 6U, callsign_codes[i]);
    }

    adsb_apply_crc(frame);

    return ENC_OK;
}

enc_status_t adsb_encode_position(const adsb_position_t *msg, uint8_t frame[ADSB_FRAME_BYTES])
{
    uint32_t alt_code;
    uint32_t cpr_lat;
    uint32_t cpr_lon;
    enc_status_t st;

    if ((msg == NULL) || (frame == NULL))
    {
        return ENC_INVALID_ARGUMENT;
    }

    if (!is_valid_icao(msg->icao))
    {
        return ENC_INVALID_ICAO;
    }
    if (!is_valid_latitude(msg->latitude_deg))
    {
        return ENC_INVALID_LATITUDE;
    }
    if (!is_valid_longitude(msg->longitude_deg))
    {
        return ENC_INVALID_LONGITUDE;
    }
    if (!is_valid_altitude(msg->altitude_ft))
    {
        return ENC_INVALID_ALTITUDE;
    }
    if ((msg->cpr_format != CPR_EVEN) && (msg->cpr_format != CPR_ODD))
    {
        return ENC_INVALID_ARGUMENT;
    }

    st = encode_altitude_12bit(msg->altitude_ft, &alt_code);
    if (st != ENC_OK)
    {
        return st;
    }

    cpr_lat = adsb_cpr_encode_latitude(msg->latitude_deg, msg->cpr_format);
    cpr_lon = adsb_cpr_encode_longitude(msg->latitude_deg, msg->longitude_deg, msg->cpr_format);

    adsb_frame_clear(frame);

    frame_set_bits(frame, 0U, 5U, ADSB_DF17);
    frame_set_bits(frame, 5U, 3U, ADSB_CA);
    frame_set_bits(frame, 8U, 24U, msg->icao & ADSB_ICAO_MAX);

    frame_set_bits(frame, 32U, 5U, 11U);                                /* Type code 11 */
    frame_set_bits(frame, 37U, 2U, 0U);                                 /* Surveillance status */
    frame_set_bits(frame, 39U, 1U, 0U);                                 /* NIC supplement-B */
    frame_set_bits(frame, 40U, 12U, alt_code & 0x0FFFU);                /* Altitude */
    frame_set_bits(frame, 52U, 1U, 0U);                                 /* UTC sync time flag */
    frame_set_bits(frame, 53U, 1U, (uint32_t)msg->cpr_format & 0x01U);  /* CPR format */
    frame_set_bits(frame, 54U, 17U, cpr_lat & 0x1FFFFU);
    frame_set_bits(frame, 71U, 17U, cpr_lon & 0x1FFFFU);

    adsb_apply_crc(frame);

    return ENC_OK;
}

enc_status_t adsb_encode_surface_position(const adsb_surface_position_t *msg, uint8_t frame[ADSB_FRAME_BYTES])
{
    uint32_t cpr_lat;
    uint32_t cpr_lon;
    uint32_t movement;
    uint32_t track_code;

    if ((msg == NULL) || (frame == NULL))
    {
        return ENC_INVALID_ARGUMENT;
    }

    if (!is_valid_icao(msg->icao))
    {
        return ENC_INVALID_ICAO;
    }
    if (!is_valid_latitude(msg->latitude_deg))
    {
        return ENC_INVALID_LATITUDE;
    }
    if (!is_valid_longitude(msg->longitude_deg))
    {
        return ENC_INVALID_LONGITUDE;
    }
    if (!is_valid_speed(msg->ground_speed_kt))
    {
        return ENC_INVALID_SPEED;
    }
    if (!is_valid_track(msg->ground_track_deg))
    {
        return ENC_INVALID_TRACK;
    }
    if ((msg->cpr_format != CPR_EVEN) && (msg->cpr_format != CPR_ODD))
    {
        return ENC_INVALID_ARGUMENT;
    }

    movement = encode_movement(msg->ground_speed_kt);
    track_code = ((uint32_t)lround(msg->ground_track_deg / (360.0 / 128.0))) & 0x7FU;

    cpr_lat = adsb_cpr_encode_latitude(msg->latitude_deg, msg->cpr_format);
    cpr_lon = adsb_cpr_encode_longitude(msg->latitude_deg, msg->longitude_deg, msg->cpr_format);

    adsb_frame_clear(frame);

    frame_set_bits(frame, 0U, 5U, ADSB_DF17);
    frame_set_bits(frame, 5U, 3U, ADSB_CA);
    frame_set_bits(frame, 8U, 24U, msg->icao & ADSB_ICAO_MAX);

    /*
     * Type code 8: surface position, navigation accuracy unknown. NIC
     * selection isn't exposed on the public API yet, mirroring
     * adsb_encode_position, which similarly fixes TC 11 rather than
     * exposing NIC.
     */
    frame_set_bits(frame, 32U, 5U, 8U);
    frame_set_bits(frame, 37U, 7U, movement & 0x7FU);
    frame_set_bits(frame, 44U, 1U, 1U);            /* Ground track status: valid */
    frame_set_bits(frame, 45U, 7U, track_code);
    frame_set_bits(frame, 52U, 1U, 0U);            /* UTC sync time flag */
    frame_set_bits(frame, 53U, 1U, (uint32_t)msg->cpr_format & 0x01U);
    frame_set_bits(frame, 54U, 17U, cpr_lat & 0x1FFFFU);
    frame_set_bits(frame, 71U, 17U, cpr_lon & 0x1FFFFU);

    adsb_apply_crc(frame);

    return ENC_OK;
}

enc_status_t adsb_encode_velocity(const adsb_velocity_t *msg, uint8_t frame[ADSB_FRAME_BYTES])
{
    double track_rad;
    double v_east;
    double v_north;
    double resolution_kt;
    uint32_t subtype;
    int32_t ew_speed;
    int32_t ns_speed;
    uint32_t ew_dir;
    uint32_t ns_dir;
    uint32_t ew_field;
    uint32_t ns_field;
    int32_t vr_q;
    uint32_t vr_sign;
    uint32_t vr_field;

    if ((msg == NULL) || (frame == NULL))
    {
        return ENC_INVALID_ARGUMENT;
    }

    if (!is_valid_icao(msg->icao))
    {
        return ENC_INVALID_ICAO;
    }
    if (!is_valid_speed(msg->ground_speed_kt))
    {
        return ENC_INVALID_SPEED;
    }
    if (!is_valid_track(msg->track_deg))
    {
        return ENC_INVALID_TRACK;
    }
    if (!is_valid_vertical_rate(msg->vertical_rate_fpm))
    {
        return ENC_INVALID_VERTICAL_RATE;
    }

    track_rad = msg->track_deg * (adsb_pi() / 180.0);
    v_east = msg->ground_speed_kt * sin(track_rad);
    v_north = msg->ground_speed_kt * cos(track_rad);

    /*
     * Subtype 1 (subsonic, 1 kt/LSB) covers components up to 1022 kt.
     * Subtype 2 (supersonic, 4 kt/LSB) covers components up to 4088 kt
     * and is selected automatically when subtype 1's range is exceeded.
     */
    ew_speed = (int32_t)lround(fabs(v_east));
    ns_speed = (int32_t)lround(fabs(v_north));

    if ((ew_speed <= 1022) && (ns_speed <= 1022))
    {
        subtype = 1U;
        resolution_kt = 1.0;
    }
    else
    {
        subtype = 2U;
        resolution_kt = 4.0;
        ew_speed = (int32_t)lround(fabs(v_east) / resolution_kt);
        ns_speed = (int32_t)lround(fabs(v_north) / resolution_kt);
    }

    if ((ew_speed > 1022) || (ns_speed > 1022))
    {
        return ENC_INVALID_SPEED;
    }

    ew_dir = (v_east < 0.0) ? 1U : 0U;
    ns_dir = (v_north < 0.0) ? 1U : 0U;

    ew_field = (uint32_t)(ew_speed + 1);
    ns_field = (uint32_t)(ns_speed + 1);

    vr_q = (int32_t)lround((double)abs(msg->vertical_rate_fpm) / 64.0);
    if (vr_q > 510)
    {
        return ENC_INVALID_VERTICAL_RATE;
    }

    vr_sign = (msg->vertical_rate_fpm < 0) ? 1U : 0U;
    vr_field = (uint32_t)(vr_q + 1);

    adsb_frame_clear(frame);

    frame_set_bits(frame, 0U, 5U, ADSB_DF17);
    frame_set_bits(frame, 5U, 3U, ADSB_CA);
    frame_set_bits(frame, 8U, 24U, msg->icao & ADSB_ICAO_MAX);

    frame_set_bits(frame, 32U, 5U, 19U);   /* Type code 19: airborne velocity */
    frame_set_bits(frame, 37U, 3U, subtype);  /* Subtype 1: subsonic, 2: supersonic */
    frame_set_bits(frame, 40U, 1U, 0U);    /* Intent change flag */
    frame_set_bits(frame, 41U, 1U, 0U);    /* IFR capability / reserved */
    frame_set_bits(frame, 42U, 3U, 0U);    /* NACv unknown */
    frame_set_bits(frame, 45U, 1U, ew_dir);
    frame_set_bits(frame, 46U, 10U, ew_field & 0x03FFU);
    frame_set_bits(frame, 56U, 1U, ns_dir);
    frame_set_bits(frame, 57U, 10U, ns_field & 0x03FFU);
    frame_set_bits(frame, 67U, 1U, 1U);    /* Vertical rate source: barometric */
    frame_set_bits(frame, 68U, 1U, vr_sign);
    frame_set_bits(frame, 69U, 9U, vr_field & 0x01FFU);
    frame_set_bits(frame, 78U, 2U, 0U);    /* Reserved */
    frame_set_bits(frame, 80U, 1U, 0U);    /* GNSS/baro diff sign */
    frame_set_bits(frame, 81U, 7U, 0U);    /* GNSS/baro diff unavailable */

    adsb_apply_crc(frame);

    return ENC_OK;
}

enc_status_t adsb_encode_emergency(const adsb_emergency_t *msg, uint8_t frame[ADSB_FRAME_BYTES])
{
    uint32_t mode_a_field;

    if ((msg == NULL) || (frame == NULL))
    {
        return ENC_INVALID_ARGUMENT;
    }

    if (!is_valid_icao(msg->icao))
    {
        return ENC_INVALID_ICAO;
    }
    if (!is_valid_emergency_state(msg->emergency_state))
    {
        return ENC_INVALID_EMERGENCY_STATE;
    }
    if (!is_valid_mode_a_code(msg->mode_a_code))
    {
        return ENC_INVALID_MODE_A_CODE;
    }

    mode_a_field = encode_mode_a_code(msg->mode_a_code);

    adsb_frame_clear(frame);

    frame_set_bits(frame, 0U, 5U, ADSB_DF17);
    frame_set_bits(frame, 5U, 3U, ADSB_CA);
    frame_set_bits(frame, 8U, 24U, msg->icao & ADSB_ICAO_MAX);

    frame_set_bits(frame, 32U, 5U, 28U);  /* Type code 28 */
    frame_set_bits(frame, 37U, 3U, 1U);   /* Subtype 1 */
    frame_set_bits(frame, 40U, 3U, (uint32_t)msg->emergency_state);
    frame_set_bits(frame, 43U, 13U, mode_a_field & 0x1FFFU);
    frame_set_bits(frame, 56U, 32U, 0U);  /* Reserved */

    adsb_apply_crc(frame);

    return ENC_OK;
}

enc_status_t adsb_encode_target_state(const adsb_target_state_t *msg, uint8_t frame[ADSB_FRAME_BYTES])
{
    uint32_t altitude_field;
    uint32_t pressure_field;
    uint32_t heading_field = 0U;
    enc_status_t st;

    if ((msg == NULL) || (frame == NULL))
    {
        return ENC_INVALID_ARGUMENT;
    }
    if (!is_valid_icao(msg->icao))
    {
        return ENC_INVALID_ICAO;
    }
    if (!is_valid_altitude_source(msg->selected_altitude_source))
    {
        return ENC_INVALID_ARGUMENT;
    }
    st = encode_target_altitude(msg->selected_altitude_ft, &altitude_field);
    if (st != ENC_OK)
    {
        return st;
    }
    st = encode_barometric_pressure(msg->barometric_pressure_mbar, &pressure_field);
    if (st != ENC_OK)
    {
        return st;
    }
    if (msg->nac_p > 15U)
    {
        return ENC_INVALID_NAC_P;
    }
    if (msg->sil > 3U)
    {
        return ENC_INVALID_SIL;
    }
    if (msg->selected_heading_valid)
    {
        if (!is_valid_track(msg->selected_heading_deg))
        {
            return ENC_INVALID_HEADING;
        }
        heading_field = (uint32_t)lround(msg->selected_heading_deg * (512.0 / 360.0)) & 0x1FFU;
    }

    adsb_frame_clear(frame);

    frame_set_bits(frame, 0U, 5U, ADSB_DF17);
    frame_set_bits(frame, 5U, 3U, ADSB_CA);
    frame_set_bits(frame, 8U, 24U, msg->icao & ADSB_ICAO_MAX);

    frame_set_bits(frame, 32U, 5U, 29U);  /* Type code 29 */
    frame_set_bits(frame, 37U, 2U, 1U);   /* Subtype 1 */
    frame_set_bits(frame, 39U, 1U, 0U);   /* SIL supplement, reserved */
    frame_set_bits(frame, 40U, 1U, (uint32_t)msg->selected_altitude_source);
    frame_set_bits(frame, 41U, 11U, altitude_field);
    frame_set_bits(frame, 52U, 9U, pressure_field);
    frame_set_bits(frame, 61U, 1U, msg->selected_heading_valid ? 1U : 0U);
    frame_set_bits(frame, 62U, 9U, heading_field);
    frame_set_bits(frame, 71U, 4U, msg->nac_p);
    frame_set_bits(frame, 75U, 1U, msg->nic_baro ? 1U : 0U);
    frame_set_bits(frame, 76U, 2U, msg->sil);
    frame_set_bits(frame, 78U, 1U, msg->mode_status ? 1U : 0U);
    frame_set_bits(frame, 79U, 1U, msg->mode_status && msg->autopilot_engaged ? 1U : 0U);
    frame_set_bits(frame, 80U, 1U, msg->mode_status && msg->vnav_mode ? 1U : 0U);
    frame_set_bits(frame, 81U, 1U, msg->mode_status && msg->altitude_hold_mode ? 1U : 0U);
    frame_set_bits(frame, 82U, 1U, 0U);   /* IMF / ADS-R, reserved */
    frame_set_bits(frame, 83U, 1U, msg->mode_status && msg->approach_mode ? 1U : 0U);
    frame_set_bits(frame, 84U, 1U, msg->tcas_operational ? 1U : 0U);
    frame_set_bits(frame, 85U, 1U, msg->mode_status && msg->lnav_mode ? 1U : 0U);
    frame_set_bits(frame, 86U, 2U, 0U);   /* Reserved */

    adsb_apply_crc(frame);

    return ENC_OK;
}

enc_status_t adsb_encode_operational_status(const adsb_operational_status_t *msg, uint8_t frame[ADSB_FRAME_BYTES])
{
    if ((msg == NULL) || (frame == NULL))
    {
        return ENC_INVALID_ARGUMENT;
    }
    if (!is_valid_icao(msg->icao))
    {
        return ENC_INVALID_ICAO;
    }
    if (msg->subtype > 1U)
    {
        return ENC_INVALID_OPERATIONAL_SUBTYPE;
    }
    if (msg->adsb_version > 7U)
    {
        return ENC_INVALID_ADSB_VERSION;
    }
    if (msg->nac_p > 15U)
    {
        return ENC_INVALID_NAC_P;
    }
    if (msg->sil > 3U)
    {
        return ENC_INVALID_SIL;
    }

    adsb_frame_clear(frame);

    frame_set_bits(frame, 0U, 5U, ADSB_DF17);
    frame_set_bits(frame, 5U, 3U, ADSB_CA);
    frame_set_bits(frame, 8U, 24U, msg->icao & ADSB_ICAO_MAX);

    frame_set_bits(frame, 32U, 5U, 31U);  /* Type code 31 */
    frame_set_bits(frame, 37U, 3U, msg->subtype);
    frame_set_bits(frame, 40U, 16U, msg->capability_class);
    frame_set_bits(frame, 56U, 16U, msg->operational_mode);
    frame_set_bits(frame, 72U, 3U, msg->adsb_version);
    frame_set_bits(frame, 75U, 1U, msg->nic_supplement_a ? 1U : 0U);
    frame_set_bits(frame, 76U, 4U, msg->nac_p);
    frame_set_bits(frame, 80U, 2U, 0U);  /* GVA / reserved */
    frame_set_bits(frame, 82U, 2U, msg->sil);
    frame_set_bits(frame, 84U, 1U, msg->nic_baro ? 1U : 0U);
    frame_set_bits(frame, 85U, 1U, msg->heading_reference_magnetic ? 1U : 0U);
    frame_set_bits(frame, 86U, 1U, msg->sil_supplement ? 1U : 0U);
    frame_set_bits(frame, 87U, 1U, 0U);  /* Reserved */

    adsb_apply_crc(frame);

    return ENC_OK;
}
