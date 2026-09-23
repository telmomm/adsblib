#ifndef ADSB_SIGNAL_H
#define ADSB_SIGNAL_H

#include "adsblib.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Signal Synthesis (optional module)
 *
 * Turns DF17 frames into 1090 MHz Extended Squitter baseband
 * samples. Depends on the core; the core never depends on it.
 * See developer-docs/decisions/0007-signal-modulation-model.md.
 * ============================================================ */

/* ============================================================
 * Waveform Definitions
 * ============================================================ */

/** Duration of the 1090ES preamble, in microseconds. */
#define ADSB_SIGNAL_PREAMBLE_US   8U

/** Duration of one modulated DF17 frame (preamble + 112 data bits), in microseconds. */
#define ADSB_SIGNAL_FRAME_US      (ADSB_SIGNAL_PREAMBLE_US + ADSB_FRAME_BITS)

/** Supported sample rates are positive integer multiples of this value. */
#define ADSB_SIGNAL_RATE_STEP_HZ  2000000U

/* ============================================================
 * Sizing
 * ============================================================ */

/**
 * Number of complex samples needed to hold one modulated DF17 frame.
 *
 * Samples are complex baseband, interleaved 32-bit float I/Q, so the
 * caller's float buffer must hold twice the returned count.
 *
 * @param sample_rate_hz  Sample rate in Hz. Must be a positive integer
 *                        multiple of ADSB_SIGNAL_RATE_STEP_HZ (2 MHz),
 *                        so that every 0.5 us pulse spans a whole number
 *                        of samples.
 *
 * @return Complex sample count (e.g. 240 at 2 MHz, 480 at 4 MHz), or 0 if
 *         the sample rate is not supported.
 */
size_t adsb_signal_sample_count(
    uint32_t sample_rate_hz
);

#ifdef __cplusplus
}
#endif

#endif /* ADSB_SIGNAL_H */
