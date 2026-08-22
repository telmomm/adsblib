# Security Policy

## Scope and intended use

adsblib is an experimental, non-certified C99 library for encoding ADS-B Out
DF17 (Extended Squitter) messages. It is intended for offline validation,
research, and experimental avionics work — **not** for use in certified
avionics, safety-of-life, or any production surveillance system. It has not
been evaluated against DO-178C or any comparable safety standard.

If you embed adsblib in a system where incorrect encoding could have safety,
legal, or regulatory consequences (e.g. transmitting real RF), that system's
maintainers are responsible for independent verification of the encoded
output before use.

## Supported versions

adsblib is pre-1.0 and does not yet maintain parallel supported branches.
Security fixes are made against the latest `main` and released in the next
tagged version.

| Version   | Supported          |
| --------- | ------------------- |
| `main`    | :white_check_mark:  |
| < 0.1.0   | :x:                  |

## Reporting a vulnerability

Please **do not** open a public GitHub issue for security-relevant bugs
(e.g. buffer overflows, out-of-bounds reads/writes, integer overflows in
encoding paths, or any input that can corrupt memory or produce a frame that
silently violates the DF17 spec in a way that could mislead a downstream
decoder).

Instead, report privately by emailing **telmom95@gmail.com** with:

- A description of the issue and its potential impact.
- Steps to reproduce, including the exact input values used.
- The adsblib version/commit, compiler, and OS.

You should expect an initial response within 7 days. Once a fix is
available, it will be released and credited in `CHANGELOG.md` (unless you
prefer to remain anonymous).

## Design notes relevant to security review

- adsblib performs **no dynamic memory allocation** — all buffers are
  fixed-size and caller-owned.
- All public encoding functions validate their inputs and return a
  status code (`enc_status_t`) rather than trusting caller-supplied
  values blindly.
- The library has no network, file, or OS dependencies beyond the C
  standard library and `libm`.
