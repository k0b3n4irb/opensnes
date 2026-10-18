# OpenSNES Roadmap

Current state of the project and planned work. The single source of truth for
known traps and trade-offs is [`KNOWN_LIMITATIONS.md`](KNOWN_LIMITATIONS.md);
the design principles every roadmap item must respect live in
[`PHILOSOPHY.md`](PHILOSOPHY.md); what has shipped, release by release, is in
[`CHANGELOG.md`](CHANGELOG.md). This file tracks **where the project stands**
and **what is next**.

---

## Current Status: post-v0.51.0

Since 0.49 the SDK carries the public API meant for 1.0: the deprecated
aliases are gone and the two calls that change meaning have changed
(`docs/UPGRADING.md`). It is not 1.0: before 1.0 come the compiler and
library work still under way, a review of the examples with their assets
redone, and the first console session of the hardware protocol
(`docs/HARDWARE_VERIFICATION.md`, rows 1 to 7). `docs/STABILITY.md` is the
promise 1.0 will make.

A modern, well-tested SNES SDK ready for serious hobby development, game
jams, and educational use, building toward commercial-grade maturity. The
compiler produces code about 38 % faster than PVSnesLib + 816-opt estimated
on 34 functions, and 62 % faster measured on luna on 23 workloads, each of
them faster, none larger, and level or shallower in stack on 22
(`docs/BENCHMARK.md`). 68 working examples cover every major subsystem,
with cross-platform CI on Linux, macOS, and Windows enforcing not just "it
compiles" but the full functional test suite (luna, cycle-accurate native —
corpus liveness + visual regression + functional probes; SA-1/Super FX/DSP-1
run natively).

### Audience

OpenSNES is for C programmers. No SNES experience is needed to start: the
[learning path](docs/LEARNING_PATH.md) meets the hardware one example at a
time. The hardware traps (VRAM only during VBlank, the DMA budget of a
frame, the 8 KB RAM band, the argument order against PVSnesLib) are listed
in [`KNOWN_LIMITATIONS.md`](KNOWN_LIMITATIONS.md); read it before a project
of your own.

---

## The v1.0 gate

### Must-have

| Item | Status | Why it matters |
|------|--------|----------------|
| Pre-built binary releases | Done (`release.yml`) | Users do not compile the compiler |
| Hardware verification | Protocol written (`docs/HARDWARE_VERIFICATION.md`, `make hardware-kit`); first console session pending | Credibility: tested on a real SNES through an FXPak Pro |
| Showcase game (not a port) | In progress outside this repository | Proves the SDK can ship a complete game |
| Asset provenance | Every asset redone and the examples reviewed before a 1.0 release candidate. 66 asset files in 23 examples are PVSnesLib's, some of them Nintendo characters (Mario, Goomba, Koopa); they are listed in `ATTRIBUTION.md` until then | Legal clarity for a 1.0 that would ship Nintendo-derived art |
| Published performance benchmark | Done (`docs/BENCHMARK.md`, `docs/PERF.md`) | Where cc65816 and the library stand against PVSnesLib, with data |
| Migration guide from PVSnesLib | Done (`docs/MIGRATING_FROM_PVSNESLIB.md`) | The smoothest path for existing PVSnesLib users |

### Nice-to-have

| Item | Status | Why it matters |
|------|--------|----------------|
| Pixel mode (Mode 3 direct drawing) | Not started | Feature parity with PVSnesLib (niche) |
| Streaming audio | Not started | Long samples and voice without a full upload |
| Video tutorials | Not started | Wider audience reach |
| Save format in the library (checksum, version, slots), then `opensnes-save verify` | After 1.0 (1.1): a new public API is not frozen with 1.0 | A save that survives a corrupted byte and a new version of the game |
| String tables in `opensnes-text` (the strings of a game, by language) | After 1.0: no example needs them yet | Localisation without editing C sources |

---

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for guidelines, branch policy
(`main` = stable / `develop` = active), and PR rules. Build instructions
live in [`README.md`](README.md).

*Last updated: 2026-10-18. Anchored claims (version, examples count, framework
opt-in list) verified by `make lint-docs` — see `devtools/check_doc_drift.py`.*
