#ifndef ADSB_ENCODER_H
#define ADSB_ENCODER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Library Version
 * ============================================================ */

#define ADSBLIB_VERSION_MAJOR  0
#define ADSBLIB_VERSION_MINOR  2
#define ADSBLIB_VERSION_PATCH  0

/**
 * Get the library version as a "MAJOR.MINOR.PATCH" string.
 */
const char *adsb_version_string(void);

/* ============================================================
 * General Definitions
 * ============================================================ */

#define ADSB_FRAME_BYTES      14U
#define ADSB_FRAME_BITS       112U
#define ADSB_ICAO_MAX         0xFFFFFFU

#define ADSB_CALLSIGN_LEN     8U

/* ============================================================
 * Status Codes
 * ============================================================ */

typedef enum
{
    ENC_OK = 0,

    ENC_INVALID_ARGUMENT,
    ENC_INVALID_ICAO,
    ENC_INVALID_LATITUDE,
    ENC_INVALID_LONGITUDE,
    ENC_INVALID_ALTITUDE,
    ENC_INVALID_CALLSIGN,
    ENC_INVALID_SPEED,
    ENC_INVALID_TRACK,
    ENC_INVALID_VERTICAL_RATE,
    ENC_INVALID_EMERGENCY_STATE,
    ENC_INVALID_MODE_A_CODE,
    ENC_INVALID_SELECTED_ALTITUDE,
    ENC_INVALID_BARO_PRESSURE,
    ENC_INVALID_NAC_P,
    ENC_INVALID_SIL,
    ENC_INVALID_HEADING,
    ENC_INVALID_OPERATIONAL_SUBTYPE,
    ENC_INVALID_ADSB_VERSION,

    ENC_CPR_ERROR,
    ENC_INTERNAL_ERROR

} enc_status_t;

/* ============================================================
 * CPR Frame Type
 * ============================================================ */

typedef enum
{
    CPR_EVEN = 0,
    CPR_ODD  = 1

} cpr_format_t;

/* ============================================================
 * Aircraft Identification
 * ============================================================ */

typedef struct
{
    uint32_t icao;
    char callsign[ADSB_CALLSIGN_LEN + 1];

} adsb_identification_t;

/* ============================================================
 * Airborne Position
 * ============================================================ */

typedef struct
{
    uint32_t icao;

    double latitude_deg;
    double longitude_deg;

    int32_t altitude_ft;

    cpr_format_t cpr_format;

} adsb_position_t;

/* ============================================================
 * Surface Position
 * ============================================================ */

typedef struct
{
    uint32_t icao;

    double latitude_deg;
    double longitude_deg;

    /* Ground speed in knots (>= 0). The surface position message encodes
     * speed with a non-linear resolution (as fine as 0.125 kt at low
     * speed, as coarse as 5 kt approaching 175 kt); values >= 175 kt are
     * encoded as the message's open-ended "175 kt or more" code. */
    double ground_speed_kt;

    /* Ground track in degrees [0, 360), quantized to steps of 360/128
     * degrees (~2.8125 deg) by the message format. */
    double ground_track_deg;

    cpr_format_t cpr_format;

} adsb_surface_position_t;

/* ============================================================
 * Airborne Velocity
 * ============================================================ */

typedef struct
{
    uint32_t icao;

    /* Ground speed in knots (>= 0). Encoded as subtype 1 (subsonic,
     * 1 kt/LSB) when both velocity components fit within +/-1022 kt;
     * subtype 2 (supersonic, 4 kt/LSB, up to +/-4088 kt per component)
     * is selected automatically otherwise. */
    double ground_speed_kt;

    double track_deg;

    int32_t vertical_rate_fpm;

} adsb_velocity_t;

/* ============================================================
 * Emergency / Priority Status
 * ============================================================ */

typedef enum
{
    ADSB_EMERGENCY_NONE = 0,
    ADSB_EMERGENCY_GENERAL,
    ADSB_EMERGENCY_LIFEGUARD,
    ADSB_EMERGENCY_MINIMUM_FUEL,
    ADSB_EMERGENCY_NO_COMMUNICATIONS,
    ADSB_EMERGENCY_UNLAWFUL_INTERFERENCE,
    ADSB_EMERGENCY_DOWNED_AIRCRAFT

} adsb_emergency_state_t;

typedef struct
{
    uint32_t icao;

    adsb_emergency_state_t emergency_state;

    /* Four-digit Mode A code, represented as an octal value (0000..7777). */
    uint16_t mode_a_code;

} adsb_emergency_t;

/* ============================================================
 * Target State and Status
 * ============================================================ */

typedef enum
{
    ADSB_ALTITUDE_SOURCE_MCP = 0,
    ADSB_ALTITUDE_SOURCE_FMS = 1

} adsb_altitude_source_t;

typedef struct
{
    uint32_t icao;

    /* -1 means not available; otherwise encoded at 32 ft resolution. */
    int32_t selected_altitude_ft;
    adsb_altitude_source_t selected_altitude_source;

    /* -1.0 means not available; otherwise encoded at 0.8 mbar resolution. */
    double barometric_pressure_mbar;

    bool selected_heading_valid;
    double selected_heading_deg;

    uint8_t nac_p;
    bool nic_baro;
    uint8_t sil;

    bool mode_status;
    bool autopilot_engaged;
    bool vnav_mode;
    bool altitude_hold_mode;
    bool approach_mode;
    bool tcas_operational;
    bool lnav_mode;

} adsb_target_state_t;

/* ============================================================
 * Aircraft Operational Status
 * ============================================================ */

typedef struct
{
    uint32_t icao;

    /* Subtype 0 is airborne; subtype 1 is surface for ADS-B v1/v2. */
    uint8_t subtype;

    /* Version- and subtype-specific BDS 6,5 bit regions, kept raw. */
    uint16_t capability_class;
    uint16_t operational_mode;

    uint8_t adsb_version;
    bool nic_supplement_a;
    uint8_t nac_p;
    uint8_t sil;
    bool nic_baro;
    bool heading_reference_magnetic;
    bool sil_supplement;

} adsb_operational_status_t;

/* ============================================================
 * Public Encoding Functions
 * ============================================================ */

/**
 * Encode DF17 Aircraft Identification message.
 */
enc_status_t adsb_encode_identification(
    const adsb_identification_t *msg,
    uint8_t frame[ADSB_FRAME_BYTES]
);

/**
 * Encode DF17 Airborne Position message.
 */
enc_status_t adsb_encode_position(
    const adsb_position_t *msg,
    uint8_t frame[ADSB_FRAME_BYTES]
);

/**
 * Encode DF17 Surface Position message (Type Code 8).
 */
enc_status_t adsb_encode_surface_position(
    const adsb_surface_position_t *msg,
    uint8_t frame[ADSB_FRAME_BYTES]
);

/**
 * Encode DF17 Airborne Velocity message.
 *
 * Automatically selects subtype 1 (subsonic) or subtype 2 (supersonic,
 * 4 kt/LSB) based on the encoded ground speed. See adsb_velocity_t.
 */
enc_status_t adsb_encode_velocity(
    const adsb_velocity_t *msg,
    uint8_t frame[ADSB_FRAME_BYTES]
);

/**
 * Encode DF17 Aircraft Status / Emergency and Priority Status message
 * (Type Code 28, subtype 1).
 */
enc_status_t adsb_encode_emergency(
    const adsb_emergency_t *msg,
    uint8_t frame[ADSB_FRAME_BYTES]
);

/**
 * Encode DF17 Target State and Status message (Type Code 29, subtype 1).
 */
enc_status_t adsb_encode_target_state(
    const adsb_target_state_t *msg,
    uint8_t frame[ADSB_FRAME_BYTES]
);

/**
 * Encode DF17 Aircraft Operational Status message
 * (Type Code 31, BDS 6,5).
 */
enc_status_t adsb_encode_operational_status(
    const adsb_operational_status_t *msg,
    uint8_t frame[ADSB_FRAME_BYTES]
);

/* ============================================================
 * Utility Functions
 * ============================================================ */

/**
 * Calculate Mode-S CRC24.
 *
 * Input:
 *   First 11 bytes (88 bits) of DF17 frame.
 *
 * Output:
 *   24-bit CRC value.
 */
uint32_t adsb_crc24(
    const uint8_t *data,
    uint32_t num_bits
);

/**
 * Insert CRC parity into frame.
 */
void adsb_apply_crc(
    uint8_t frame[ADSB_FRAME_BYTES]
);

/**
 * Verify CRC validity.
 */
bool adsb_verify_crc(
    const uint8_t frame[ADSB_FRAME_BYTES]
);

/* ============================================================
 * CPR Functions
 * ============================================================ */

/**
 * Encode CPR latitude.
 */
uint32_t adsb_cpr_encode_latitude(
    double latitude_deg,
    cpr_format_t format
);

/**
 * Encode CPR longitude.
 */
uint32_t adsb_cpr_encode_longitude(
    double latitude_deg,
    double longitude_deg,
    cpr_format_t format
);

/**
 * NL(latitude) function.
 */
int adsb_cpr_nl(
    double latitude_deg
);

/* ============================================================
 * Frame Utilities
 * ============================================================ */

/**
 * Convert frame to hexadecimal string.
 *
 * Output buffer size must be >= 29 bytes.
 */
void adsb_frame_to_hex(
    const uint8_t frame[ADSB_FRAME_BYTES],
    char hex_string[29]
);

/**
 * Clear frame contents.
 */
void adsb_frame_clear(
    uint8_t frame[ADSB_FRAME_BYTES]
);

#ifdef __cplusplus
}
#endif

#endif /* ADSB_ENCODER_H */