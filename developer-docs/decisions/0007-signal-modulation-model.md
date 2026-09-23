# 0007. Modulation model and validation strategy for `adsblib_signal`

Status: Accepted

## Context

`ROADMAP.md` M2 adds RF/IQ signal synthesis as the optional module
`adsblib_signal` (see `0005-optional-modules-boundary.md`). Before
implementing it, the roadmap requires settling:

1. The waveform being produced and its sample representation.
2. Which sample rates are supported.
3. How the modulated output is validated without turning adsblib into a
   decoder (`0003-encoder-only-core.md`).

The 1090 MHz Extended Squitter waveform (ICAO Annex 10 Vol. IV / DO-260B)
is pulse position modulation at 1 Mbit/s:

- **Preamble**, 8 µs: four 0.5 µs pulses starting at 0.0, 1.0, 3.5 and
  4.5 µs.
- **Data block**, 112 µs: one bit per 1 µs, starting at 8.0 µs. A `1` bit
  is a pulse in the first 0.5 µs half of its bit period, a `0` bit a pulse
  in the second half.
- Total frame duration: **120 µs**. DF17 is always 112 bits, so the
  duration does not depend on the frame contents.

The smallest time unit of the waveform is the 0.5 µs chip.

## Decision

### Sample representation

The module produces **complex baseband IQ as 32-bit float, interleaved**
(`I0, Q0, I1, Q1, ...`, commonly "CF32"). This is the native format of
GNU Radio and SoapySDR, and the usual input format of SDR transmit
chains.

- A pulse chip is `I = amplitude, Q = 0`; a gap chip is `I = 0, Q = 0`.
  The signal is at 0 Hz with zero carrier phase. Frequency offset, phase
  rotation, noise, and gain are the caller's (or the transmit chain's)
  responsibility.
- `amplitude` is a caller parameter in `(0, 1]`, so full-scale output maps
  cleanly onto integer formats.
- Integer formats (CS8 for HackRF, CU8 for RTL-SDR-style `--ifile` input)
  are **not** part of the public API. Conversion from CF32 is a few lines
  of code, and it lives in the worked example and the test harness.
- Pulses are rectangular (no pulse shaping). Rise/fall-time shaping may be
  added later as an option. It is not needed for either of the target
  consumers.

### Sample rates

Only sample rates that are a **positive integer multiple of 2 MHz** are
accepted (2, 4, 6, 8, ... Msps). At these rates every 0.5 µs chip spans a
whole number of samples, so the output is exact and bit-for-bit
reproducible, with no timing jitter from rounding pulse edges. Every other
rate is rejected.

Consequence for sizing: a frame is always `120 µs × fs` samples, i.e.
`240 × (fs / 2 MHz)`. Examples: 240 samples at 2 Msps, 480 at 4 Msps.

### API shape

Following the caller-allocates pattern from `0005`:

```c
/* Complex samples needed for one modulated DF17 frame at the given rate,
 * or 0 if the rate is not a positive multiple of 2 MHz. */
size_t adsb_signal_sample_count(uint32_t sample_rate_hz);

/* Planned, not implemented yet: */
adsb_signal_status_t adsb_signal_modulate(
    const uint8_t frame[ADSB_FRAME_BYTES],
    uint32_t sample_rate_hz,
    float amplitude,
    float *iq,                  /* 2 * capacity floats, interleaved I/Q */
    size_t capacity_samples,
    size_t *samples_written
);
```

- Counts are in **complex samples**. The float buffer holds
  `2 × count` floats.
- `adsb_signal_sample_count()` returns `0` as its error sentinel instead
  of a status code. `0` is never a valid sample count, and this keeps the
  sizing call trivial to use.
- The module has its own status enum (`adsb_signal_status_t`) and does not
  add values to the core's `enc_status_t`. The core must not know the
  module exists (`0005`).
- `adsb_signal_modulate()` does **not** check the frame's CRC. Frames
  with deliberately corrupted parity are a legitimate input for testing a
  receiver's error handling. Garbage in, faithfully modulated garbage out.
- Leading/trailing silence and multi-frame bursts are handled by the
  caller, who allocates a larger buffer and offsets into it. The scenario
  module (M3) is the natural place for timed multi-frame output.

### Validation

Validation has two independent layers:

1. **Test-only demodulator** (`validation/test_signal.c`). A minimal PPM
   demodulator written as `static` functions inside the test program.
   It is never compiled into the library, never declared in a public
   header, and handles only adsblib's own clean, noise-free,
   known-alignment output. That is the "self-verification only" carve-out
   `0003` anticipates. It checks exact sample placement: the preamble
   positions, the chip at every bit, and silence everywhere else.
2. **External reference receiver in CI.** Modulated frames are converted
   to CU8 by the test harness, written to a file, and fed to an existing,
   independent Mode S receiver in file-input mode (`--ifile`). The job
   checks that the receiver reports exactly the frames that were
   modulated. This is the signal-level equivalent of the pyModeS
   cross-check. It guards against a bug shared by the modulator and the
   test demodulator, such as both using the wrong preamble timing.

The specific receiver is chosen when layer 2 is implemented. The main
constraint is that it must demodulate at a 2 MHz multiple. The 2 MHz
demodulator of the original dump1090 line qualifies. readsb and
dump1090-fa default to 2.4 MHz, which this ADR does not produce.

## Consequences

- Output is exact and deterministic, so tests can assert individual
  sample values instead of tolerances.
- Consumers that need 2.4 Msps (a common RTL-SDR rate) or other
  non-2 MHz-multiple rates must resample on their side. If demand appears,
  relaxing the rate constraint (rounding pulse edges to the nearest
  sample) is a backwards-compatible extension: rates that are accepted
  today keep producing identical output.
- Only CF32 is exposed, which keeps the API and test surface small.
  Users with integer-format hardware write a trivial conversion loop,
  which the worked example demonstrates.
- The external-receiver CI job adds a build-from-source dependency and
  some maintenance cost. That cost is accepted as the price of an
  independent reference.
- adsblib still exposes no decoding API. The test demodulator is test
  code, not library code.
