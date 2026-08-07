# Luna-First: internal scripts are transitory (Auto-loaded)

CRITICAL, load-bearing convention (owner decision, 2026-08-05):

**Everything goes through luna. luna is the single validation/emulation backend
— the one source of truth for running, inspecting, and judging ROMs.**

Internal scripts (Python or any language) that add a *capability luna does not
yet expose* are **TRANSITORY PROTOTYPES**, never permanent tools. The ideal
steady state is **zero permanent internal validation code**: every need is
covered by luna itself. A prototype exists only to discover and *prove the exact
capability* luna should own, so the eventual luna feature is specified by a
working reference, not a hunch.

## The mandatory lifecycle for a missing capability

1. **PROTOTYPE** — write the smallest internal script that proves the capability
   and pins its exact shape (what input, what output, what assertion). Use it for
   real work in the meantime. Keep it under `tools/luna-test/`.
2. **VALIDATE with the owner** — show that the prototype does what's needed; get
   explicit sign-off. Do **not** file a luna issue on a guess — the owner
   validates internally what's missing first.
3. **FILE the luna issue** — only when sure. Describe precisely what luna should
   expose; the prototype **is** the spec (paste its I/O contract). This is the
   "challenge luna" step.
4. **luna SHIPS it** (exactly as agreed) → **validate together** that the shipped
   capability reproduces the prototype's output.
5. **DELETE the prototype** — rewire every caller to luna and remove the internal
   code. The capability now lives in luna. Leaving the script behind creates a
   second, drifting source of truth — the exact thing this rule forbids.

## Transitory vs. not

- **Transitory (must eventually disappear):** any script that *reimplements a
  capability luna should own* — decoding, analysis, capture, measurement.
  Example: audio FFT/pitch/envelope analysis; a bespoke ROM disassembly; a
  hand-rolled state differ.
- **Not transitory (the thin orchestration layer):** the harness that merely
  *drives* luna and asserts on its outputs — `luna_runner.py`, `wram_regress.py`,
  `budget.py`, `probes/*`. These call luna; they do not reimplement it. They are
  the pragmatic exception, and still shrink as luna exposes more. When in doubt:
  *am I asking luna and checking its answer (keep), or computing the answer luna
  should give (transitory)?*

## Current transitory prototypes (keep this list live)

**None right now** — the ideal steady state. When a missing capability forces a
prototype, add a row here (script path · the luna capability it proves · status)
and follow the lifecycle above; when luna ships the capability and it is
validated, delete the script and remove the row.

A prior audio-output-analysis prototype (`audio_analyze.py`) was dropped rather
than promoted: it proved unreliable for melody/tempo verification, so it is not
a sound basis for a luna capability request. Any future audio-analysis need
starts a fresh prototype held to the validation bar above before any luna issue.

## Why this exists

A permanent internal tool is a second source of truth: it drifts from luna, is
invisible to contributors who only have luna, and duplicates work luna should
own. Prototypes are cheap and *hyper-useful for the everyday* — but they are
scaffolding. The finished building is luna.

## Cross-references

- `.claude/rules/testing.md` — luna is already the test backend; this rule adds
  the transitory-tooling discipline on top.
- The owner runs luna engineering: validated capability requests become luna
  issues (`gh issue create --repo k0b3n4irb/luna`), tracked to deletion here.
