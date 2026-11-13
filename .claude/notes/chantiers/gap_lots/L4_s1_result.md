# L4 S1: the placer and the link check, dormant

Step S1 of `L4_plan_decoupe.md`, with the contest's points 2, 3, 4, 5 and 7 as far as they concern S1. Dormant: no
function gets a static frame yet (the compiler claims `sf=0` everywhere), so the ROMs must not move by one byte.

## Prediction (written 2026-11-14, before any code; committed alone)

Nothing below is measured. Each line says what would contradict it.

**What is built** (the order of the mission):
1. The compiler writes its side table always (comment lines, frames on or off), the library's C with it; the library's
   texts (`.c.asm`, and a copy of each assembly source) are installed beside its objects. `place` reads the table of the
   C and the text of the assembly; a routine is closed when its `jsl/jml/jsr/jmp` reach closed routines, with no indirect
   jump and no `pea` + return; the routines read by hand (memcpy, memmove, lzssDecodeRam, sprite_oamset's two computed
   returns, as in S0) are listed with their reason and a fingerprint of their text; an object without a text, or a C text
   without a table, gives the coarse edge; the runtime helpers are named one by one.
2. `place` writes `sframe.asm` (no section, no `sf.*` label today) and refuses to write it, removing any old one, on a
   re-entry or a window overflow.
3. `opensnes-rom check --objects linkfile` derives the edges, the address-taken functions and each frame's extent from
   the instruction lines of the linked objects' texts, compares them with the tables, requires the `sf.*` labels of the
   `.sym` to equal the tables, and refuses two live frames that overlap, a re-entry through a framed function, a frame past
   `$FF` or outside its window, a frame an NMI root reaches past the NMI page.
4. Faults: `QBE_SFRAME_FAULT=noedge`, a forced overlap, a window on `tcc__lf` and on `.snesmod_zp`, a cycle; each turns
   one named check red.

**Predictions**
- **ROMs**: the 65 corpus ROMs and their `.sym` byte-identical to the ones built from `develop` before the change
  (`cmp`, 65/65), and the library objects (`.o`, four flavours) identical. Contradicted by any difference.
- **The new checks on the corpus**: `place` writes an empty `sframe.asm` for all 65, the check passes on all 65 and
  refuses nothing. Table against text: **0 mismatches** in the 65 projects and in the four library flavours (S0 measured 0
  on LoROM and both games, with the helpers left out by prefix; here they are left out by name, so a helper outside the
  list would show).
- **The tool against the S0 script** (its `closure_rev` model, the one S1 delivers: closure + the reviewed routines), in a
  what-if mode where every compiler candidate gets its S0 frame: the same refused set on every example (0 everywhere, as
  S0 found) and the same deepest chain with parameters (at most 38 B, S0). Hypothesis on where they could differ: the tool
  reads the linked objects only, the script every library function and every SDK assembly file; a difference would come
  from an edge through a routine that is not linked (predicted none). Second hypothesis: the tool expands the assembly's
  macros (`DIV32_IN_CALLBACK`, `VCOL_ENTRY`...), the script reads their bodies as plain lines; predicted no effect on the
  verdicts.
- **Faults**: each refused by the check it targets, naming the function (and the path for a cycle), exit 1.
- **Gates**: `make test-compiler` 150/150 unless a check pins the absence of `; sframe` lines (none known); `make tests`
  all passed; `diff_corpus` 65/65 MATCH; `difftest.py` and `difftest_stmt.py` 2000/2000 on seeds 24001-26000 (the
  compiler's code does not change, only comments).
- **Measured before this note, while choosing the label** (contest point 11): WLA reads the `.b` of `sf.foo.b+2` as a
  size suffix (`Unresolved reference to "sf.foo"`); `sf.foo.b.f+2` assembles to `$AA` with `sf.foo.b.f` at `$A8`. So the
  frame label is `sf.<fn>.f`. The same suffix rule breaks today's code: a file `b.c` with a static variable `v` and a
  static function fails to link (`Reference to an unknown label "v"`, `lda.w v.b`), and would bind to a global `v` if one
  existed. Outside L4; reported, not fixed here.
