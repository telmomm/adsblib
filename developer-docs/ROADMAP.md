# Capability roadmap

This is the *technical* roadmap: what adsblib needs to become to fully
cover its [Statement of Need](../README.md#statement-of-need) and to offer
real capabilities the existing decode-oriented ecosystem doesn't. For the
separate question of what a JOSS submission additionally requires
(commit history, evidence of research use, `paper.md`), see
[`JOSS_ROADMAP.md`](JOSS_ROADMAP.md).

Milestones are ordered by priority, not necessarily by calendar time — M2
(signal synthesis) is more valuable than M1 (spec completeness) in terms
of differentiation, but M1 is cheap and closes a credibility gap in the
current README, so it comes first.

## M0 — Repository foundations — **done**

Statement of Need, comparison with related software, `CONTRIBUTING.md`,
`CODE_OF_CONDUCT.md`, `SECURITY.md`, `CITATION.cff`, issue templates,
CI-generated (not committed) API docs, library version macros. Tracked in
`CHANGELOG.md` under `[0.1.0]`.

## M1 — Full DF17 message-type coverage

Today adsblib encodes 3 of the DF17 message families (identification,
airborne position, airborne velocity). "Encoder" is currently an
overstatement relative to the DF17 spec. Close that gap:

- [x] Surface position (Type Codes 5–8). Implemented as
      `adsb_encode_surface_position()` (fixed TC 8; NIC selection not yet
      exposed, matching `adsb_encode_position()`'s existing TC 11 default).
- [x] Supersonic airborne velocity subtype. `adsb_encode_velocity()` now
      selects subtype 1 (subsonic, 1 kt/LSB) or subtype 2 (supersonic,
      4 kt/LSB) automatically, per velocity component.
- [x] Aircraft status / emergency & priority status (Type Code 28), via
      `adsb_encode_emergency()` (subtype 1 with Mode A/Gillham encoding).
- [x] Target state and status (Type Code 29), via
      `adsb_encode_target_state()` (subtype 1 / BDS 6,2).
- [x] Aircraft operational status (Type Code 31), via
      `adsb_encode_operational_status()` (BDS 6,5).
- [x] Extended `validation/encoder_validation.ipynb` with pyModeS
      cross-checks for the implemented message types (TEST 7–11) — same
      "validate against an independent decoder" bar as the existing
      tests.

**Why this order:** it's the cheapest milestone (same encode-only pattern
as existing code, no new architectural concerns) and it's what makes
"encoder" in the README's Statement of Need actually true instead of
"encoder for a subset of DF17."

## M2 — RF/IQ signal synthesis (`adsblib_signal`)

The actual differentiator: none of dump1090/readsb/pyModeS can turn a
DF17 frame into a transmittable or injectable RF signal — they only
decode. See [`ARCHITECTURE.md`](ARCHITECTURE.md#target-layout) for the
module boundary and [`decisions/0005-optional-modules-boundary.md`](decisions/0005-optional-modules-boundary.md)
for why this lives outside the core.

- [ ] Define the modulation model: 1090ES uses PPM (pulse position
      modulation) at 1 Mbit/s with a fixed preamble; settle the exact
      sample representation (real-valued envelope vs. complex IQ) based
      on target consumers (SDR TX chains typically want complex IQ).
- [ ] `adsb_signal_sample_count()` — given a sample rate, return the
      number of samples a modulated frame will need, so the *caller*
      allocates (keeps the module itself allocation-free).
- [ ] `adsb_signal_modulate()` — DF17 frame + sample rate + caller buffer
      -> modulated samples.
- [ ] Validate modulated output by demodulating it back (either with a
      minimal internal demodulator used only for self-test, or by
      round-tripping through an external tool such as `dump1090` in a
      test harness) and confirming it decodes to the original frame.
- [ ] Document target sample formats/rates and at least one worked
      example of feeding output to a real SDR TX path or a receiver's
      test input.

**Open design question to resolve before implementation:** whether
"validate by demodulating" requires adsblib to grow an internal decoder.
If so, that decoder must stay scoped to *self-verification of adsblib's
own output* — see [`decisions/0003-encoder-only-core.md`](decisions/0003-encoder-only-core.md)
for why it should not become a general-purpose public decoding API.

## M3 — Scenario / trajectory generation (`adsblib_scenario`)

- [ ] Define an aircraft state model (position, velocity, heading,
      vertical rate) that evolves over discrete time steps.
- [ ] `adsb_scenario_step()` (or similar) — advance one or more aircraft
      states by one time step and emit the DF17 frames a real transponder
      would squitter in that interval, using realistic squitter timing
      (e.g. ~0.4–0.6s for airborne position, ~1s for identification).
- [ ] Support multiple simultaneous simulated aircraft (distinct ICAO
      addresses) sharing one scenario clock.
- [ ] At least one worked example: a synthetic multi-aircraft traffic
      generator producing a timestamped frame stream, usable as
      relatively realistic input for decoder/receiver testing or dataset
      generation.

**Why this matters beyond being a nice-to-have:** `JOSS_ROADMAP.md`'s
Fase 3 identifies "evidence of use in research" as the hardest JOSS gate
to clear. A scenario generator that produces synthetic ADS-B datasets is
exactly the kind of concrete, documented downstream use case that gate
asks for — it's both a technical capability and a path to satisfying that
requirement.

## M4 — Robustness

Tracked in detail in `JOSS_ROADMAP.md` Fase 1–2 (unit tests, CI matrix
build, sanitizers, fuzzing, coverage). Applies to every module above as it
lands, not just the current core — no module (signal, scenario) ships
without the same "validated against an independent reference, not just
itself" bar the core holds itself to.

## M5 — Packaging and distribution

Tracked in `JOSS_ROADMAP.md` Fase 4: CMake support, package manager
listing, Zenodo DOI archival.

## Non-goals (see `MISSION.md` for the full list)

General-purpose ADS-B decoding, non-DF17 downlink formats, network
distribution protocols (SBS/Beast), GUI/visualization, and certified
avionics use are explicitly out of scope. If a milestone above starts
requiring one of these, that's a signal to open an ADR rather than to
quietly expand scope.
