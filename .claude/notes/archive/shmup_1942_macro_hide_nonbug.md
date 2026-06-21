---
name: shmup_1942 "macro hide breaks bullet firing" — DEBUNKED (2026-06-20)
description: A suspected cc65816 codegen footgun (commit 7113039) claiming that inlining enemy_hide/bullet_hide as preprocessor macros instead of static functions broke the bullet-firing path in examples/games/shmup_1942. Investigated with the regression_method.md bisection/repro workflow and proven NON-REPRODUCIBLE on the current toolchain. No code change needed; recorded so the open question from the 2026-06-20 engine review is closed and nobody re-chases it.
type: project
---

## Verdict: not a bug. The macro form fires bullets correctly.

Commit `7113039` ("fix(examples): shmup_1942 — restore bullet firing
(revert inline-macro hides)") restored `enemy_hide`/`bullet_hide` from
`#define` macros to `static` functions and stated, honestly, that it had
**no root cause** — "functions work, macros break the bullet firing path",
suspected as a cc65816 codegen quirk (bank placement / inlining /
side-effect fence around `oamMemory[]`). The 2026-06-20 engine review
flagged this as P1-1 (a revert-without-root-cause, violating
`.claude/rules/regression_method.md`).

## What the investigation found (three independent proofs)

1. **Semantic equivalence.** `OAM_WRITE(_id, ...)` parenthesises `_id`
   (`u16 _o = (u16)(_id) << 2;`), so `bullet_hide(i)` as a macro expands
   to exactly what the function body does. No operator-precedence or
   double-evaluation hazard (`i` and the OAM-base constants are
   side-effect-free).

2. **Byte-identical codegen on the firing path.** Built both variants
   (`make clean && make`), captured `main.c.asm`, normalised QBE's local
   label-number suffixes (`@name.NN` → `@name.N`, which shift only because
   the absent `*_hide` functions move the global label counter), and
   diffed `bullet_fire`: **182 lines, identical**. The macro/function
   choice changes only where `*_hide` is emitted (inlined at call sites
   vs one `.text.*_hide SUPERFREE` section), never `bullet_fire`.

3. **Runtime confirmation (Mesen2).** Built the macro variant, loaded it,
   held **A**, ran frames, read hardware OAM at the bullet slots
   (`BULLET_OAM_BASE=9` → OAM byte 36, 8 slots):
   - baseline: all 8 bullets hidden (`Y=0xEF=239`, i.e. `BULLET_HIDE_Y-1`)
   - after firing: slot 0 `X=0x78 Y=0x77(119)`, then `Y=0x63(99)` while
     slot 1 appears at `Y=0x83(131)` — bullets spawn and stream upward.
   Firing works in the *allegedly broken* macro ROM.

   Bank $00 also has **5138 bytes free** for this example, so the
   "macros inline more code → bank $00 spill → garbage" theory is
   impossible here (a handful of inlined `OAM_WRITE` sites are tens of
   bytes, not 5 KB).

## Most likely real story

The "intermediate broken state" the commit describes (text module
reverted, hides still macros) carried **another** difference that broke
firing — an incomplete text-module revert and/or the separately-tracked
bullet-despawn bug, which later commits fixed independently
(`< -BULLET_SPRITE` → `< -12` → today's `< 0`). The macro→function swap
rode along with the real fix and got the blame. Classic
symptom-vs-cause misattribution — exactly what `regression_method.md`
exists to prevent.

## State left behind

- No code change. `examples/games/shmup_1942/main.c` keeps the `static`
  function form (committed state) — it is correct and idiomatic; this
  note is *not* a call to switch back to macros, just a record that the
  macro form is not dangerous.
- The example README has **no** macro-vs-function footgun entry (the
  commit message claimed it added one to "the README's existing footgun
  list", but no such entry exists today — nothing to remove).
- The cc65816 conditional-JSL note in this folder
  (`cc65816_conditional_jsl_codegen_bug.md`) is a *different*, real (now
  silently fixed) codegen issue — don't conflate the two.

## If the symptom ever returns

Re-run the same three-step proof: (1) check `OAM_WRITE` macro hygiene,
(2) normalise-and-diff `bullet_fire` in `main.c.asm` between the two
forms, (3) read OAM at byte 36 in Mesen2 after holding A. If (2) ever
*differs*, that is a genuine codegen bug — bisect the compiler per
`regression_method.md` and reopen.
