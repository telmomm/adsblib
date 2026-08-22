# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
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
