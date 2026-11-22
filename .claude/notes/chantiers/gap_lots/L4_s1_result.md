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

## Result (2026-11-14, measured after the prediction above was committed, `68236c54`)

**Verdict: as predicted, with two corrections.** The ROMs do not move (130/130 `.sfc` and `.sym` identical to `develop`'s,
on the clean build); the new checks pass on the 65 examples and refuse nothing; the tool and the S0 script give the same
answer on every example. Wrong in the prediction: the library's `.o` are **not** byte-identical (WLA's line records move
with the comment lines; the ROMs, which are what counts, do not), and the reading found a real defect in an existing gate
(below), fixed in its own commit.

### What was built (branch `l4s1`, nothing pushed)

| commit | what |
|---|---|
| `68236c54` | this note's prediction, alone |
| `0635b089` + qbe `1aeda99` | the side table written for every function, frames on or off (`sf=0`, `; sframe table 1`); `QBE_SFRAME_FAULT=noedge` |
| `8d12d8bd` | fix: the NMI / WRAM-port lint read `fill.main` as `fill` (below) |
| `bf3fc91a` | the library's assembly objects keep `X-asm.text.asm`, `crt0.o` keeps `crt0.text.asm` |
| `f938c32e` | `opensnes-rom place` and `check --objects` (`tools/opensnes-rom/src/sframe.c`), 24 golden cases, `devtools/sframe/test_sframe_link.py` in `make tests`, the release ships the library's texts |

- **The model of the reader** (S0's, in C): the linked objects come from the linkfile (`[objects]`, paths relative to it);
  each object's text is `X.wrap.asm`, `X.c.asm` or `X.text.asm` beside it. C texts give functions from `; Function:` and
  the label, the table from `; sframe`, the instruction lines for the check. Assembly routine by routine: a global label
  to the next (`_local` labels named `<object>:_x`), falling into the next label is an edge (also from a label with no
  instruction and no data: two names of one routine, which S0 missed), `.SECTION` / `.ENDS` end a routine, `.DEFINE A B`
  with B a plain symbol is an alias (`__mul16` -> `tcc_mul16`), macros of the same text expanded with their arguments, an
  unknown mnemonic that is not a macro of the text opens the routine (the three section macros of `assets.inc` excepted).
  Open: a jump through a pointer (but `(@local,x)`), a target that is an expression, a `pea` / `per` not pulled back at an
  `rtl` / `rts`. UNKNOWN reaches every exported C function, **every global assembly routine** (S0 had C only: a text-less
  object may call an assembly routine that names a static C function) and every address-taken function.
- **REVIEWED**: memcpy, memmove (re-read today: the MVN / MVP stub on the stack returns to `@back`), lzssDecodeRam,
  sprite_oamset's `_ohr_big` / `_ohr_lo1` (unchanged since S0's reading, `git log`), keyed by object and by a fingerprint
  of their instruction lines (FNV-1a 64, `unsigned long long` so that Windows' 32-bit `long` gives the same value): a line
  changed and the routine is open again (golden case). `OPENSNES_SFRAME_FINGERPRINTS=1` prints them.
- **HELPERS**: `__mul16 __div16 __mod16 __sdiv16 __smod16 __mul32 tcc_mul32 tcc_udivmod32 tcc_sdivmod32` (every `jsl __` /
  `jsl tcc_` of the backend): the only calls a C text may make without a table line; place trusts them closed, the check
  follows them through the runtime's text. `__evil` in a text: refused (golden).
- **The frame label is `sf.<fn>.f`** (point 11, measured). The window `sframe.asm` is one `RAMSECTION ".sframe" BANK 0
  SLOT 1 ORGA $A8 FORCE` whose labels are laid by offset with `DSB` of the gap to the next (overlapping frames share an
  address): assembled and linked by hand with wla on the golden diamond, the `.sym` gives every label at its offset and
  `lda.b sf.d.bb.f+2` assembles to `$B4` (`build/` scratch, not a test: S2's link fixture is the test).
- **Not wired** into `make/common.mk` (S2). Cost measured: `check --objects` on the 65 examples, 0.42 s in all (6 ms a
  link).

### The tool against the oracle

`place --what-if` gives every compiler candidate of the project (cand=yes, not on a cycle of its file) its S0 frame (stack
temps + the larger of its parameters and the largest argument block a caller sends) and reports the refusals and the
deepest chain. Against `sframe_count.py --corpus examples --lib-casm lib/build/lorom`, per example: same candidates, same
refused names, same deepest chain with parameters.

| model | agree |
|---|--:|
| `closure_rev` (the tool as delivered) | **65 / 65** |
| `closure` (tool with `OPENSNES_SFRAME_FAULT=noreview`) | **65 / 65** |

The first run disagreed on 9 examples, from four causes, all found by this comparison before any
commit: a mnemonic buffer of 16 bytes cut `DMA_SCREEN_CHECK` (the macro, unknown, opened `dmaCopyVram` & co.: 6 examples
refused), and `.DEFINE MS_ID tcc__lf+0` (sprite_meta) taken for a routine alias (an indirect call reached it: shard_dash), two defects of the reader; then `@nox\@:`
(a macro's unique child label, dynamic_sprite) and the five fingerprints not yet written (lzss_ram). No golden case reads the
library's real assembly (they are synthetic): the oracle on the corpus is the test of the reader.

### Faults (each turns one named check red; golden unless said)

| fault | refused by |
|---|---|
| `QBE_SFRAME_FAULT=noedge` on `text/print_string` rebuilt (ROM identical to the corpus one) | `table/text: ... calls ... and its table does not say so` (`test_sframe_link.py`; on `games/blocks` by hand: 19 findings) |
| a call the table omits, by hand | `table/text` |
| a text calling `__evil` (not a named helper) | `table/text` |
| re-entry through C of another file (`f.a -> x -> e1 -> f.a`), through assembly naming `e0`, through a function pointer, through assembly jumping through a pointer, through an object without a text | `place`: `reentry:` with the path and why the node is open; nothing written, an old `sframe.asm` removed |
| overlap forced (`OPENSNES_SFRAME_FAULT=overlap`, every frame at +0) | `overlap: g.a ... can run while f.a ... is active` |
| the window on `tcc__lf` (`$80`) | `window: .sframe ... overlaps .leaf_frame`, `lies on .leaf_frame` |
| the window on `.snesmod_zp` (`$40`) | `window: ... overlaps .snesmod_zp` |
| a frame past `$FF` (window at `$F2`) | `window: the frame of d.bb ... ends past $FF` |
| a cycle written anyway (`OPENSNES_SFRAME_FAULT=reentry`) | `reentry: f.a has a static frame and can be active twice: f.a -> x -> e1 -> f.a` |
| a frame an `nmiSet` literal callback reaches, at `$A8` | `nmi: f.a can run in the NMI ... past the NMI page (168 bytes)` |
| a table frame without its label / a label without its table frame | `sym:` |
| `lda.b sf.f.a.f+6` with `sf=4` / `lda.w sf.f.a.f+2` / assembly naming `sf.f.a.f` | `frame:` |
| memcpy changed by one line | `place`: `memcpy [... computed return]` |
| and the positive controls: the chain + diamond placed (f 0, g 4, h 6; dia 0, b 2, c 2, d 10) and accepted by the check from its `.sym`; the named helpers accepted; an NMI-reachable frame inside the NMI page not refused for the NMI | |

### Found on the way

- **The NMI / WRAM-port lint was blind to static functions** (`asm.c`, `take_ident` in three readers): a `$2180` write in
  `fill.main`, called from an NMI callback, passed when the function above it in the file was not reachable. Fixed
  (`take_sym`), golden case `nmirace_static` (exit 0 before, refused after); the corpus passes unchanged. Lesson in the kit.
- **A file `b.c` / `w.c` / `l.c` cannot hold a static** (WLA's size suffix): link error, or a silent binding to a global of
  the same name. Not fixed (outside L4, touches cc65816's naming of statics); for the conductor.

### Gates
Re-run 2026-11-14 on the branch's last commit `f938c32e`, machine idle (load under 2), each alone:

- `make clean && make`: exit 0, the same warning lines as `develop`'s first build in this worktree (clang `-rdynamic`,
  CMake deprecation, make's jobserver x3). `cmp` of the 65 `.sfc` and 65 `.sym` against the ones built from `develop`
  before any change: **130 files, 0 differ**. 137 library assembly objects, 137 `.text.asm` beside them.
- `make test-compiler`: `Compiler checks: 150 passed, 0 failed (+0 compile-only; 0/0 unchecked ratchet; 31/31 refusals pinned)`.
- `make tests`: `ALL CHECKS PASSED (luna)` (coverage 63 OK / 2 INPUT-DEP of 65 at both power-ons; torture `PASS 12, FAIL
  2, REFUSED 12`, the expected file; in it `sframe corpus: 65/65 examples: empty window, check passes`, `sframe fault
  QBE_SFRAME_FAULT=noedge on text/print_string: refused by the check, same ROM`, `sframe oracle: 130/130 (example,
  model) pairs agree with sframe_count.py`).
- `python3 testing/diff_corpus.py --ref <develop's ROMs>`: `DIFF CORPUS: 65/65 match, 0 diff, 0 missing (tolerance ±0)`.
- `python3 testing/difftest.py --seeds 24001-26000`: `difftest: 2000/2000 seeds clean, 48000 expressions in 4 shapes`.
- `python3 testing/difftest_stmt.py --seeds 24001-26000`: `difftest-stmt: 2000/2000 seeds clean, 20000 programs`.
- `python3 tools/opensnes-rom/tests/run_golden.py`: `opensnes-rom golden: 47/47 ok` (24 static-frame cases, 1 NMI-lint
  case); the same and `test_sframe_link.py --oracle` with `opensnes-rom` built `SANITIZE=1` (ASan + UBSan,
  `halt_on_error=1`): 47/47, 65/65, refused, 130/130. `make test-sanitizers` itself was not run (it rebuilds the whole
  toolchain; only this tool's host code changed besides comments in QBE).
- `make lint`: exit 0 (`lint-docs` no drift, `lint-cppcheck` OK, `OK: all 5 commits pass lint`).
- `devtools/verify_toolchain.py`: `OK: toolchain pins match (3 submodule(s) verified)`.
- Kit: `make lecons`: `lecons : 143 fiche(s), index à jour` (two lessons, kit commit `949d1ac`).

### What S1 did not do, and what S2 inherits

1. **Wire it**: `make/common.mk` runs `opensnes-rom place linkfile` before `wlalink`, assembles `sframe.asm` with
   `wrap_asm` and links it, and passes `--objects linkfile` to the post-link check (6 ms a link). With no frame the window
   is a comment file and nothing links from it.
2. **The NMI page** (contest point 5): today the check refuses any NMI-reachable frame ending past `.nmi_registers` (168
   bytes), so every frame in the `$A8` window that an NMI root reaches is refused. S2 extends the NMI page to the whole
   window (`APPENDTO`, H3 to prove with a two-object fixture) and the check's limit follows the `.sym`.
3. **S2b** is mandatory before S5 by point 8 (game 1's 90 B chain with parameters against today's 88 B hole); its gate
   compares by symbol.
4. **Not yet refused**: `irqSet` given a C function, and assembly naming a decorated static (`foo.main`) from a text the
   graph reads only as an edge (contest point 5). A text-less object naming one is covered by UNKNOWN only if the static
   is address-taken: to close in S2.
5. **The link fixture**: a real wla link of a generated `sframe.asm` with an operand landing past `$FF` (wlalink must
   refuse, plan S2), and the check run on that real `.sym` (S1's check cases read hand-written `.sym`).
6. **The two games**: `place --what-if` and `check --objects` were run on the corpus only. Their builds (private) must
   be read by the game sessions or the conductor with the same commands; the S0 count said 0 refusals under
   `closure_rev` on both.
7. **Push**: qbe `1aeda99` is a local commit on branch `l4-s1-sframe-table` of the submodule, to be pushed to the fork's
   `feat/b2-far-qualifier` before this reaches `develop`'s remote (`compiler/PINS.md` names it). The game sessions are
   told before it lands: every `.c.asm` gains comment lines, `crt0.o` a `crt0.text.asm` beside it, `bin/` rebuilt.
8. Tail calls (point 6) and the emitter guard (point 7, the compiler half) are S3's; the check already refuses an `sf.`
   operand without `.b`.
