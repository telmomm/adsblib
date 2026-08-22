# Mission, vision, and scope

## Mission

Provide a portable, deterministic, dependency-light C99 core for
generating standards-compliant ADS-B DF17 (Extended Squitter) messages —
and, built on top of that core, the additional tooling needed to turn
those messages into usable synthetic ADS-B traffic: RF-ready signal, and
realistic multi-message aircraft scenarios.

## Vision

The existing open-source ADS-B ecosystem (dump1090, readsb, pyModeS) is
almost entirely oriented around *receiving and decoding* real traffic.
adsblib's long-term goal is to be the reference toolkit for the inverse
problem — generating correct, synthetic ADS-B traffic — at every layer
someone might need it:

1. **Frame bytes** — a correct, spec-compliant 112-bit DF17 frame. *(done today)*
2. **RF signal** — the same frame as PPM/Manchester-modulated IQ samples,
   ready to feed an SDR transmitter or to inject directly into a
   receiver's decode pipeline for testing. *(planned)*
3. **Scenario** — a realistic sequence of frames describing one or more
   aircraft moving over time, at correct squitter intervals, for
   generating synthetic traffic datasets or exercising a decoder/receiver
   under realistic load. *(planned)*

Each layer is optional and builds on the one below it. Someone who only
needs correct frame bytes for an embedded project should never have to pay
(in dependencies, allocation, or complexity) for signal synthesis or
scenario generation they don't use. See
[`ARCHITECTURE.md`](ARCHITECTURE.md) for how that boundary is enforced,
and [`decisions/0005-optional-modules-boundary.md`](decisions/0005-optional-modules-boundary.md)
for why.

## Scope

### In scope

- Encoding DF17 Extended Squitter messages: aircraft identification,
  airborne and surface position (CPR), airborne velocity (subsonic and
  supersonic), aircraft status/emergency, target state and status,
  operational status.
- Mode-S CRC24 computation, application, and verification.
- Frame-level utilities (hex formatting, buffer helpers).
- (Planned) PPM/Manchester modulation of DF17 frames into IQ sample
  buffers, at a configurable sample rate.
- (Planned) Generation of realistic multi-frame scenarios for one or more
  simulated aircraft over time.
- Cross-validation of encoder output against an independent reference
  decoder (currently pyModeS).

### Explicitly out of scope

- **Decoding arbitrary received ADS-B traffic.** That problem is already
  well served by dump1090, readsb, and pyModeS; adsblib complements them
  rather than re-implementing them. adsblib may *decode its own output*
  internally where that's useful for self-verification, but it is not a
  general-purpose Mode-S/ADS-B decoder.
- **Non-DF17 downlink formats** (DF4/5/20/21 Comm-B, DF11 all-call,
  Mode A/C) — may be revisited if a concrete need arises, but is not
  currently planned.
- **Network protocols** (SBS/BaseStation, Beast binary) for distributing
  generated traffic — out of scope unless a specific downstream use case
  requires it.
- **Certified avionics use.** adsblib is experimental and has not been
  evaluated against DO-178C or any comparable safety standard. See
  [`SECURITY.md`](../SECURITY.md).
- **GUI/visualization.** adsblib is a library, not an application.

## Design principles

These hold for the **core encoder** (`adsblib.h`/`adsblib.c`) at all
times, and are the default for any new module unless an ADR explicitly
justifies an exception:

- **No dynamic memory allocation.** Callers own all buffers. See
  [`decisions/0002-no-dynamic-allocation.md`](decisions/0002-no-dynamic-allocation.md).
- **Deterministic.** Same inputs always produce the same outputs; no
  hidden I/O, randomness, or global mutable state in the encoding path.
- **C99, portable, dependency-light.** No dependencies beyond the C
  standard library and `libm`. See
  [`decisions/0001-c99-target.md`](decisions/0001-c99-target.md).
- **Validated against an independent reference,** not just tested against
  itself — the point of the pyModeS cross-check is that a bug shared
  between the encoder and its own tests would go undetected otherwise.
