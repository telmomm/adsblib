#include "adsblib_signal.h"

/* ============================================================
 * Internal Constants
 * ============================================================ */

/* Samples per 0.5 us chip at the base rate step (2 MHz). */
#define ADSB_SIGNAL_CHIPS_PER_US  2U

/* ============================================================
 * Sizing
 * ============================================================ */

size_t adsb_signal_sample_count(uint32_t sample_rate_hz)
{
    if (sample_rate_hz == 0U || (sample_rate_hz % ADSB_SIGNAL_RATE_STEP_HZ) != 0U)
    {
        return 0U;
    }

    size_t samples_per_chip = (size_t)(sample_rate_hz / ADSB_SIGNAL_RATE_STEP_HZ);

    return (size_t)ADSB_SIGNAL_FRAME_US * ADSB_SIGNAL_CHIPS_PER_US * samples_per_chip;
}
