# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## [0.1.0] - 2026-06-12

### Added
- Initial repository documentation for adsblib.
- Initial C99 implementation of adsblib (`adsblib.c`, `adsblib.h`).
- Encoding functions for ADS-B DF17:
  - Aircraft Identification
  - Airborne Position (CPR even/odd)
  - Airborne Velocity
- CRC24 Mode-S calculation and verification helpers.
- CPR helper functions and frame utility helpers.
- Initial validation tests (CRC, callsign, CPR, altitude, velocity, stress).
- Project baseline documentation and licensing.
