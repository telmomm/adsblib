# Contributing to adsblib

Thanks for your interest in contributing to adsblib. This document explains how to build the
project, run the validation suite, and submit changes.

## Ground rules

- adsblib targets **C99** and must build warning-free with `-Wall -Wextra -Werror`.
- The library must not perform any dynamic memory allocation (no `malloc`/`calloc`/`realloc`
  in `adsblib.c`). This is a hard design constraint, not a style preference.
- Keep the public API in `adsblib.h` minimal and documented with Doxygen comments.
- All notable changes must be recorded in `CHANGELOG.md` following
  [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## Building

### macOS (.dylib)

```bash
cc -std=c99 -Wall -Wextra -Werror -dynamiclib -o adsblib.dylib adsblib.c -lm
```

### Linux (.so)

```bash
cc -std=c99 -Wall -Wextra -Werror -fPIC -shared -o adsblib.so adsblib.c -lm
```

Verify the build succeeded:

```bash
ls -lh adsblib.dylib adsblib.so
```

## Validation

Encoder correctness is currently validated through `encoder_validation.ipynb`, which
cross-checks adsblib's output against [pyModeS](https://github.com/junzis/pyModeS) (CRC,
callsign, CPR, altitude, velocity, and stress tests).

Requirements: Python 3 and `pyModeS`.

```bash
pip install pyModeS
jupyter notebook encoder_validation.ipynb
```

If you add or change encoding behavior, extend the notebook with cases that cover it,
including edge cases and invalid inputs, not just the happy path.

## Code style

- Follow the existing formatting in `adsblib.c` / `adsblib.h` (tabs for indentation, braces
  on their own line).
- Prefer explicit, bounded loops and fixed-size buffers over anything that could allocate or
  grow unbounded — this library is meant to be usable in constrained/embedded contexts.
- Document every public function, struct, and enum with Doxygen-style comments (parameters,
  return values, and any preconditions).

## Submitting changes

1. Fork the repository and create a branch from `main`.
2. Make your change, keeping commits focused and descriptive.
3. Make sure the library still builds warning-free and the validation notebook passes.
4. Update `CHANGELOG.md` under an `[Unreleased]` section describing your change.
5. Open a pull request describing what changed and why.

## Reporting issues

Please use the GitHub issue templates (bug report / feature request) when opening an issue.
Include:

- adsblib version/commit, compiler, and OS.
- A minimal reproduction (input values, expected vs. actual output).
- Any relevant output from the validation notebook, if applicable.

## Code of Conduct

This project follows the [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are
expected to uphold it.
