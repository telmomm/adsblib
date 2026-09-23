#include "../adsblib_signal.h"

#include <stdio.h>

#define CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return 1; \
        } \
    } while (0)

static int test_sample_count(void)
{
    CHECK(ADSB_SIGNAL_FRAME_US == 120U);

    CHECK(adsb_signal_sample_count(2000000U) == 240U);
    CHECK(adsb_signal_sample_count(4000000U) == 480U);
    CHECK(adsb_signal_sample_count(6000000U) == 720U);
    CHECK(adsb_signal_sample_count(20000000U) == 2400U);

    /* Largest supported rate representable in uint32_t. */
    CHECK(adsb_signal_sample_count(4294000000U) == 515280U);

    return 0;
}

static int test_sample_count_rejects_unsupported_rates(void)
{
    CHECK(adsb_signal_sample_count(0U) == 0U);
    CHECK(adsb_signal_sample_count(1000000U) == 0U);
    CHECK(adsb_signal_sample_count(2400000U) == 0U);
    CHECK(adsb_signal_sample_count(2000001U) == 0U);
    CHECK(adsb_signal_sample_count(3000000U) == 0U);
    CHECK(adsb_signal_sample_count(UINT32_MAX) == 0U);

    return 0;
}

int main(void)
{
    int failures = 0;

    failures += test_sample_count();
    failures += test_sample_count_rejects_unsupported_rates();

    if (failures != 0)
    {
        fprintf(stderr, "%d test group(s) failed\n", failures);
        return 1;
    }

    printf("Signal unit tests passed\n");
    return 0;
}
