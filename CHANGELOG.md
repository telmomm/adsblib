# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- `developer-docs/`: mission/vision/scope, architecture (current and
  target module layout), a capability roadmap (`ROADMAP.md`) covering
  full DF17 message-type coverage, RF/IQ signal synthesis, and scenario
  generation, and architecture decision records (ADRs) for the project's
  core design constraints.
- `adsb_encode_surface_position()`: DF17 Surface Position (Type Code 8)
  encoding, with non-linear movement-field (ground speed) and ground
  track encoding, per ICAO Annex 10 / DO-260B. Cross-checked against
  pyModeS in `validation/encoder_validation.ipynb` (TEST 7).
- Supersonic ground speed support in `adsb_encode_velocity()`: subtype 2
  (4 kt/LSB, up to ±4088 kt per velocity component) is now selected
  automatically whenever a component exceeds subtype 1's ±1022 kt range.
  Cross-checked against pyModeS in `validation/encoder_validation.ipynb`
  (TEST 8).

### Changed
- Moved `adsblib_roadmap_JOSS.md` to `developer-docs/JOSS_ROADMAP.md` to
  live alongside the rest of the developer documentation.

## [0.1.0] - 2026-08-22

### Added
- Initial C99 implementation of adsblib (`adsblib.c`, `adsblib.h`).
- Encoding functions for ADS-B DF17:
  - Aircraft Identification
  - Airborne Position (CPR even/odd)
  - Airborne Velocity
- CRC24 Mode-S calculation and verification helpers.
- CPR helper functions and frame utility helpers.
- `ADSBLIB_VERSION_MAJOR`/`MINOR`/`PATCH` macros and `adsb_version_string()`
  to query the library version.
- Initial validation tests (CRC, callsign, CPR, altitude, velocity, stress).
- Project baseline documentation and licensing.
- `CONTRIBUTING.md` with build, validation, and pull request guidelines.
- `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1).
- GitHub issue templates for bug reports and feature requests.
- `SECURITY.md` describing the intended-use scope and vulnerability reporting process.
- `CITATION.cff` for academic citation.
- `.github/workflows/docs.yml`: Doxygen API docs are now built and deployed to
  GitHub Pages automatically on every push to `main`.
- README rewritten with a Statement of Need and a comparison against
  dump1090, readsb, and pyModeS, per the JOSS-oriented roadmap.

### Changed
- Generated Doxygen output (`docs/`) is no longer committed to the
  repository; it is now build-only output produced by CI and published to
  GitHub Pages. `docs/` is now git-ignored.
