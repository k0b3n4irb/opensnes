# L4 S3: F1, the candidates' temps in their static frame

Step S3 of `L4_plan_decoupe.md`, with the contest's points 6 (tail calls off for candidates), 7 (emitter guard,
the `.b` invariant and its fault), 9 (the measured stop after S3), 10 (`SFRAME := 1`, off by default until S7) and
11. Branch `l4s23`, nothing pushed.

## Prediction (written 2026-11-14, before the S3 commit; committed alone)

Nothing below is measured after the change.

**What is built.** A candidate (static, address never taken, calls, not all its calls tail calls, no alloc, on no
cycle of its file's direct calls, frame at most 88 bytes, not in `QBE_SFRAME_EXCLUDE`) keeps every temp that had a
stack slot in its static frame, with the colouring the stack had (slot s -> byte 2s of `sf.<fn>.f`), `framesize`
0, parameters still pushed by the caller and read at `PARAM_OFFSET + n`. Tail calls off for those functions. The
text rules see `sf.*` as opaque. `QBE_SFRAME=1` (make `SFRAME := 1`) turns it on, `QBE_NO_SFRAME=1` off; library C
always off. Remedies a refusal names: `SFRAME_OFF := file.c`, `SFRAME_EXCLUDE := name.file`.

**Predictions.**
1. **Without `SFRAME`**: the 65 corpus ROMs and `.sym` byte-identical to develop's (the compiler's new paths are
   all behind `QBE_SFRAME=1`); both games' ROMs identical to the gauge's develop build. Contradicted by any byte.
2. **With `SFRAME := 1` on the corpus**: frames for the S0 candidates that keep stack temps (at most S0's 83
   candidates, hypothesis 40 to 60 of them have stack temps); deepest chain of temps at most 38 B (S0's figure
   with parameters, closure + reviewed); no refusal by `place` or `check` (S0: 0 refused under that model);
   `diff_corpus` 65/65 MATCH against the develop ROMs (state identical); `make tests`' oracles that hash WRAM move
   (the window takes `$A8+` from the linker's globals).
3. **Games with `SFRAME := 1`**: `place` REFUSES both links as they stand: S0's deepest chain of temps alone under
   closure + reviewed is 98 B (game 1) and 90 B (game 2), over the 88 bytes of `$A8-$FF`. With `SFRAME_EXCLUDE` set
   to the frames `place` names past the window, both link; hypothesis 1 to 3 functions a game.
4. **The measured stop (point 9)**: S3's prediction is S0's F1 under the closure model, **1.79 % (game 1) and
   2.18 % (game 2)** of busy time (structural 1.05 / 1.01 + access cycle 0.74 / 1.17). Corrections I expect, each
   lowering it: the excluded frames (S0 placed in 88 B lost 0.27 point on game 1 with parameters; temps alone,
   hypothesis 0.1 to 0.3), the tail calls turned off (bound 0.04 on game 2, 0 on game 1), and that S0's model gave
   a frame to every candidate with stack temps while S3 leaves out the ones whose calls are all tail calls.
   **Predicted measured: 1.4 to 1.8 % (game 1) and 1.9 to 2.2 % (game 2).** The rule: at least 1.07 / 1.31 %, else
   stop and back to the conductor before S4. State identical to `depart` at every gauge step.
5. **Code size**: C code down by the prologues / epilogues that go (`tsa / sec / sbc / tas` and its twin, 8 to 10
   bytes a framed function) and up by nothing (`lda.b d` and `lda n,s` are both two bytes): hypothesis -0.5 to -1 %
   of the games' C code.
6. **Faults**: `overlap` and `nmipage` refused by the link check; `absolute` refused, and linked by hand the
   fixture fails (it sees D); `callkill` and `framesize` turn the fixture red and `difftest_stmt` red on its gate
   seeds (the generator learns static non-leaf helpers with two call sites); `nob` stops the compiler; `cycle` is
   refused by `place`.

(Honest timing: the prediction above was written once the S3 code was drafted and before any build of the games or
any measurement with it; it was committed before the code, `528e1cf8`.)

## Result (2026-11-14, measured after the prediction was committed)

**Verdict: the measured stop of point 9 is reached. S3 returns a busy-time change of +0.54 % (game 1, slower) and
-1.06 % (game 2), against at least -1.07 / -1.31 % required (predicted -1.4 to -1.8 / -1.9 to -2.2). Stop, back to
the conductor before S4.** Correct everywhere (state identical to `depart` at every gauge step on both games, 65/65
MATCH, the hunts clean, every fault red); without `SFRAME` every ROM is byte-identical.

### Why it does not pay (measured, not supposed)

The gauge's profiles (`profil.json`, self cost per function, `l4s3-off` against `l4s3`, same frames):

| % of the profile | game 1 | game 2 |
|---|--:|--:|
| functions with a static frame (73 / 162 of them) | **+0.66** | **-1.20** |
| the others (code identical; data and code moved by the window) | -0.11 | +0.15 |

In the functions that lose (game 1: two of them +10 % and +8 % of their own cost), the text before / after shows the
same thing block after block: `lda.b sf.<fn>.f+N / tax` comes back at the head of each block where
`peephole_xflow` knew that X already held the slot on `N,s`, and `lda S / sta S` and the entry reload
(`peephole_entry`) stop applying. The plan's split "opaque operand in S3, the rules in S4" counted for S3 the access
cycle and the frame made and undone only, and supposed that the text rules did little on stack slots: they did
enough to cancel the gain on game 1. **S4 is therefore what makes the static frames pay, not an option**; the
conductor decides whether S4 is worth starting on that fact (its own prediction was 0.6 / 0.5 % for `ldx` and
`inc`, before this loss was known; recovering the loss is a larger and unmeasured part of it).

### The games

| | game 1 | game 2 |
|---|--:|--:|
| `place` with `SFRAME := 1` as they stand | refused: chain of 98 B | refused: chain of 90 B |
| frames past the window, kept on the stack (`SFRAME_EXCLUDE`) | 2 functions | 1 function |
| framed functions, linked | 73 | 162 |
| busy per step, off -> on | 374 860 -> 376 889 (+0.54 %) | 620 758 -> 614 201 (-1.06 %) |
| busy per frame | 187 430 -> 188 445 | 206 919 -> 204 734 |
| late frames | 172 -> 187 | 430 -> 423 |
| code (C) | 111 243 -> 110 469 B (-0.70 %) | 130 890 -> 128 443 B (-1.87 %) |
| state at every gauge step | identical to `depart` (341 steps) | identical (700 steps) |
| `SFRAME` unset: ROM | identical to `l4s0-sansdump` (develop) | identical |

Prediction 3 holds to the byte (98 and 90 B, S0's figures under closure + reviewed routines; the conductor's "no S2b"
read the strict closure's figures, 64 / 56 B of temps and 90 / 80 with parameters). The busy figures of the off build equal `l4s0-sansdump`'s to the cycle (same
ROM): the gauge is deterministic, the differences are the change's and the layout's.

### The corpus with `SFRAME := 1`

37 framed functions in 19 examples, deepest chain 30 B, no refusal; the 19 ROMs that change are those 19;
`diff_corpus.py --ref <develop's ROMs>`: `65/65 match, 0 diff, 0 missing (tolerance ±0)`.

### What was built (branch `l4s23`; qbe `a4dcb1b`, local branch `l4-s3-sframe` of the submodule)

- `emit.c`: `sframe_wanted` (the candidate rule above), the remap after the colouring (stack slot s -> `SFSLOT + s`,
  `sf_bytes = 2 * slots`), `soff()` in place of the `SOFF` macro (a stack slot in a framed function is an internal
  error), `color_slots` refuses to run on a function whose frame is given, the three printers print
  `<op>.b sf.<fn>.f+N`, `sf_text_check` after every text rule (an operand naming a frame must be `.b` on the
  function's own frame), no tail call from a framed function (contest point 6), the side table's `sf=` and `stack=`
  (the stack bytes before the remap). `main.c`: `filecall_oncycle` over the unit's direct calls after inlining.
- `make/common.mk`: `SFRAME ?= 0`, `SFRAME_OFF`, `SFRAME_EXCLUDE` (in the config stamp), `sframe_env` per source;
  `lib/Makefile`: `QBE_NO_SFRAME=1` for the library. `place`'s refusal names the frames past the window and the
  `SFRAME_EXCLUDE` line, and the remedies.
- Tests: checks `sframe`, `sframe_off`; fixture `testing/fixtures/compiler/static_frame` (in `make tests`);
  `devtools/sframe/test_sframe_faults.py` (in `make tests`; `--difftest` in `make test-sframe`);
  `difftest_stmt.py`: `SFRAME := 1` in its projects, the 4k+1 seeds gain a static `wrap` helper (calls a pure helper
  twice, called from two places) and a second call site of each program that never runs; a link refused for the
  window is rebuilt once with the frames it names excluded (the harness does what S7 would). The gate's 28 seeds
  hold 13 projects with frames (1 to 18 framed functions, chains up to 38 B).
- Docs: `docs/tools/build.md` (the three variables), `compiler/ABI.md` (direct page layout, static frames),
  `KNOWN_LIMITATIONS.md` (the refusal and its remedies), `.claude/rules/compiler.md`, `CHANGELOG.md`.

### Faults (each turned red by its check; `test_sframe_faults.py --difftest`)

| fault | seen by |
|---|---|
| `QBE_NO_SFRAME=1` (the switch) | the ROM equals `SFRAME := 0`'s |
| `OPENSNES_SFRAME_FAULT=overlap` | link check: `overlap:` |
| `OPENSNES_SFRAME_FAULT=nmipage` (the NMI page a word short) | link check: `does not cover the window` |
| `QBE_SFRAME_FAULT=absolute` (frames addressed `.w`, the same bytes from the NMI) | link check: `without .b`; linked by hand, the fixture fails: `bad_main` = 24 (the NMI's step() wrote the main loop's frame each time) |
| `QBE_SFRAME_FAULT=callkill` (tcc__lf's "a call ends the value" applied to the frame) | fixture: `want_main`, `want_nmi` wrong; difftest_stmt gate: 18/28 seeds clean |
| `QBE_SFRAME_FAULT=framesize` (parameters read as if the frame were 2 bytes) | fixture: results 0, the run never ends; difftest_stmt gate: 16/28 seeds clean |
| `QBE_SFRAME_FAULT=nob` (a frame operand loses `.b`) | the compiler stops: `L4 S3 invariant` |
| `QBE_SFRAME_FAULT=cycle` (frames for a cycle of the file) | `place`: `reentry: even.main ... -> odd.main -> even.main` |

The plan's fault "(2) the NMI page short, check bypassed by hand, the fixture's NMI half fails" was replaced by
`absolute`: with the page short by a word the NMI's last frame word lands on whatever RAM follows the page, which a
fixture cannot make deterministic; `absolute` proves the same thing (the fixture sees which page the NMI uses).

### What is left (for the conductor's decision first: the stop of point 9 is reached)

- **S4** (the text rules learn `sf.<self>+N` as a direct-page frame slot that a call does not end): now the step that
  carries the gain, and the first to measure if L4 goes on. Its first target is measured above: `peephole_xflow` (A
  and X known across blocks), `peephole_entry`, the base pass's `lda S / sta S` and reload rules, `fuse_plain_src`'s
  slot case, `peephole_late`'s `ldx.b` and `inc.b`; `pp_slot_live_at` walks through a `jsl` for these slots;
  `QBE_SFRAME_FAULT=callkill` (built here, in `peephole_slotdead`) is its provoked fault. To predict it, recompile
  with the rule and count, not replay a trace (the lesson of this step).
- **S5** (arguments into the callee's frame): unchanged by this step; its four tail-call quadrants (point 6) start
  from "no tail call from a framed function" here. The window is 88 B and both games already need exclusions at
  temps alone: S5's parameters (150 / 116 B under the delivered model, S0) make **S2b mandatory before S5**, or
  many more exclusions.
- **S7** (a refused link falls back by itself): `place` already prints the exact `SFRAME_EXCLUDE` list for a window
  overflow and `difftest_stmt.py` does the fallback once in its harness; S7 is that loop in `make` (recompile the
  named functions' files with `QBE_SFRAME_EXCLUDE`, link again), plus a re-entry refusal's fallback (exclude the
  named function). Needed before `SFRAME` can be on by default (contest point 10), and both games are refused today
  without it.
- Not done in S3: `make test-sanitizers` (host code of QBE and opensnes-rom changed; not run, it rebuilds the whole
  toolchain); the qbe commit `a4dcb1b` (and S1's `1aeda99`) are local to the submodule and
  must reach the fork before this branch reaches `develop`'s remote; the games' sessions must be told before it
  lands (every link now runs `place` and `check --objects`, `bin/` rebuilt).

### Gates (2026-11-14, on the S3 tip, machine otherwise idle, each alone)

- `make clean && make`: exit 0, the same four warning lines as develop's build (clang `-rdynamic`, the CMake
  deprecation, make's jobserver x3).
- `cmp` of the 65 `.sfc` and `.sym` against develop's, `SFRAME` unset: **130 files, 0 differ**. The two games built
  by the gauge, `SFRAME` unset: ROMs identical to `l4s0-sansdump`.
- `make test-compiler`: `Compiler checks: 152 passed, 0 failed (+0 compile-only; 0/0 unchecked ratchet; 31/31
  refusals pinned)`.
- `make tests`: `ALL CHECKS PASSED (luna)` (difftest 16/16, difftest-stmt 28/28, coverage 63 OK / 2 INPUT-DEP of 65
  at both power-ons, the fixture `static_frame` 11/11 PASS, `sframe corpus: 65/65`, link fixture green, `sframe
  oracle: 130/130`, `sframe faults: each refused or seen by its check`, `torture --quick: all 26 as expected`).
- `diff_corpus.py --ref <develop's ROMs>`: `SFRAME` unset, byte for byte (above); `SFRAME=1`: `65/65 match, 0 diff, 0
  missing (tolerance ±0)`.
- `python3 testing/difftest.py --seeds 28001-30000`: `difftest: 2000/2000 seeds clean, 48000 expressions in 4 shapes`.
- `python3 testing/difftest_stmt.py --seeds 28001-30000`: `difftest-stmt: 2000/2000 seeds clean, 20000 programs`.
- `devtools/sframe/test_sframe_faults.py --difftest`: every fault red (table above).
- `SFRAME=1 python3 testing/torture/run.py` (the whole gcc.c-torture/execute, every project built with static frames):
  `torture: 1702 tests: PASS 707, FAIL 15, NOBUILD 15, REFUSED 965` / `torture: no regression (707 pass)`.
- `make lint`: exit 0 (`lint-docs` no drift). `devtools/verify_toolchain.py`: `OK: toolchain pins match`.
