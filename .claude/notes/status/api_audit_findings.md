# API audit — findings collected on the way (running list)

> Superseded where they overlap by the full review,
> `.claude/notes/reviews/2026-09-20_api_audit.md` (2026-09-20).

Started 2026-09-20 while covering the never-executed public functions (road
to v1.0, step 3). Each entry is something the coverage work surfaced that
belongs to the API audit before the freeze (step 4), or to the luna team.
Fixed items are not listed here — they are in the commit messages and the
tutorials' Gotchas.

## For the API audit

- ~~`nmiSet(callback)` dispatches in bank `$00`~~ — **wrong, corrected
  2026-09-20**: `nmiSet` derives the bank from the far pointer
  (`console.c:268-274`); only the header's "must be in bank 0" note is stale.
  The function that drops the bank is `irqSet` (`irqSetBank(handler, 0)`).
  See `.claude/notes/reviews/2026-09-20_api_audit.md` §2 B2 and §6.
- **`objCollidObj(idx1, idx2)` takes slot indices** where `objKill` and
  `objGetPointer` take handles. Documented now; an API that masks its
  arguments (`and #$00ff`) would accept both and remove the trap.
- **The hdma module costs 1346 + 113 bytes of bank-`$00` RAM** for the wave
  and brightness tables (`.hdma_wave_tables`, `.hdma_brightness`), linked or
  not used. HDMA reads its table from any bank; the tables could live in
  `$7E:2000+` with `FAR` access from the C fill routines.
- **`mouseSetSensitivity` is silent without a mouse**: the request is
  recorded and never applied, the getter keeps returning 0. By design (the
  NMI only talks to a device that answered), but worth a line in the header.
- **`consoleInitEx(options)` ignores its argument** ("reserved").
- **`oamDrawMeta` / `oamDrawMetaFlip` return the next free sprite id**, which
  is what the metasprite example chains on; the Flip header said "number of
  sprites used" (fixed 2026-09-20). Worth one name for the concept in both.
- **`audioUpdate()` is a documented no-op** kept for source compatibility —
  a candidate for removal at the freeze.
- **`snesmodGetPosition` and `objCollidObj` both stored their result in
  `tcc__r0` instead of returning it in A.** objCollidObj returned garbage
  (fixed); snesmodGetPosition happens to leave the value in A's low byte.
  Worth a sweep of every hand-written ASM function with a return value.

## For the build guards

- **`devtools/check_lib_rodata.py` looks stale since #121 / #127.3.** It
  forbids `static const` tables in lib C modules because "the compiler's
  16-bit C deref reads garbage" when the section lands in bank `$01+`. Every
  C read of const data has been a far read since #121, and const data goes
  to the asset banks by design since v0.41.0. The lint now only forces lookup
  tables into bank-`$00` RAM: `mode7.c`'s 256-byte sine table, `hdma.c`'s
  tables. Tried 2026-09-20: making the Mode 7 table `const` rendered all six
  Mode 7 examples identically (diff_corpus 85/85) and gave 256 bytes back —
  reverted because retiring a gate is the owner's call, not a side effect of
  a test lot. Decision needed: retire the lint (and reclaim the RAM across
  the lib) or restate its reason.

- **The C RAM budget does not know the stack.** The libtest fixture passed
  `--check-ram-budget` with 942 bytes free while its stack reached 989 bytes
  below `$2000` during the audio driver's boot and overwrote result globals.
  `RAM_FAIL_THRESHOLD` (512) is a guess at stack depth; a measured figure
  would be better (see the luna request below).

## For the luna team (owner validates before filing — luna_tooling.md)

- `luna profile` has `--input` only: no `--mouse`, `--superscope`,
  `--input2`. The ROM-coverage ratchet therefore cannot replay the mouse and
  Super Scope manifests; ten `input.h` functions stay "never executed" by
  any example leg (the fixture executes them with no device).
- `--dsp1-rom ""` (an empty path) **installs an empty file over the existing
  firmware** in `~/.config/luna/firmware/` and reports success. Happened on
  2026-09-19 (restored from another copy, MD5-verified). A size / hash check
  before install, or refusing an empty path, would prevent it.
- A **minimum-SP report** (deepest stack reach over a run) in `luna state`
  JSON would turn the stack-vs-globals collision above into a gate; today it
  was found by `--trace-writes` on a corrupted variable, then measured from
  stack residue in zero-initialised RAM.
