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
#define ADSBLIB_VERSION_MINOR  1
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