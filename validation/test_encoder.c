#include "../adsblib.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return 1; \
        } \
    } while (0)

static uint32_t frame_get_bits(const uint8_t frame[ADSB_FRAME_BYTES], uint32_t start_bit, uint32_t num_bits)
{
    uint32_t value = 0U;
    for (uint32_t bit_index = 0U; bit_index < num_bits; ++bit_index)
    {
        uint32_t absolute_bit = start_bit + bit_index;
        uint32_t byte_index = absolute_bit / 8U;
        uint32_t bit_in_byte = 7U - (absolute_bit % 8U);

        value = (value << 1U) | ((frame[byte_index] >> bit_in_byte) & 0x1U);
    }

    return value;
}

static int test_frame_utilities(void)
{
    static const uint8_t known_frame[ADSB_FRAME_BYTES] = {
        0x8D, 0x40, 0x62, 0x1D, 0x58, 0xC3, 0x82,
        0xD6, 0x90, 0xC8, 0xAC, 0x28, 0x63, 0xA7
    };
    char hex[29];
    uint8_t cleared[ADSB_FRAME_BYTES];

    CHECK(adsb_verify_crc(known_frame));
    CHECK(adsb_crc24(known_frame, 88U) == 0x2863A7U);

    adsb_frame_to_hex(known_frame, hex);
    CHECK(strcmp(hex, "8D40621D58C382D690C8AC2863A7") == 0);

    memset(cleared, 0xFF, sizeof(cleared));
    adsb_frame_clear(cleared);
    CHECK(frame_get_bits(cleared, 0U, ADSB_FRAME_BITS) == 0U);

    CHECK(strcmp(adsb_version_string(), "0.1.0") == 0);
    return 0;
}

static int test_identification(void)
{
    adsb_identification_t message = {0xABCDEFU, "ECABC"};
    uint8_t frame[ADSB_FRAME_BYTES];

    CHECK(adsb_encode_identification(&message, frame) == ENC_OK);
    CHECK(frame_get_bits(frame, 0U, 5U) == 17U);
    CHECK(frame_get_bits(frame, 8U, 24U) == 0xABCDEFU);
    CHECK(frame_get_bits(frame, 32U, 5U) == 4U);
    CHECK(frame_get_bits(frame, 40U, 6U) == 5U);
    CHECK(adsb_verify_crc(frame));

    message.callsign[0] = '!';
    CHECK(adsb_encode_identification(&message, frame) == ENC_INVALID_CALLSIGN);
    CHECK(adsb_encode_identification(NULL, frame) == ENC_INVALID_ARGUMENT);
    CHECK(adsb_encode_identification(&message, NULL) == ENC_INVALID_ARGUMENT);
    return 0;
}

static int test_position(void)
{
    adsb_position_t message = {0xABCDEFU, 37.5, -122.1, 32000, CPR_EVEN};
    uint8_t frame[ADSB_FRAME_BYTES];

    CHECK(adsb_encode_position(&message, frame) == ENC_OK);
    CHECK(frame_get_bits(frame, 32U, 5U) == 11U);
    CHECK(frame_get_bits(frame, 40U, 12U) != 0U);
    CHECK(frame_get_bits(frame, 54U, 17U) == adsb_cpr_encode_latitude(message.latitude_deg, CPR_EVEN));
    CHECK(frame_get_bits(frame, 71U, 17U) == adsb_cpr_encode_longitude(message.latitude_deg, message.longitude_deg, CPR_EVEN));
    CHECK(adsb_verify_crc(frame));

    message.latitude_deg = 91.0;
    CHECK(adsb_encode_position(&message, frame) == ENC_INVALID_LATITUDE);
    message.latitude_deg = 37.5;
    message.altitude_ft = 60000;
    CHECK(adsb_encode_position(&message, frame) == ENC_INVALID_ALTITUDE);
    return 0;
}

static int test_surface_position(void)
{
    adsb_surface_position_t message = {0xABCDEFU, 37.5, -122.1, 25.0, 90.0, CPR_ODD};
    uint8_t frame[ADSB_FRAME_BYTES];

    CHECK(adsb_encode_surface_position(&message, frame) == ENC_OK);
    CHECK(frame_get_bits(frame, 32U, 5U) == 8U);
    CHECK(frame_get_bits(frame, 37U, 7U) > 1U);
    CHECK(frame_get_bits(frame, 44U, 1U) == 1U);
    CHECK(frame_get_bits(frame, 45U, 7U) == 32U);
    CHECK(frame_get_bits(frame, 53U, 1U) == 1U);
    CHECK(adsb_verify_crc(frame));

    message.ground_speed_kt = -1.0;
    CHECK(adsb_encode_surface_position(&message, frame) == ENC_INVALID_SPEED);
    message.ground_speed_kt = 25.0;
    message.ground_track_deg = 360.0;
    CHECK(adsb_encode_surface_position(&message, frame) == ENC_INVALID_TRACK);
    return 0;
}

static int test_velocity(void)
{
    adsb_velocity_t message = {0xABCDEFU, 600.0, 90.0, 640};
    uint8_t frame[ADSB_FRAME_BYTES];

    CHECK(adsb_encode_velocity(&message, frame) == ENC_OK);
    CHECK(frame_get_bits(frame, 32U, 5U) == 19U);
    CHECK(frame_get_bits(frame, 37U, 3U) == 1U);
    CHECK(adsb_verify_crc(frame));

    message.ground_speed_kt = 2000.0;
    CHECK(adsb_encode_velocity(&message, frame) == ENC_OK);
    CHECK(frame_get_bits(frame, 37U, 3U) == 2U);
    CHECK(adsb_verify_crc(frame));

    message.ground_speed_kt = -1.0;
    CHECK(adsb_encode_velocity(&message, frame) == ENC_INVALID_SPEED);
    message.ground_speed_kt = 600.0;
    message.vertical_rate_fpm = 32641;
    CHECK(adsb_encode_velocity(&message, frame) == ENC_INVALID_VERTICAL_RATE);
    return 0;
}

static int test_emergency(void)
{
    adsb_emergency_t message = {0xABCDEFU, ADSB_EMERGENCY_UNLAWFUL_INTERFERENCE, 07500U};
    uint8_t frame[ADSB_FRAME_BYTES];

    CHECK(adsb_encode_emergency(&message, frame) == ENC_OK);
    CHECK(frame_get_bits(frame, 32U, 5U) == 28U);
    CHECK(frame_get_bits(frame, 37U, 3U) == 1U);
    CHECK(frame_get_bits(frame, 40U, 3U) == ADSB_EMERGENCY_UNLAWFUL_INTERFERENCE);
    CHECK(frame_get_bits(frame, 43U, 13U) == 0xAA2U);
    CHECK(adsb_verify_crc(frame));

    message.mode_a_code = 010000U;
    CHECK(adsb_encode_emergency(&message, frame) == ENC_INVALID_MODE_A_CODE);
    return 0;
}

static int test_target_state(void)
{
    adsb_target_state_t message = {
        0xABCDEFU, 32000, ADSB_ALTITUDE_SOURCE_FMS, 1013.2,
        true, 90.0, 9U, true, 2U, true, true, true, true, false, true, true
    };
    uint8_t frame[ADSB_FRAME_BYTES];

    CHECK(adsb_encode_target_state(&message, frame) == ENC_OK);
    CHECK(frame_get_bits(frame, 32U, 5U) == 29U);
    CHECK(frame_get_bits(frame, 37U, 2U) == 1U);
    CHECK(frame_get_bits(frame, 40U, 1U) == 1U);
    CHECK(frame_get_bits(frame, 41U, 11U) == 1001U);
    CHECK(frame_get_bits(frame, 52U, 9U) == 268U);
    CHECK(frame_get_bits(frame, 62U, 9U) == 128U);
    CHECK(frame_get_bits(frame, 71U, 4U) == 9U);
    CHECK(adsb_verify_crc(frame));

    message.selected_heading_deg = 360.0;
    CHECK(adsb_encode_target_state(&message, frame) == ENC_INVALID_HEADING);
    return 0;
}

static int test_operational_status(void)
{
    adsb_operational_status_t message = {
        0xABCDEFU, 1U, 0xA55AU, 0x5AA5U, 2U,
        true, 12U, 3U, true, true, true
    };
    uint8_t frame[ADSB_FRAME_BYTES];

    CHECK(adsb_encode_operational_status(&message, frame) == ENC_OK);
    CHECK(frame_get_bits(frame, 32U, 5U) == 31U);
    CHECK(frame_get_bits(frame, 37U, 3U) == 1U);
    CHECK(frame_get_bits(frame, 40U, 16U) == 0xA55AU);
    CHECK(frame_get_bits(frame, 56U, 16U) == 0x5AA5U);
    CHECK(frame_get_bits(frame, 72U, 3U) == 2U);
    CHECK(frame_get_bits(frame, 76U, 4U) == 12U);
    CHECK(frame_get_bits(frame, 82U, 2U) == 3U);
    CHECK(frame_get_bits(frame, 84U, 1U) == 1U);
    CHECK(frame_get_bits(frame, 85U, 1U) == 1U);
    CHECK(frame_get_bits(frame, 86U, 1U) == 1U);
    CHECK(adsb_verify_crc(frame));

    message.subtype = 2U;
    CHECK(adsb_encode_operational_status(&message, frame) == ENC_INVALID_OPERATIONAL_SUBTYPE);
    message.subtype = 1U;
    message.adsb_version = 8U;
    CHECK(adsb_encode_operational_status(&message, frame) == ENC_INVALID_ADSB_VERSION);
    return 0;
}

int main(void)
{
    int failures = 0;

    failures += test_frame_utilities();
    failures += test_identification();
    failures += test_position();
    failures += test_surface_position();
    failures += test_velocity();
    failures += test_emergency();
    failures += test_target_state();
    failures += test_operational_status();

    if (failures != 0)
    {
        fprintf(stderr, "%d test group(s) failed\n", failures);
        return 1;
    }

    printf("C unit tests passed\n");
    return 0;
}
