---
name: Mesen2 validation cadence + diagnose-before-patch
description: Ask user to spot-check in Mesen2 between non-trivial changes; do not use --quick automated count as the sole proxy for visual correctness. Diagnose mechanism BEFORE applying any fix, even small ones.
type: feedback
---

When making non-trivial compiler / lib ASM / runtime changes, two
discipline failures keep happening:

1. **`--quick` count taken as proxy for full validation.** The
   automated snes9x-based suite catches large visual regressions,
   but misses subtle ones (mode-7 affine drift, palette transition
   timing, frame-perfect sprite positioning, lag-frame edge cases).
   `.claude/rules/testing.md` is explicit that the 3-pillar workflow
   (auto + Mesen2 manual + full rebuild) is mandatory, not
   discretionary. Mesen2 is what surfaced the A6+A7 acache bug in
   30 min after three prior sessions had stalled on the same
   46-failure mystery.

2. **Patch-before-diagnose** on speculative hypotheses. Example
   from chantier A6+A7 (2026-05-14): the replay README pointed to
   "lib ASM dmaCopyVram bank-byte read" as a working hypothesis;
   I rewrote `lib/source/dma.asm` to read bank from `11,s` BEFORE
   confirming the hypothesis via Mesen2 step-through. The test
   suite moved +1 example (marginal). I reverted. Lost ~20 min on
   a dead branch. The actual bug was in QBE codegen
   (`emit_store_high` stale acache), found by reading the
   generated asm of the failing loop AFTER Mesen2 located the
   freeze site.

**Why:** Maintainer-grade chantiers (A6, future A6.12, B-series)
involve many small mechanical edits each of which could regress a
subtle visual. The auto suite is the gross filter; Mesen2 is the
fine filter. Skipping Mesen2 means subtle regressions ship and the
chantier gets reverted weeks later. Patching before diagnosing
means time wasted on wrong branches.

**How to apply:**

1. **Before any compiler / lib ASM / runtime fix**, state the
   mechanism hypothesis explicitly. Test the hypothesis with a
   read-only investigation (asm dump, IR trace, Mesen2 watch) —
   not a patch.
2. **Patch only after mechanism is proven.** "I think it's X, let
   me try" is the smell. "X causes Y because Z; I will fix by W"
   is the standard.
3. **For chantiers spanning many edits** (e.g. A6.12 lib ASM
   rework across ~8 functions), ask the user to Mesen2-spot-check
   ONE failing example per edit, not every N edits. The cadence
   is: fix one site → build → quick suite → ask user "load X in
   Mesen2, confirm the visual regresses as expected vs the
   no-fix state" → next site.
4. **End-of-chantier**: full Mesen2 pass on the triaged
   representative-list (per `.claude/rules/testing.md` triage
   workflow), not just `--quick`.
5. Resist the urge to make multiple "small" fixes in one commit
   when you haven't validated each in isolation. One fix per
   commit is fine when the fixes are independent; if they
   interact, validate the COMBINATION on Mesen2 before committing.
