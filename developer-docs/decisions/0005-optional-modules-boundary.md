# 0005. Optional modules boundary for signal synthesis and scenario generation

Status: Accepted

## Context

`ROADMAP.md` targets two major new capabilities beyond the current frame
encoder: RF/IQ signal synthesis (M2) and multi-aircraft scenario
generation (M3). Both are legitimately harder to make allocation-free and
fixed-size than frame encoding is:

- IQ signal synthesis produces a sample buffer whose size depends on
  sample rate and frame count — not a fixed constant like
  `ADSB_FRAME_BYTES`.
- Scenario generation produces a variable number of frames over time for
  a variable number of simulated aircraft.

Folding either directly into `adsblib.c` would force a choice between
weakening the core's "no dynamic allocation" guarantee for *all* users
(see `0002-no-dynamic-allocation.md`), or contorting these genuinely
variable-size problems into awkward fixed-capacity APIs that don't fit
the problem.

## Decision

Signal synthesis and scenario generation are implemented as separate,
optional header/source pairs (`adsblib_signal.h`/`.c`,
`adsblib_scenario.h`/`.c`) that depend on the core but that the core never
depends on. Each optional module may make its own allocation tradeoff
appropriate to its problem — the preferred pattern is a sizing function
(e.g. `adsb_signal_sample_count()`) that lets the *caller* allocate, so
the module's own code stays allocation-free even though the overall
capability handles variable-size data.

A consumer who only links `adsblib.c`/`adsblib.h` pulls in none of this —
no additional dependencies, no relaxed guarantees, no larger binary.

## Consequences

- The core's guarantees (`0002`, `0004`) stay true unconditionally,
  regardless of what else ships in the repository.
- Each optional module can be evaluated and adopted independently — a
  project that wants scenario generation but not signal synthesis links
  only what it needs.
- Slightly more build/documentation surface (multiple header/source pairs
  instead of one) — accepted as the cost of not compromising the core.
- Establishes the pattern for *any* future capability: default to a new
  optional module rather than growing the core, unless a new ADR argues
  otherwise for a specific case.
