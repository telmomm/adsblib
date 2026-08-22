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

enc_status_t adsb_encode_velocity(const adsb_velocity_t *msg, uint8_t frame[ADSB_FRAME_BYTES])
{
    double track_rad;
    double v_east;
    double v_north;
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

    ew_speed = (int32_t)lround(fabs(v_east));
    ns_speed = (int32_t)lround(fabs(v_north));

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
    frame_set_bits(frame, 37U, 3U, 1U);    /* Subtype 1: ground speed */
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
