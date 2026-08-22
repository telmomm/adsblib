# 0002. No dynamic memory allocation in the core library

Status: Accepted

## Context

adsblib's core use case includes experimental avionics and embedded/SDR
test contexts, where `malloc`/`free` are often unavailable, discouraged,
or a source of nondeterministic latency and fragmentation risk that
matters more than in general application code. Encoding a single DF17
frame is also a bounded, fixed-size problem (14 bytes, fixed-size input
structs) — there is no inherent need for dynamic sizing.

## Decision

`adsblib.c` performs no dynamic memory allocation (no `malloc` / `calloc`
/ `realloc` / `free`). All buffers are fixed-size and caller-owned; public
functions write into buffers the caller provides rather than returning
allocated memory.

This is a hard constraint, not a style preference — see
`CONTRIBUTING.md`.

## Consequences

- Deterministic memory behavior: no fragmentation, no allocation failure
  paths to handle, usable in allocation-restricted environments.
- Callers are responsible for buffer sizing (mitigated by fixed-size
  constants like `ADSB_FRAME_BYTES` in the public API).
- This constraint is easy to state but not automatically true for future
  problems that are *not* fixed-size by nature (e.g. an arbitrary-length
  IQ sample stream, or a scenario with an arbitrary number of aircraft).
  Rather than weaken the guarantee for the core to accommodate those, new
  capabilities with genuinely variable-size output are split into
  separate optional modules that can make their own allocation tradeoff
  (typically: a sizing function so the *caller* allocates, keeping the
  module itself allocation-free). See
  `0005-optional-modules-boundary.md`.
