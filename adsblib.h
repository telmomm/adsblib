#ifndef ADSB_ENCODER_H
#define ADSB_ENCODER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * Airborne Velocity
 * ============================================================ */

typedef struct
{
    uint32_t icao;

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
 * Encode DF17 Airborne Velocity message.
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