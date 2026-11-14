# L4 cut into steps: static frames (F1) and arguments in the callee's frame (change 3)

Plan written 2026-11-13, before any code, from `L4_frames_and_calls.md` (design, Fable's contest and its five changes,
M1/M2/M3), the end of `L3_machine_optimiser.md`, `../c_asm_gap.md` (Static frames along the call graph), `compiler.md`,
`testing.md`, `compiler/ABI.md`, the backend (`compiler/qbe/w65816/emit.c`), `opensnes-rom check`
(`tools/opensnes-rom/src/`), wlalink (`compiler/wla-dx/wlalink/write.c`) and the 65 linked `.sym` of the corpus. The
conductor opened the lot at its counted gain, **2.9 % (game 1) and 3.3 % (game 2) of busy time** (M2, F1 + change 3), below
the 3 % rule on game 1, because the set of axes is above it. Pair: complex, so Opus writes and Fable reviews every step's plan
and its closure (CLAUDE.md, models); Class A, so each code step is written in its own worktree
(`git checkout -B work develop` first) and the session re-runs every gate before the merge.

Nothing here is measured after a change. Every figure is a prediction or a reading of today's tree, and says which.

## 0. What the direct page can give (read 2026-11-13 from the corpus `.sym`, built after the LF+M5 lot)

Today, with D = 0, the 256 bytes of the direct page hold:

| bytes | what | fixed? |
|---|---|---|
| `$00-$33` (52) | `.registers` (`tcc__r0..r10h`, `tcc__retval_hi`, `nmi_callback`, the hooks, `tcc__fp`, `irq_callback`) | `ORGA 0 FORCE` |
| `$34-$3F` (12) | whatever the linker puts there (`.div32_ram` in `snesmod_music`) | free for the linker |
| `$40-$60` (33) | `.snesmod_zp`, only when SNESMOD is linked (4 examples of 65) | `ORGA $40 FORCE` |
| `$61-$7F` (31), or `$40-$7F` (64) without SNESMOD | linker's | free for the linker |
| `$80-$A7` (40) | `tcc__lf` | `ORGA $80 FORCE` |
| `$A8-$FF` (88) | linker's | free for the linker |

- **The linker fills the free bytes with plain globals** (minimum free in the corpus: 0 bytes; median 93 bytes of movable
  sections in the page). Those globals are addressed `.w` by C (`mul32_lo`, `div32_*` too: `sta.w`), so moving them out costs no
  cycle; it costs the plain band the bytes the static frames take.
- **What the page can give the static frames**: 164 bytes without SNESMOD, 131 with it, in holes. **Largest contiguous hole
  without moving anything: 88 bytes** (`$A8-$FF`).
- **The need (M2, measured before the LF+M5 lot)**: deepest static chain 136 bytes (game 1), 104 (game 2), temps only. With
  change 3 the parameters join the frames and the need grows (not counted yet).
- So: game 2 fits the 131 of a page with SNESMOD only if the frames can be placed in holes; game 1's 136 fits only without
  SNESMOD, or after a reorder of the page. **Hypothesis H1: whether each game links SNESMOD** (one `grep spc_ptr` on its `.sym`,
  step S0).
- **A reorder that makes the hole contiguous**: `.registers` `$00-$33`, `tcc__lf` `$34-$5B`, SNESMOD after it when linked
  (`$5C-$7C`), window to `$FF`: 164 bytes contiguous without SNESMOD, 131 with it. It also **shrinks the NMI page from 168 to
  92 bytes** (it must mirror `.registers` and `tcc__lf`, now ending at `$5C`), 76 bytes back to every ROM's plain band.
  SNESMOD's block is addressed through its labels (`snesmod.asm`, no literal `$40` but in a comment), so it should move with
  its `ORGA`: **hypothesis H2**, tested by `audio_regress.py` on the four SNESMOD examples after the move.
- **The NMI page follows only what the NMI can run.** Its bytes past `tcc__lf` mirror nothing today. A static frame needs a
  mirror only if its function can run inside the NMI callback; the placement below puts those frames first, as low as it can,
  and the NMI page grows only to the end of the highest of them (an `APPENDTO` extension generated at link time, size 0 when
  nothing needs it). A frame of a function no NMI root reaches lives in the main page alone: in the NMI, nothing addresses it.
  **Hypothesis H3**: wlalink's `APPENDTO` keeps a `RAMSECTION` contiguous with its target across objects (doc `asmdiv.rst`
  ~3028; to check with a two-object fixture before S2 relies on it). Fallback: the generated file defines the whole NMI page and
  `crt0.asm` stops defining it.
- **What must not move**: `.registers` (offsets known to hand-written assembly and PVSnesLib-compatible), the NMI page's
  alignment (a page boundary).

wlalink refuses a direct-page operand out of range, read from the source: a plain 8-bit label above 255
(`write.c` ~3237, "too much to be a 8-bit value") and an 8-bit computation (`sf_f+N`) above 255 (`write.c` ~3962, "out of
8-bit range"). So a static frame placed past `$FF` is a link error, not a silent truncation. Still to be proven by a faulted
fixture (S2), because the whole scheme's safety leans on it.

## 1. The design the steps implement (the five changes of the contest, taken)

1. **Candidates, decided by the compiler alone**: `static`, address never taken (`addrtaken()`), not a leaf (a leaf keeps
   `tcc__lf`), on no cycle of the file's call graph, no `alloc` (first delivery; a function with a stack object keeps its
   stack frame; share measured in S0). Library C: built with static frames off in the first delivery (no side table to ship);
   a call into the library is an unknown callee (below).
2. **The compiler emits, per candidate, symbolic direct-page operands** `lda.b sf.<fn>+N` (the frame's words, costliest first
   as `color_slots` already orders them), and a side table in the `.c.asm` (comment lines, as `check_nmi_race` already reads
   the `.c.asm`): frame bytes, parameter bytes (change 3), and every call it makes (direct callee, or `*` for an indirect call).
   Every non-candidate function emits its calls too (the graph needs them), and whether it is exported or address-taken.
3. **Placement at link time over the whole program** (`opensnes-rom place`, new subcommand, run after assembling and before
   `wlalink`, reading the project's `.c.asm`): builds the call graph (direct edges; an indirect call reaches every
   address-taken function of the program; a call to a symbol no `.c.asm` defines, assembly or library, reaches every exported
   or address-taken C function: it may call back), computes for each candidate f the candidates that can be active when f runs
   (its ancestors), and gives f the offset just past the deepest ancestor frame (longest weighted path: optimal for a
   contiguous window, since the conflict graph is a comparability graph). It writes `sframe.asm`: one `RAMSECTION` `FORCE` at
   the window's base sized to the deepest chain (0 bytes when there is no candidate), each `sf.<fn>` label at its offset, and
   the NMI page's extension.
4. **The overlap criterion is the check, not cycles** (contest (a)): `opensnes-rom check`, after the link, re-reads the graph
   from the `.c.asm` and the placement **from the `.sym`** (not from the placer's memory, so a placer bug is caught by an
   independent path) and refuses the link when: a candidate is its own ancestor (re-entry, with or without a cycle visible in a
   file: Fable's `f -> B.x -> A.e1 -> h` with f and h overlapping is this case); two frames overlap while one is an ancestor of
   the other; a frame of a function an NMI root reaches (an address-taken function, or one named by assembly that runs in the
   NMI) ends past the NMI page; a frame leaves the window. The message names the function, the path, and the remedy.
5. **Arguments into the callee's frame** (change 3, replaces F3): a call to a candidate stores each argument with `sta.b
   sf.<callee>+K` after every argument value is computed (QBE's `call` has all its arguments as temps already, so a nested
   call `f(a, g(b))` is done before the first store), no push, no stack restore after the call; the callee reads its
   parameters from its frame.
6. **A refusal's remedy** (contest change 5): first delivery, a per-file switch (`SFRAME_OFF := file.c` in the project
   Makefile, passed as `QBE_NO_SFRAME=1` for that file) named by the refusal message; the automatic remedy (the check writes
   the refused functions, make recompiles those files with them excluded and links again) is S7, built only if a real
   program of the corpus or of the games is refused (measured in S0 and at S3).

Why the direct page and not plain RAM: the NMI isolation (the callback runs on `tcc__nmi_registers`, the same `.b` operand
names another byte), and a third to a half of the gain (M1: 1.65 / 3.29 of 4.12 / 6.19 is the cycle of each access, which only
the direct page gives).

## 2. The steps

Each step: one sub-agent's worth (Opus in a worktree for the code, the session re-runs every gate), its prediction written here
before its code, its switch, and the faulted run that proves its check can turn red.

### S0, the count before any code (spike, no codegen change)

**Done 2026-11-13: `L4_s0_count.md`.** Rule figure (point 9, strict closure model, net of refusals) 1.71 % (game 1) and
2.07 % (game 2): the lot continues to S1, by 0.07 point, inside the spread of the gauge's windows (1.02 to 2.61 on game 2);
the stop after S3 (at least 1.07 / 1.31 %) decides. Deepest chain with parameters 90 / 80 B: S2b mandatory by point 8
(game 1, by 2 bytes). No NMI-reachable candidate; neither game links SNESMOD.

- **Object**: a count mode in the backend, `QBE_SFRAME_DUMP=1`, that writes the side table of point 2 as comments only (what
  each function would get: candidate or the reason it is not, frame words that would go static, parameter bytes, its calls);
  a host script (`devtools/sframe/sframe_count.py`, maintainer tool, never in a user build) that runs points 3 and 4 on a
  build's `.c.asm`, under the three layouts (today's `$A8-$FF`; holes; the reorder of section 0), and replays M2's traces
  restricted to the functions accepted **and placed**.
- **Files**: `compiler/qbe/w65816/emit.c` (dump only, after `color_slots`), `devtools/sframe/`.
- **What it must say, per game and for the corpus**: deepest chain in bytes, temps alone and temps + parameters (after the
  LF+M5 lot: `tcc__lf` grew by 8 bytes, M2's 136 / 104 is stale); refusals under the overlap criterion with the unknown-callee
  edges (M2's approximation saw no indirect call and no library call; **hypothesis H4: the library's callback-taking
  routines** — object engine, scene stack, game loop — **make the unknown edge refuse much more than M2's 1 of 143 / 1 of 233**);
  share of candidates with an `alloc`; the NMI-reachable candidates and their bytes (do they fit below `$A8`?); SNESMOD
  linked or not (H1); the recount of F1 and of change 3 on accepted and placed functions, split by part (access cycle, frame
  made and undone, `ldx`, `inc`, argument stores), each figure replaying the compiler's own static decision (the LF lesson: a
  figure that classifies on the executed path is written "ceiling").
- **Prediction**: deepest chain at most 136 / 104 bytes (temps moved to `tcc__lf` by the LF lot can only shorten it), plus
  10 to 30 bytes of parameters (hypothesis); refusals under 10 % of the counted gain on each game; recount F1 about 2.2 / 2.5 %
  and F1 + change 3 about 2.7 / 3.1 % (M2 less what the LF lot already took and what does not fit).
- **Gate**: the ROMs are byte-identical with and without the dump (the dump is comments): `cmp` of the 65 `.sfc`, both games'
  ROMs identical.
- **Switch**: off unless `QBE_SFRAME_DUMP=1`.
- **Faulted run**: three generated programs whose answer is known: Fable's cross-file re-entry (f and h of file A, through
  B.x and A.e1), the same inside one file, and through an indirect call; the script must refuse each, naming f; and a fourth
  without re-entry it must accept. Then break the script's unknown-edge rule (env `SFRAME_COUNT_FAULT=no_unknown`): the
  first case must pass, proving the fixture sees the rule.
- **Stop rule, written before the count**: if F1 + change 3 on accepted and placed functions is under **2.0 % on both games**,
  the lot stops after S0 and goes back to the conductor with the figure; if the deepest chain does not fit 131 bytes for a
  game that links SNESMOD, the reorder (S2b) is required, or the lot stops if that is refused.

### S1, the placer and the overlap check (`opensnes-rom place` / `check`), dormant

**Done 2026-11-14: `L4_s1_result.md`** (branch `l4s1`). ROMs identical (130/130), the checks pass on 65/65, the tool agrees
with the S0 script on 65/65 under both closure models, every fault refused by a named check; the NMI / WRAM-port lint's
blindness to static names found and fixed on the way. What S2 inherits: the result note's last section.

- **Object**: points 3 and 4 in compiled C (the user build runs no interpreter): the `.c.asm` side-table reader (beside
  `check_nmi_race`'s function parser in `asm.c`), the graph, the placement, `sframe.asm`; the check reading the `.sym` back.
  Nothing calls it from `make/common.mk` yet.
- **Files**: `tools/opensnes-rom/src/` (`sframe.c` new, `main.c`, `checks.h`), `tools/opensnes-rom/tests/`,
  `docs/tools/opensnes-rom.md`.
- **Prediction**: no ROM changes (0 bytes); on S0's dumps, the same placement and the same refusals as the S0 script (two
  implementations, one answer: the script is the oracle of the tool).
- **Gate**: the tool's tests (side tables written by hand: a chain, a diamond, the three re-entries, an NMI root, a window too
  small); the tool and the script agree on the corpus's and the games' dumps; `make test-sanitizers` (the tool is host code).
- **Switch**: not wired yet; later `SFRAME_CHECK=0` is NOT offered (a check that can be turned off is a miscompilation in
  waiting); the per-file switch of point 6 is the only way out.
- **Faulted run**: env `OPENSNES_SFRAME_FAULT=overlap` makes the placer give the third re-entry's h the offset of f; the check
  must refuse the link from the `.sym`. `OPENSNES_SFRAME_FAULT=nmi` places an NMI-reachable frame past the NMI page: refused.

### S2, the window and the NMI page sized at link time, still empty

- **Object**: `make/common.mk` runs `opensnes-rom place` before `wlalink` and links `sframe.asm`; with no candidate it is a
  0-byte window and no NMI page extension. S2b (only if S0 asks for it): the reorder of section 0 (`tcc__lf` to `$34`, SNESMOD's
  `ORGA` after it, the NMI page 92 bytes).
- **Files**: `make/common.mk`, `tools/opensnes-rom/`; S2b: `templates/crt0.asm`, `lib/source/snesmod.asm`,
  `compiler/qbe/w65816/emit.c` (`DPF_BASE` names `tcc__lf`, the operand does not change), `lib/source/sprite_meta.asm` and
  `sprite_world.asm` (through `tcc__lf`, unchanged), `compiler/ABI.md` (direct page layout), `.claude/rules/compiler.md`.
- **Prediction**: S2: the 65 ROMs byte-identical, `.sym` identical but the new zero-size section. S2b: code identical, every
  address in `$34-$FF` moves; NMI page -76 bytes, plain band +76 bytes free in every ROM; `wram_regress` moves everywhere
  (expected, the reason in the commit); audio identical (H2).
- **Gate**: S2: `cmp` of the 65 ROMs and both games'. S2b: `make clean && make`, `make tests`, `diff_corpus.py` 65/65 MATCH,
  `audio_regress.py` (with `luna diff --audio --align-onset` quoted), `nmi_budget.py`, the leaf-frame fixture (main loop
  against NMI), both games' state at every gauge step.
- **Switch**: S2 has nothing to switch (no effect); S2b is a layout, not switched (a layout with two variants is two ABIs).
- **Faulted run**: a two-line fixture whose `lda.b sf.x+N` lands at `$100`: wlalink must refuse (proves the reading of
  `write.c` above); a fixture with an `APPENDTO` extension in another object: the `.sym` shows it contiguous (H3). S2b: put
  the NMI page back to 168 bytes but `tcc__lf` at `$34` with the page shortened to `$40`: the leaf-frame fixture must fail.

S2 ships alone (no effect). S2b ships alone only for its own gain (76 bytes of plain band) if the conductor wants it without L4.

### S3, F1: the candidates' temps in their static frame

- **Object**: in `color_slots`, a candidate's temps that would take a stack slot (cross a call, or overflow `tcc__lf`) take a
  slot in a new range (`SFSLOT`, like `DPSLOT`) that `SOFF` / `emit_stack_*` print as `sf.<fn>+N`; `framesize` 0 when nothing is
  left on the stack (parameters then read at `PARAM_OFFSET + n`: the `textInit` precedent of 2026-10-08); the side table for
  real; the text rules see the new operand as **opaque** (a named direct-page symbol, not a frame slot: no rule rewrites it).
  `testing/difftest_stmt.py` first learns to generate static non-leaf functions with two call sites (one site is inlined
  whole by `splice_fn` and would never be a candidate: today's odd seeds do not reach this code), with values that cross
  calls, and the three re-entry shapes (which must be refused at link, never run).
- **Files**: `compiler/qbe/w65816/emit.c`, `make/common.mk` (per-file switch), `testing/difftest_stmt.py`,
  `testing/fixtures/compiler/static_frame/` (new), `devtools/compiler-tests/cases/` (`sframe`, `sframe_off`),
  `compiler/ABI.md`, `.claude/rules/compiler.md`, `KNOWN_LIMITATIONS.md` (the refusal and its switch).
- **Prediction** (from M2's F1, 2.38 / 2.69 %, split by M1's proportions; to be replaced by S0's recount before the code):
  the access cycle and the frame made and undone only, the `ldx` and `inc` waiting for S4: **about 1.8 % (game 1) and 2.1 %
  (game 2)**; code size down by the prologues and epilogues that go (`lda.b d` and `lda n,s` are both two bytes;
  hypothesis -0.5 to -1 %).
- **Gate**: `make clean && make`; `make tests`; `make test-compiler`; `difftest.py` and `difftest_stmt.py` 20 000 seeds each
  (new shape: the peephole rule's standard); `diff_corpus.py` 65/65 MATCH; the fixture `static_frame` (values against a host
  model; the same file's static functions run by the main loop and by the NMI callback; a function reached from the NMI
  placed below `$A8`); both games' state identical at every gauge step and their busy cycles measured; `make
  test-sanitizers`; `make test-torture` once (C conformance with candidates everywhere).
- **Switch**: `QBE_NO_SFRAME=1` (the compiler emits no candidate; the ROM is then S2's).
- **Faulted runs**: (1) `QBE_SFRAME_FAULT=cycle` gives a static frame to a function on a cross-file re-entry: the link check
  refuses; (2) shorten the NMI page extension by one word in `sframe.asm`: the check refuses, and with the check bypassed by
  hand the fixture's NMI half fails (proves the fixture sees D); (3) `QBE_SFRAME_FAULT=framesize` reads the parameters as if
  `framesize` were 2: `difftest_stmt` must find it within its gate.
- **Ships alone**: yes, with S1 and S2 before it; never before S1 (contest change 1).

### S4, the text rules learn the static-frame slot

- **Object**: `pp_slot`, `xf_slot`, the reload, `tax`, `ldx`, `inc`/`dec` and loop-in-X/Y rules, `peephole_slotdead`, take
  `sf.<fn>+N` as a direct-page frame slot, with the one difference that matters: **a call does not end its value** (a
  `tcc__lf` slot is dead after a `jsl`; a static-frame slot survives it, it is the point). `pp_slot_live_at` walks through a
  `jsl` for it; `xflow` forgets A and X at a call but keeps nothing about the slot either (conservative: the slot's value is
  kept, what A or X equals is not).
- **Files**: `compiler/qbe/w65816/emit.c`, compiler checks (`sframe_peep`, `sframe_peep_off`).
- **Prediction**: the `ldx` and `inc` parts on placed functions, about **0.6 % (game 1) and 0.5 % (game 2)**, plus what the
  base peephole removes after them (the LF+M5 lot found a chain three times its count: hypothesis, not counted).
- **Gate**: as S3 (20 000 seeds each, a new rule class).
- **Switch**: `QBE_NO_PEEP_SFRAME=1` (operands opaque again, S3's code).
- **Faulted run**: `QBE_SFRAME_FAULT=callkill` makes `pp_slot_live_at` treat a `jsl` as the end of a static-frame slot (the
  `tcc__lf` behaviour): the store of a value read after a call goes; the fixture `static_frame` and `difftest_stmt` must turn
  red. This is the "two rules right and wrong together" case of `compiler.md`: the rule that is right for `tcc__lf` is wrong
  here.

### S5, change 3: the arguments stored into the callee's frame

- **Object**: a call to a candidate stores its arguments with `sta.b sf.<callee>+K` (32-bit and far-pointer arguments in two
  words) and pops nothing; the callee's parameter slots are in its frame (the placement counts their bytes); the side table
  gives parameter bytes. `peephole_slotdead` and S4's liveness count **a `jsl` to f as a read of f's parameter words**, since
  nothing in the caller reads them again.
- **Files**: `compiler/qbe/w65816/emit.c` (call emission, `count_fn_param_bytes_via_slots`, the parameter read path at
  `PARAM_OFFSET`), `tools/opensnes-rom/src/sframe.c` (parameter bytes), compiler checks (`sframe_args`, `sframe_args_off`),
  `compiler/ABI.md` (a `static` function may take its arguments in the direct page: no assembly can call it, its name is not
  exported).
- **Prediction**: M2's change 3, **0.50 % (game 1) and 0.58 % (game 2)** (the pushes and the caller's stack restore), less
  what a larger chain makes not fit (S0).
- **Gate**: as S3; the fixture adds `f(a, g(b))` with f and g siblings at the same offset, a 32-bit and a far-pointer
  argument, a call whose result is an argument of the next.
- **Switch**: `QBE_NO_SFRAME_ARGS=1` (arguments pushed again; F1 stays).
- **Faulted runs**: `QBE_SFRAME_FAULT=argsdead` lets `slotdead` remove an argument store before the `jsl` (the callee's read
  is invisible in the caller's text): fixture and `difftest_stmt` red. `QBE_SFRAME_FAULT=argsearly` stores the first argument
  before evaluating a nested call: the sibling fixture red.
- **Ships alone**: after S3 (S4 not required, but S5's liveness change goes into S4's code if S4 is in).

### S6, the gauge and the lot's closure

Both games measured with each switch alone and together (the LF+M5 protocol: builds before and after, gauge window, state
identical at every step), the corpus `diff_corpus` and the oracles re-captured with their reasons, `make bench-gap`, the
lesson in the kit's `lecons/`, the notes and `CHANGELOG`.

### S7, only if needed: the automatic remedy for a refusal

The check writes the refused functions; `make` recompiles their files with them excluded (`QBE_SFRAME_EXCLUDE=list`) and links
again, once. Built only if S0 or S3 shows a refusal in the corpus or the games; otherwise the per-file switch and its message
stand, written in `KNOWN_LIMITATIONS.md`.

## 3. Order, and what ships alone

`S0 -> S1 -> S2 (S2b if S0 asks) -> S3 -> S4 -> S5 -> S6`, S7 on demand.

- Alone: S0 (comments only), S1 (dormant tool), S2 (empty window), S2b (for its own 76 bytes), S3 after S1+S2, S4 after S3,
  S5 after S3.
- Never alone: S3 before S1 (a static frame without the overlap check is a miscompilation the link cannot see); S5 without
  the liveness of its own argument stores (inside S5); S4 without its call-survival rule (it is the step).
- Each step announced to the games before it lands on `develop` (`make/common.mk` and the compiler change their builds), with
  `bin/` rebuilt on the checkout they read.

## 4. Silent failures, and what catches each

| risk | how it would show | what catches it |
|---|---|---|
| a re-entry the graph does not see (assembly calling C by name, a table of code in assembly) | a frame overwritten while live, wrong value, no error | unknown callee reaches every exported or address-taken C function; check reads the `.sym`; S0 faulted fixtures |
| a text rule treating a static slot as `tcc__lf` (dead at a call) | a store removed, a value read after a call stale | S4's faulted `callkill`; 20 000 seeds |
| an argument store removed (its read is in the callee) | the callee reads the previous call's argument | S5's `argsdead` |
| parameters read at a wrong offset when `framesize` becomes 0 | every parameter off by 2 | S3's `framesize` fault; `difftest_stmt` |
| an NMI-reached frame outside the NMI page | wrong values in the callback only | the check; fixture's NMI half |
| a frame past `$FF` truncated to its low byte | another variable written | wlalink refuses (read), S2's faulted fixture proves it |
| assembly that moves D and calls C (none today, contest (a)) | a frame read through the wrong page | **hypothesis H5**: none in `lib/source`, `templates`; S0 greps every `tcd`/`pld` region for a `jsl` to a C symbol; a lint line if one can appear |
| the Super FX owned-cart NMI path | it keeps the interrupted D (`crt0.asm` ~2458, `phd` without `tcd`) and calls `gsu_present_step`, assembly | S0 confirms it calls no C (H5) |
| the library's C (frames off) reached through a callback | none: its own functions keep stack frames, the unknown edge covers its calls back | S0 counts what that costs |

## 5. Hypotheses, and the measurement that settles each

- **H1** each game links SNESMOD or not: its `.sym` (S0).
- **H2** SNESMOD's block moves with its `ORGA`: `audio_regress.py` after S2b.
- **H3** `APPENDTO` keeps the NMI page extension contiguous: two-object fixture (S2).
- **H4** the library's callback routines make the unknown edge refuse far more than M2's approximation: S0's refusals.
- **H5** no assembly runs C on a moved direct page: S0's grep, kept as a lint if it is cheap.
- **H6** the deepest chain after the LF lot, with parameters, fits 131 bytes (SNESMOD) or 164 (without): S0.
- **H7** the NMI-reachable candidates fit below `$A8` in the holes (no NMI page growth) or need an extension: S0.
- **H8** the recount replays the compiler's decision: here a candidate's stack temps all go static whatever the executed path,
  so M2's executed-path classification does not bias F1 as it biased M3; it can bias the window fit (frames are static
  sizes). S0 measures frames from the compiler's dump, not from the traces.
- **H9** QBE parses the whole file before emitting, so `addrtaken()` knows a later function's address-taking when an earlier
  candidate is decided (the `rep #$20` rule relies on the same): a compiler check where a function's address is taken after
  its body (S3).
- **H10** S3 and S4's split of the gain follows M1's proportions: the gauge with `QBE_NO_PEEP_SFRAME=1` (S4).

## 6. What Fable is asked to contest (before S0 starts)

1. The **unknown-callee edge** (a call into assembly or the library reaches every exported or address-taken C function):
   sound? too coarse (H4)? Is there an entry into C the graph still misses (a hook pointer set by assembly to a C function
   by name, `main`, a `__ramcode` copy)?
2. **Confining NMI-reachable frames low and sizing the NMI page at link time**, instead of growing the page to 256 bytes: is
   "reached from an address-taken function" the right NMI root set, given that `nmi_callback` and `nmi_lag_callback` are the
   only ways C runs in the NMI today?
3. **The page reorder (S2b)**: worth its WRAM-wide move for 76 bytes and a contiguous window, or are holes enough?
4. **Opaque operands first (S3), rules after (S4)**: is an opaque `sf.<fn>+N` really invisible to every text rule today
   (`fuse_plain_src` takes a named symbol as a source: safe here?), or is there a rule that reads any `.b` operand as
   `tcc__lf`?
5. **Change 3's ordering argument** (all argument values exist before the first store, because QBE's `call` lists temps):
   does any emit path compute an argument at the call (a far pointer's bank, a `Kl` high half) after a store to the callee's
   frame?
6. **The stop rule** (under 2.0 % on both games after S0, stop): too lenient, given the lot was opened under the 3 % rule?
7. **The per-file switch as the only first remedy**: acceptable against `PHILOSOPHY.md` (hidden quirk with a documented
   escape), or must S7 come with S3?

## Contest (Fable, 2026-11-13, written): the plan does not start as written; changes taken

Finding that moves everything (counted on the examples' `.c.asm`): of 98 non-leaf `static` functions emitted, 85 call directly a symbol
no project `.c.asm` defines (library or assembly), and 92 % reach one by transitivity. With the plan's "unknown callee reaches every
exported function" edge each of them is its own ancestor and is refused: M2's refusals (1 of 143 / 233) do not hold for the rule as
written. H4 is therefore under-estimated; the library's closure table is not an economy to skip. Taken, in order:
1. **S0 counts under three edge models** (coarse edge; library closure table; M2's approximation) and separates the structural gain
   (frame made and undone, pushes, stack restore) from the "one cycle per `n,S` access" part (40 to 53 % of M1's F1). It reports the
   NMI bytes under the sound root set, the single-band RAM cost per game (window plus NMI mirror: up to 2 x 136 bytes, 23 % of the
   tight game's free bytes) and the deepest chain with parameters against 88 / 131 / 164.
2. **S1 carries the library's closure table**: C of the library emits its call table (comments) even with frames off and installs it
   with the objects; assembly (library and project) is read by `place`: a routine is closed if its `jsl/jml/jsr/jmp` reach closed
   routines, with no indirect jump and no `pea` + `rtl`. Table absent = the coarse edge (the safe default). The runtime helpers
   (`__*`, `tcc_*`) are named explicitly (the allow-list of `pp_slot_live_at`), with a provoked case.
3. **The link check derives the edges and each frame's extent from the instruction lines** of the `.c.asm` (`call_target` exists in
   `tools/opensnes-rom/src/asm.c`), `jml` tail calls included, and compares them to the tables: otherwise the placer and the check
   share one source and agree on a missed edge. Equality of the `sf.*` of the `.sym` with the tables; the exact list of linked
   objects comes from `make`, not from a directory; provoked fault `QBE_SFRAME_FAULT=noedge` (the table hides a call) must fail the link.
4. **`place` refuses to write `sframe.asm` on a re-entry**, known before the link, so a project that links otherwise fails on an
   undefined label instead of getting an unchecked ROM.
5. **NMI page = the whole window in the first delivery** (size zero without candidates); confining low frames is a later
   optimisation on a root set made of the literal arguments of `nmiSet` / `nmiSetLag` (the `pea` + `jsl` pattern `check_nmi_race`
   reads), falling back to "every address-taken and every exported function" when an argument is not constant or an assembly file
   writes `nmi_callback`. `irqSet(C function)` and assembly naming a decorated `static` are refused at link.
6. **Tail calls**: S3 handles them or turns them off for candidates; S5 covers the four quadrants (candidate or not, to candidate or
   not); a check `sframe_tco` and a `difftest_stmt` form (`return g(...)` between `static` functions with two sites). The tail-call
   branch is taken before the normal one and stores arguments in the caller's own stack parameter slots, which a candidate lacks.
7. **Emitter guard**: an internal error if an `SFSLOT` number reaches a path other than the three printers (`slot >= DPSLOT`
   tests would take it for a `tcc__lf` word: `color_slots`, `SOFF`, 25 uses of `DPSLOT` / `DPF_BASE`); the invariant "every rule
   that re-emits an `sf.*` operand keeps its `.b`" gets its check (no line carries `sf.` without `.b`) and its provoked fault; S4's
   rules are written on `sf.<self>+N` only, the words of a callee's parameters stay write-only.
8. **S2b decided by contiguity alone**: deepest chain with parameters at most 88 bytes on both games = no S2b in this lot; above =
   S2b mandatory, in its own commit before S3. Its gate compares by symbol (addresses move). A provoked fault: the window forced
   over `.snesmod_zp` or `tcc__lf` must fail the link.
9. **Stop rule rewritten**: stop if the structural part plus half of the access-cycle part is under 2.0 % on both games, under the
   edge model actually delivered and net of refusals; a second, measured stop after S3: the gauge must return at least 60 % of
   S3's prediction, else back to the conductor before S4 and S5 (the LF lot returned 0.00 / 0.24 for 0.64 / 1.14); the RAM cost enters the rule.
10. **S7 with S3**, or S3 ships off by default (`SFRAME := 1` to enable) until S7: an optimisation on by default that fails a link
    that passed yesterday is not a sane default (PHILOSOPHY.md); S0 and S3 count the refusals on the corpus and both games, and one
    refusal on a program that links today makes S7 mandatory before default activation.
11. Minor: the separator of `sf` fixed and tested with a file named `b.c` (WLA may read `sf.fn.b+2` as a size suffix; hypothesis);
    `argsearly` requalified as a proof of the set-up, not of a rule; S2b's gate and the SNESMOD-linked case beyond 131 bytes are a
    link refusal, not a silent shrink.

## Stop reached after S3 (2026-11-14): S2 and S3 are NOT on `develop`

S3's prediction was written before its builds and measurements (1.4-1.8 / 1.9-2.2 % of busy time, jeu 1 / jeu 2); the stop rule (contest point 9)
asked for at least 1.07 / 1.31 %. Measured with the ROMs built `SFRAME=1`, state identical to the reference at every step:
**+0.54 % (slower) on game 1 and -1.06 % on game 2**. The functions with a frame lose 0.66 % of game 1's profile and gain 1.20 % of game 2's;
the others (code unchanged) move -0.11 / +0.15 % (data and code displaced by the window). In the lines of the losers `lda.b sf... / tax` comes back
at the head of each block: on a stack slot `N,s`, `peephole_xflow` knew X already held the value and `lda S / sta S` and `peephole_entry` applied;
the opaque `sf.*` operand loses what the text rules did on the stack, and S0's count (a replay of traces without recompiling) could not see it.
Lesson in the kit. Per the rule the lot stops and goes back to the conductor before S4. What stays: the notes `L4_s2_result.md`,
`L4_s3_result.md`; the code stays on the worktree branch `l4s23` (S2 `be8da548`, S3 `e13266df`, QBE `a4dcb1b`, local to the submodule: not on the
fork, not on `develop`) for a later S4. With `SFRAME := 1` as is, `place` refuses both games' links (chains of 98 / 90 bytes with the
reviewed-routine closure; both link with 2 and 1 excluded functions): S7 is a precondition of any default activation, and S2b of S5.
