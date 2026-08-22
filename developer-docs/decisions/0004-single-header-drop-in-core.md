# 0004. Single header + single translation unit for the core

Status: Accepted

## Context

adsblib targets embedded and experimental-avionics projects, which often
have ad hoc or constrained build systems (vendor IDEs, Makefiles hand-
written per board, no package manager). Requiring a build system, a
library-specific installation step, or multiple source files to track
adds friction disproportionate to the size of the problem being solved
(one 112-bit frame encoder).

## Decision

The core ships as exactly one public header (`adsblib.h`) and one
implementation file (`adsblib.c`), with no required build system. A
consumer can either compile it as a static/shared library (see root
`README.md#building`) or simply drop both files into their own project
tree and compile them as part of their existing build.

## Consequences

- Minimal integration friction — this is a deliberate usability choice
  for the target audience, not just an artifact of the project's current
  size.
- As `ROADMAP.md` milestones land, new capabilities (signal synthesis,
  scenario generation) are added as their *own* additional header/source
  pairs (e.g. `adsblib_signal.h`/`.c`) rather than growing `adsblib.c`
  monolithically — see `0005-optional-modules-boundary.md`. This decision
  specifically covers the *core's* shape; it does not mandate that the
  whole project stay single-file forever.
- A future CMake/package-manager story (`ROADMAP.md` M5) must keep the
  drop-in path working as an equally valid option, not replace it.
