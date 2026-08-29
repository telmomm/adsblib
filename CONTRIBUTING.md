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

Encoder correctness is validated through
[`validation/encoder_validation.ipynb`](validation/encoder_validation.ipynb), which
cross-checks adsblib's output against [pyModeS](https://github.com/junzis/pyModeS) — CRC,
callsign, CPR, altitude, velocity (subsonic and supersonic), surface position, and stress
tests.

The notebook loads adsblib through `ctypes`, so **you must build the shared library before
running it**, and it must be built *into the `validation/` directory* (the notebook looks for
`adsblib.dylib`/`adsblib.so` next to itself, not at the repo root):

```bash
# macOS
cc -std=c99 -Wall -Wextra -Werror -dynamiclib -o validation/adsblib.dylib adsblib.c -lm

# Linux
cc -std=c99 -Wall -Wextra -Werror -fPIC -shared -o validation/adsblib.so adsblib.c -lm

python3 -m venv validation/.venv
source validation/.venv/bin/activate   # Windows: validation\.venv\Scripts\activate
pip install -r validation/requirements.txt
jupyter notebook validation/encoder_validation.ipynb
```

Then run all cells (Run All / Restart & Run All). Notes:

- **Recompile and restart the kernel after changing `adsblib.c`/`.h`.** `ctypes.CDLL` loads
  the shared library once per process; re-running "Run All" without restarting the kernel can
  silently keep testing the previous build. Use *Kernel → Restart & Run All*, not just
  *Run All*, whenever the library changed.
- The built `.dylib`/`.so` in `validation/` is a local artifact (already covered by
  `.gitignore`) — don't commit it.
- If you'd rather run it headlessly instead of opening Jupyter (e.g. to sanity-check from a
  terminal or a script), `nbclient` executes the notebook end-to-end and writes the outputs
  back into the `.ipynb`:

  ```bash
  pip install nbclient nbformat ipykernel
  python3 -c "
  import nbformat
  from nbclient import NotebookClient
  nb = nbformat.read('validation/encoder_validation.ipynb', as_version=4)
  NotebookClient(nb, timeout=180, kernel_name='python3').execute()
  nbformat.write(nb, 'validation/encoder_validation.ipynb')
  "
  ```

If you add or change encoding behavior, extend the notebook with cases that cover it,
including edge cases and invalid inputs, not just the happy path — see `TEST 7`/`TEST 8` for
the current examples of adding a new message type's cross-check.

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
