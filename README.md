# adsblib

[![Documentation Status](https://telmomm.github.io/adsblib/)](https://telmomm.github.io/adsblib/)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.22059850.svg)](https://doi.org/10.5281/zenodo.22059850)

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
- Airborne Position encoding with even/odd CPR
- Airborne Velocity encoding (Type Code 19)
- Mode-S CRC24 calculation and verification
- Public C API with **no dynamic memory allocation**
- Validation workflow compatible with pyModeS

## Repository structure

| Path | Description |
| --- | --- |
| [`adsblib.h`](adsblib.h) | Public library API |
| [`adsblib.c`](adsblib.c) | Implementation |
| [`validation/encoder_validation.ipynb`](validation/encoder_validation.ipynb) | Validation notebook (CRC, callsign, CPR, altitude, velocity, stress tests) |
| [`validation/requirements.txt`](validation/requirements.txt) | Python dependencies for the validation notebook |
| [`Doxyfile`](Doxyfile) | Doxygen configuration used to build the API docs |

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
| `adsb_encode_velocity` | Encode a DF17 Airborne Velocity message |
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
- Velocity (absolute and relative error)
- Stress tests

```bash
pip install -r validation/requirements.txt
jupyter notebook validation/encoder_validation.ipynb
```

Unit tests with CI are planned — see the project roadmap for status.

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
