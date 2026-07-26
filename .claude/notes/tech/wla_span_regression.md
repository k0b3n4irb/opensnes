# wla-dx upstream regression — SPAN commit breaks SUPERFREE placement

Status: **root-caused, filed upstream** as
https://github.com/vhelin/wla-dx/issues/729 (2026-07-26). Our pin is the
v10.7 release (clean); do not advance past `a369bec5` until this is fixed.

## The finding

Syncing `compiler/wla-dx` from our pin `ffe59ca1` toward upstream master
`4f8bbdce` (93 commits) makes the SDK fail to link: examples using
several lib `SUPERFREE` sections abort with

```
lib/build/lorom/console.o: console.asm:954: MEM_INSERT: Overwrite at $XXXX (old $cd new $YY).
FIX_LABEL_ADDRESSES: Internal error: cannot map label "track_class_end" in section ".rodata3".
```

`$cd` is the ROM fill byte, so the linker is writing section data over
space it believes is free — a section-placement overlap.

**First bad commit: `4c3c042e`** — *"Added SPAN to .SECTIONs so a
.SECTION can be placed at the border of two ROM banks. Should fix GitHub
issue #663."* Its parent `a369bec5` is good. The commit rewrites
`wlalink/analyze.c` and `files.c` and adds a `span` field to
`struct section` — the section-placement machinery. `MEM_INSERT` and
`FIX_LABEL_ADDRESSES` are two symptoms of the same broken placement.

`games/mode7_racing` and `games/mode7_flying` are the examples that trip
it (large `SUPERFREE` payload — `track_map` + a 16 KB `track_class` — on
top of the lib's `SUPERFREE` sections).

## How it was bisected soundly (the method matters)

The first attempt at this was **unsound** and blamed a documentation
commit (`a369bec5`, which this sound run confirms is *good*). Three
confounds had to be removed:

1. **`make` resets the submodule.** `make all`/`make lib` depend on a
   `submodules` target that runs `git submodule update`, snapping wla-dx
   back to the committed gitlink. Every "build at commit X" that went
   through top-level `make` silently tested the *pinned* commit instead.
   → Build via `make -C lib` and `make -C examples/...` (direct
   sub-makes), which never run that target.
2. **Object-format drift.** The `.o` format changed within the range, so
   mixing a new linker with old objects gives `unknown format ("WLAl")`,
   not the real error. → Delete all `lib/build` and example `.o` each
   step so they reassemble with the commit's own `wla-65816`.
3. **A test fixture blocks checkout.** `.gitattributes` `text=auto`
   perpetually "modifies" `tests/68000/all_instructions_test/main.s`, so
   `git bisect`'s plain checkout aborts and results are garbage. → Manual
   binary search with `git checkout -f`.

The sound classifier (`/tmp` during the session, re-derive if needed):
per commit — build wla, install, wipe objects, `make -C lib`, `make -C
examples/games/mode7_racing`, then classify **0** = ROM built, **1** =
`MEM_INSERT`/`FIX_LABEL_ADDRESSES`, **125** = anything else. Validated to
return 1 on the bad endpoint and 0 on the good endpoint *before* running
the search. Converged: idx 89 `a369bec5` GOOD, idx 90 `4c3c042e` BAD.

## Faithful reproduction

```
cd compiler/wla-dx && git checkout 4c3c042e   # or any commit >= it
# build wla, install to bin/, wipe lib+example .o, rebuild
```
`games/mode7_racing` then fails to link with `MEM_INSERT`. Reverting to
`4c3c042e~1` (`a369bec5`) links cleanly.

## Minimal reproducer (found)

The trigger is exact: **a `SUPERFREE` section whose size makes it end
precisely on a bank boundary.** Not too small, not spilling over —
*exactly* filling a bank, so its end label sits at the first address of
the next bank. That is literally what the SPAN commit set out to handle
("place a .SECTION at the border of two ROM banks"), and it mis-maps the
end label there.

`wla_span_minimal_repro.asm` — one 32 KB section in a 2-bank ROM:

```
.ROMBANKMAP
BANKSTOTAL 2
BANKSIZE $8000
BANKS 2
.ENDRO
.SECTION "big" SUPERFREE
big: .incbin "fill32k.bin"   ; exactly 32768 bytes
big_end:
.ENDS
```

`wlalink` on `a369bec5` builds it; on `4c3c042e` it aborts:
`FIX_LABEL_ADDRESSES: Internal error: cannot map label "big_end"`.

Controls pin the boundary as the trigger (same ROM, only the blob size
changes):

| section size | a369bec5 (good) | 4c3c042e (bad) |
|---|---|---|
| 32768 − 16 (doesn't fill the bank) | OK | OK |
| **32768 (ends exactly on the boundary)** | **OK** | **FAIL** |
| 32768 + 1 (genuinely spans into bank 1) | OK | OK |

So it is neither "too small" nor "over capacity" — the minimal ROM has a
whole empty bank. It is the exact-boundary end label alone.
- **Filing.** Not done — the maintainer decides. When filed, it is a
  genuinely good report (exact first-bad commit, clear symptom,
  reproducible), unlike the retracted `BANKS`/`.DEFINE` one.

## Cross-refs

- `.claude/notes/tech/wla_banks_define_upstream_issue.md` — the *other*
  wla observation from this session, which was **wrong** and retracted.
  This one is bisected and real; don't conflate them.
- Our pin: `compiler/PINS.md`, `ffe59ca1`. Do not advance past
  `a369bec5` until this is fixed upstream or worked around.
