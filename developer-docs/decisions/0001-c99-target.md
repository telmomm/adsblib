# 0001. Target the C99 standard

Status: Accepted

## Context

adsblib is positioned as a reusable encoding core for experimental
avionics and embedded contexts (see `MISSION.md`). Those environments
commonly run older or vendor-constrained toolchains, so the choice of C
standard directly affects who can use the library at all.

## Decision

Target C99 (`-std=c99`), compiled warning-free with `-Wall -Wextra
-Werror`. Do not use C11/C17/C23-only features (`_Generic`, anonymous
structs/unions, `<stdatomic.h>`, etc.) in the public API or core
implementation.

## Consequences

- Broad toolchain compatibility, including older embedded/vendor
  compilers that may lag behind the newest C standards.
- No access to newer standard-library conveniences (e.g. `static_assert`
  without a macro shim); minor ergonomic cost, accepted deliberately.
- Any future module (signal synthesis, scenario generation) inherits this
  constraint unless an ADR explicitly carves out an exception for it —
  see `0005-optional-modules-boundary.md` for how modules can diverge from
  the core's constraints when justified.
