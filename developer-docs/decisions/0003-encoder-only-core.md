# 0003. Encoder-only scope for the core library

Status: Accepted

## Context

The existing open-source ADS-B ecosystem — dump1090, readsb, pyModeS — is
built around receiving and *decoding* real ADS-B traffic, and is mature,
widely used, and well-trusted for that purpose. adsblib's Statement of
Need (see root `README.md`) is explicitly the inverse problem: generating
correct, spec-compliant traffic. Re-implementing decoding would duplicate
well-covered ground instead of filling the actual gap, and would
significantly expand the core's scope and maintenance surface.

`M2` in `ROADMAP.md` (RF/IQ signal synthesis) raises a related question:
validating modulated signal output plausibly requires demodulating it
back to confirm it decodes correctly.

## Decision

The core library (and any future module) does not expose a
general-purpose ADS-B decoder as public API. Where self-verification of
adsblib's own encoded/modulated output requires decoding capability
internally (e.g. to validate `adsblib_signal`'s modulation), that
capability is scoped strictly to verifying adsblib's own output, is not
exposed as a public decoding API, and does not aim for the completeness
or robustness of a real decoder (that's what pyModeS is for in the
validation workflow).

Continue validating correctness by cross-checking against pyModeS
(an independent, external, already-trusted decoder) rather than adsblib
re-implementing decode logic and testing itself against itself.

## Consequences

- Keeps adsblib complementary to, not competing with, the existing
  ecosystem — reinforces the differentiation argument in the README's
  comparison table.
- Avoids the correctness risk of a self-referential test (a shared bug in
  both the encoder and an internal decoder could hide behind a passing
  test) for anything the pyModeS cross-check already covers.
- If a genuine need for real decoding ever emerges, that's a scope change
  significant enough to warrant its own ADR (and likely its own
  repository/package) rather than folding it in silently.
