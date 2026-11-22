# L4 S2: the window and the NMI page sized at link time, still empty

Step S2 of `L4_plan_decoupe.md`, with the contest's point 5 (NMI page = the whole window, size 0 without
candidate; `irqSet(C)` and assembly naming a static refused at link) and point 8 as the conductor decided it after
S0 (no S2b in this lot: the deepest chain with parameters is 90 / 80 B, and the one function of game 1 that does not
fit stays on its stack). Branch `l4s23`, nothing pushed.

## Prediction (the plan's, written 2026-11-13 before any code of the lot)

"S2: the 65 ROMs byte-identical, `.sym` identical but the new zero-size section." The faulted fixture: "a
two-line fixture whose `lda.b sf.x+N` lands at `$100`: wlalink must refuse (proves the reading of `write.c`)";
"a fixture with an `APPENDTO` extension in another object: the `.sym` shows it contiguous (H3)".

## Result (2026-11-14)

**As predicted, with one correction and one defect found.** No section at all is added when no function has a
frame (`sframe.asm` holds two comment lines, `sframe.o` is empty): the 65 ROMs AND their `.sym` are byte-identical
to develop's, 130 files, `cmp`. wlalink refuses the operand at `$100` ("COMPUTE_PENDING_CALCULATIONS: Result
(256/$100) of a computation is out of 8-bit range"), and H3 holds: `.nmi_registers` 168 + 8 = 176 bytes in the
real `.sym`, the mirror at `tcc__nmi_registers + $A8`. `wlalink -d` keeps the window (RAMSECTIONs are not
discarded, measured with an unreferenced framed function).

The defect: the S1 check now runs in every build, and the first `make tests` of the step failed on the frozen
torture subset (`20011115-1`: `NOBUILD`). Its table said `foo.t`'s address is taken (the IR compares `foo == foo`
in a `static inline` nobody calls) and its text no longer did (the compare folded). That direction is no danger
(place over-approximates the indirect edges, the check reads the text's own), so it is no longer a refusal; the
other direction (the text takes an address the table omits) still is. Nothing else of the corpus, the fixtures or
the torture subset was refused.

### What was built (commit after this note's prediction, which is the plan's)

- `make/common.mk`: `opensnes-rom place -q --out sframe.asm linkfile` before wlalink, `sframe.asm` assembled by
  `wrap_asm` and linked from `linkfile.ld` (linkfile + `sframe.o` before `data_init_end.o`); `linkfile` itself
  stays the program, read again by `opensnes-rom check --objects linkfile` after the link. `make clean` removes the
  four new files; `.gitignore` takes `linkfile.ld`.
- `place` writes, after the window, `.RAMSECTION ".sframe_nmi" ... APPENDTO ".nmi_registers"` of `$A8 + deepest -
  $A8` bytes (the whole window, contest point 5), nothing when there is no frame.
- `check`: an NMI page that does not cover the window; `irqSet` given a literal C function; assembly that calls or
  names a static C function (`name.file`); a non-literal `irqSet` argument in a program with frames. Fault
  `OPENSNES_SFRAME_FAULT=nmipage` (the extension a word short).
- `devtools/sframe/test_sframe_link.py`: the real link fixture (below); the `noedge` fault is now refused by the
  faulted build's own check (it used to be linked, then checked by hand).
- `docs/tools/opensnes-rom.md`, `KNOWN_LIMITATIONS.md` (a C function given to `irqSet()` fails the link).

### The real link fixture (`test_sframe_link.py`, in `make tests`)

A hand-written C text whose table gives `fx.main` an 8-byte frame, linked into the corpus example `print_string`:
(a) the window `place` writes links, `sf.fx.main.f` at `$A8`, `.nmi_registers` 176 bytes, the mirror at the page +
`$A8`, the check accepts it from the real `.sym`; (b) a window at `$F8` of 8 bytes and `lda.b sf.fx.main.f+8`:
wlalink refuses; (c) a window of 16 bytes at `$F8` with operands below `$100`: links, the check refuses "ends past
$FF"; (d) `nmipage`: refused; (e) `irqSet(fx.main)`: refused; (f) an assembly routine `jsl fx.main`: refused.

### Gates (2026-11-14)

- `cmp` of the 65 `.sfc` and `.sym` against develop's: 130 files, 0 differ (twice: after the wiring, and with the S3
  compiler built and `SFRAME` unset).
- `make tests`: the first run failed on the torture subset only (the defect above); after the fix the second run:
  `ALL CHECKS PASSED (luna)` (difftest 16/16, difftest-stmt 28/28, coverage 63 OK / 2 INPUT-DEP of 65 at both
  power-ons, `sframe corpus: 65/65`, `sframe link fixture` green, `sframe oracle: 130/130`, `torture --quick: all
  26 as expected`). Honest note: that run's `bin/qbe` was already the S3 compiler (the qbe binary of `bin/` is a
  hard link to `compiler/qbe/qbe`, rebuilt while I checked S3's syntax), with `SFRAME` unset: its code paths are
  all behind `QBE_SFRAME=1`, and the corpus rebuilt with it is the same 130 files.
- `opensnes-rom golden: 47/47 ok` (the golden `.sym` now carries the NMI page extended to the window).
- `test_sframe_link.py --oracle`: 65/65, noedge refused by the build, link fixture green, oracle 130/130.

### What S3 inherits

The window's base stays `$A8` (88 bytes): S0 measured the deepest chain of temps alone at 98 B (game 1) and 90 B
(game 2) under the model S1 delivers (closure + reviewed routines), so `place` will refuse both games with
`SFRAME := 1` unless the frames past the window are kept on the stack: S3 adds the remedy `place` names
(`SFRAME_EXCLUDE`). The conductor's "no S2b" read the strict closure's 64 / 56 B.
