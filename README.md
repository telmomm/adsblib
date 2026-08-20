# ADSBLIB

adsblib is a portable, deterministic C99 library for encoding ADS-B Out DF17 (Extended Squitter) messages.

It is designed as a reusable encoding core for experimental avionics projects and offline validation workflows.

## Features

- Aircraft Identification encoding (Type Codes 1-4)
- Airborne Position encoding with even/odd CPR
- Airborne Velocity encoding (Type Code 19)
- Mode-S CRC24 calculation and verification
- Public C API with no dynamic memory allocation
- Validation workflow compatible with pyModeS

## Repository Structure

- `adsblib.h`: public library API
- `adsblib.c`: implementation
- `encoder_validation.ipynb`: validation notebook (CRC, callsign, CPR, altitude, velocity, stress tests)

## Documentation

[Doxygen API Documentation](https://telmomm.github.io/adsblib/) - Generated API documentation hosted on GitHub Pages

## Requirements

- C99-compatible compiler (`cc` or `gcc`)
- `make` (optional)
- Doxygen (optional, for API documentation)
- Python 3 + pyModeS (optional, for notebook validation)

## Build Shared Library

### macOS (.dylib)

```bash
cc -std=c99 -Wall -Wextra -Werror -dynamiclib -o adsblib.dylib adsblib.c -lm
```

### Linux (.so)

```bash
cc -std=c99 -Wall -Wextra -Werror -fPIC -shared -o adsblib.so adsblib.c -lm
```

### Quick Check

```bash
ls -lh adsblib.dylib adsblib.so
```

## Basic C Usage

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

	return 0;
}
```

## Validation

The `encoder_validation.ipynb` notebook includes tests for:

- CRC
- Callsign
- CPR (absolute error and compliance rate)
- Altitude
- Velocity (absolute and relative error)
- Stress tests

## Versioning and Changelog

This project tracks changes in `CHANGELOG.md`, following Keep a Changelog.

## Contributing

Contributions are welcome. See `CONTRIBUTING.md` for build instructions, validation
workflow, and pull request guidelines. Please also review the `CODE_OF_CONDUCT.md`.

## License

This project is released under the MIT License.

See `LICENSE` for the full license text.
