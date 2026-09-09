# adsblib

[![Documentation Status](https://img.shields.io/website?label=Documentation%20Status&url=https%3A%2F%2Ftelmomm.github.io%2Fadsblib%2F)](https://telmomm.github.io/adsblib/)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.22059849.svg)](https://doi.org/10.5281/zenodo.22059849)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=telmomm_adsblib&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=telmomm_adsblib)

**adsblib** is a portable, deterministic C99 library for **encoding** ADS-B Out
DF17 (Extended Squitter) messages — aircraft identification, airborne
position (CPR), airborne velocity, and Mode-S CRC24.

It is designed as a reusable encoding core for experimental avionics
projects, SDR test-signal generation, and offline validation workflows,
in contexts where you need to *produce* correct, spec-compliant DF17 frames
rather than decode them.

## Statement of need

Most open-source ADS-B tooling in wide use — [dump1090](https://github.com/flightaware/dump1090),
[pyModeS](https://github.com/junzis/pyModeS), [readsb](https://github.com/wiedehopf/readsb) —
is built to **decode** ADS-B traffic received over RF. There is comparatively
little tooling dedicated to the inverse problem: generating correct,
standards-compliant DF17 frames from scratch, for use in synthetic traffic
generation, decoder testing, SDR-based signal injection, or coursework and
research on ADS-B message structure.

adsblib fills that gap with a small, dependency-free, no-allocation C
library that any project — embedded, desktop, or bound from another
language — can link against to produce correct DF17 frames without
depending on a full decoding stack.

## Comparison with related software

| Project | Role | Language | Notes |
| --- | --- | --- | --- |
| [dump1090](https://github.com/flightaware/dump1090) | RF receiver + decoder | C | Full receiver chain, decode-only |
| [readsb](https://github.com/wiedehopf/readsb) | RF receiver + decoder | C | dump1090 derivative, decode-only |
| [pyModeS](https://github.com/junzis/pyModeS) | Decoder + research toolkit | Python | Widely used as a reference decoder; no encoder |
| **adsblib** | **Encoder** | **C99** | No decoding, no RF, no allocation — just correct frame generation |

adsblib does not compete with these projects; it complements them. In fact,
adsblib's own validation workflow cross-checks its encoded output against
pyModeS's decoder (see [Validation](#validation)).

## Features

- Aircraft Identification encoding (Type Codes 1–4)
- Aircraft Status / Emergency and Priority Status encoding (Type Code 28)
- Target State and Status encoding (Type Code 29)
- Aircraft Operational Status encoding (Type Code 31)
- Airborne Position encoding with even/odd CPR
- Surface Position encoding (Type Code 8) with movement/ground-track fields
- Airborne Velocity encoding (Type Code 19), subsonic and supersonic
- Mode-S CRC24 calculation and verification
- Public C API with **no dynamic memory allocation**
- Validation workflow compatible with pyModeS

## Repository structure

| Path | Description |
| --- | --- |
| [`adsblib.h`](adsblib.h) | Public library API |
| [`adsblib.c`](adsblib.c) | Implementation |
| [`validation/test_encoder.c`](validation/test_encoder.c) | Independent C unit tests |
| [`validation/encoder_validation.ipynb`](validation/encoder_validation.ipynb) | Validation notebook (CRC, callsign, CPR, altitude, velocity, stress tests) |
| [`validation/validate_with_pymodes.py`](validation/validate_with_pymodes.py) | CI-ready pyModeS integration validation |
| [`validation/requirements.txt`](validation/requirements.txt) | Python dependencies for the validation notebook |
| [`Doxyfile`](Doxyfile) | Doxygen configuration used to build the API docs |
| [`developer-docs/`](developer-docs/) | Mission/scope, architecture, capability roadmap, and design decisions |

## Documentation

Full API reference: [https://telmomm.github.io/adsblib/](https://telmomm.github.io/adsblib/)

Docs are generated with Doxygen from the comments in `adsblib.h` and
published to GitHub Pages automatically by
[`.github/workflows/docs.yml`](.github/workflows/docs.yml) on every push to
`main`. The generated output is not committed to the repository.

To build the docs locally:

```bash
doxygen Doxyfile
open docs/html/index.html   # or xdg-open on Linux
```

## Requirements

- C99-compatible compiler (`cc` or `gcc`)
- `make` (optional)
- Doxygen (optional, to regenerate the API documentation)
- Python 3 + pyModeS (optional, for notebook validation)

## Building

adsblib is a single translation unit (`adsblib.c`) with one public header
(`adsblib.h`) and no external dependencies beyond the C standard library and
`libm`. There is currently no build system wrapper — compile it directly as
a static object, shared library, or drop the two files into your own project.

### Static object

```bash
cc -std=c99 -Wall -Wextra -Werror -c adsblib.c -o adsblib.o -lm
```

### Shared library — macOS (`.dylib`)

```bash
cc -std=c99 -Wall -Wextra -Werror -dynamiclib -o adsblib.dylib adsblib.c -lm
```

### Shared library — Linux (`.so`)

```bash
cc -std=c99 -Wall -Wextra -Werror -fPIC -shared -o adsblib.so adsblib.c -lm
```

Verify the build:

```bash
ls -lh adsblib.dylib adsblib.so 2>/dev/null
```

## API overview

All public functions return `enc_status_t` (`ENC_OK` on success) and write
into a caller-owned, fixed-size 14-byte frame buffer — adsblib never
allocates memory.

| Function | Purpose |
| --- | --- |
| `adsb_encode_identification` | Encode a DF17 Aircraft Identification message |
| `adsb_encode_position` | Encode a DF17 Airborne Position message (even/odd CPR) |
| `adsb_encode_surface_position` | Encode a DF17 Surface Position message (movement + ground track) |
| `adsb_encode_velocity` | Encode a DF17 Airborne Velocity message (subsonic or supersonic) |
| `adsb_encode_emergency` | Encode a DF17 Aircraft Status / Emergency and Priority Status message |
| `adsb_encode_target_state` | Encode a DF17 Target State and Status message |
| `adsb_encode_operational_status` | Encode a DF17 Aircraft Operational Status message |
| `adsb_crc24` | Compute Mode-S CRC24 over the first 88 bits of a frame |
| `adsb_apply_crc` | Insert CRC parity into a frame |
| `adsb_verify_crc` | Verify a frame's CRC |
| `adsb_cpr_encode_latitude` / `adsb_cpr_encode_longitude` | Low-level CPR encoding |
| `adsb_cpr_nl` | CPR NL(lat) longitude zone helper |
| `adsb_frame_to_hex` / `adsb_frame_clear` | Frame formatting/utility helpers |
| `adsb_version_string` | Returns the library version as `"MAJOR.MINOR.PATCH"` |

See [`adsblib.h`](adsblib.h) or the [generated docs](https://telmomm.github.io/adsblib/)
for the full signatures, and [Basic usage](#basic-usage) below for an
end-to-end example.

## Basic usage

```c
#include "adsblib.h"

int main(void)
{
    adsb_identification_t msg = {0};
    uint8_t frame[ADSB_FRAME_BYTES];

    msg.icao = 0xABCDEF;
    /* Callsign can be up to 8 characters and is space padded. */
    snprintf(msg.callsign, sizeof(msg.callsign), "ECABC");

    if (adsb_encode_identification(&msg, frame) != ENC_OK)
    {
        return 1;
    }

    adsb_apply_crc(frame);

    char hex[29];
    adsb_frame_to_hex(frame, hex);
    /* hex now holds the encoded, CRC-checked DF17 frame */

    return 0;
}
```

## Validation

adsblib does not ship a decoder, so correctness is validated by
cross-checking encoder output against an independent, widely used reference
decoder ([pyModeS](https://github.com/junzis/pyModeS)) rather than testing
the encoder against itself. The
[`validation/encoder_validation.ipynb`](validation/encoder_validation.ipynb)
notebook covers:

- CRC correctness
- Callsign round-tripping
- CPR (absolute error and compliance rate)
- Altitude
- Velocity (absolute and relative error), subsonic
- Stress tests
- Surface position (movement field + ground track)
- Velocity, supersonic subtype
- Aircraft status / emergency and priority status (Type Code 28)
- Target state and status (Type Code 29)
- Aircraft operational status (Type Code 31)

The independent C unit tests are the quickest local check and require no
Python or external packages:

```bash
cc -std=c99 -Wall -Wextra -Werror validation/test_encoder.c adsblib.c -lm -o /tmp/adsblib-test
/tmp/adsblib-test
```

They run in GitHub Actions on every push to `main` and every pull request,
on both Linux and macOS. They cover the public encoders, frame utilities,
CRC generation/verification, field quantization, and invalid-input status
codes.

The cross-decoder integration validation remains in the notebook:

```bash
pip install -r validation/requirements.txt
jupyter notebook validation/encoder_validation.ipynb
```

The CI entry point is the headless script
[`validation/validate_with_pymodes.py`](validation/validate_with_pymodes.py),
which uses only the minimal dependencies in
`validation/requirements-ci.lock`. The notebook remains as an interactive,
more extensive example of the same cross-decoder validation.

## Roadmap and design

adsblib's target capabilities go beyond frame encoding — see
[`developer-docs/`](developer-docs/) for the mission/scope,
[`developer-docs/ARCHITECTURE.md`](developer-docs/ARCHITECTURE.md), the
[capability roadmap](developer-docs/ROADMAP.md), and the
[architecture decision records](developer-docs/decisions/) behind
adsblib's design constraints.

## Versioning and changelog

adsblib follows [Semantic Versioning](https://semver.org/) and tracks all
notable changes in [`CHANGELOG.md`](CHANGELOG.md), per
[Keep a Changelog](https://keepachangelog.com/).

## Contributing

Contributions are welcome. See [`CONTRIBUTING.md`](CONTRIBUTING.md) for
build instructions, the validation workflow, and pull request guidelines.
Please also review the [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md).

## Security

See [`SECURITY.md`](SECURITY.md) for the security policy, intended-use
scope, and how to report vulnerabilities.

## Citation

If you use adsblib in academic or research work, please cite it using the
metadata in [`CITATION.cff`](CITATION.cff).

## License

This project is released under the MIT License. See [`LICENSE`](LICENSE)
for the full license text.
