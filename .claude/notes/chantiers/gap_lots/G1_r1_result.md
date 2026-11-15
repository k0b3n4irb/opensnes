# G1 R1 — the technique table (result)

Branch `g1-r1` from `g1-r0` `3ee83ec5` (R0's bench `bcdbcf46`; compiler, library, templates and `make/` of `develop`
`3139a9ef`), built `make clean && make`, no `QBE_*` variable. Nothing of this enters `develop`.

## Prediction (written 2026-11-15 before any measure; committed alone)

Written after reading the generator's emitted text of the rows (`build/.../tech.c.asm`, no `QBE_*` switch) and before any
luna run. Unit: master cycles a call of the row (16 calls between the marks, minus the empty routine, as `gapbench`), the
gain of the hand pass against the compiler's own text for the same C, SlowROM / FastROM. A negative figure is a gain.

The pass is the compiler's text for the C row with ONE technique applied by hand, nothing else; the rows live in a new ROM
`devtools/g1bench/tech.{c,asm}` (copies of `gapbench`'s `mul`, `fixed`, of `forms.c`'s `mobaos`, `mobptr`, `btab`; new:
`mobfp`, a function per entity taking `Mob *`; `wpack` and `wpair`, neighbouring byte writes to a `FAR` buffer). Each row
also carries a verbatim copy of the compiler's text (`_cp`) that must read the C row's cycles (the control).

Stop threshold (step 3 of the brief): a measured gain under **half the point prediction** on its row is "under prediction".

| technique | kind | row(s) | predicted gain, slow (point, range) | predicted gain, fast (point) | stop threshold slow / fast |
|---|---|---|---|---|---|
| T1 direct page on the entity (`tcd`, fields `lda.b`; temps that were in the direct page go absolute while D is moved) | exists only inside an allocator (D is a register of the bank; the direct-page temps must move out); measured as a hand pass with the temps forced absolute | `mobfp` (D = the `Mob *` argument in the callee, `phd`/`pld`) | -2 400 (-1 700 to -3 000) | -1 800 | -1 200 / -900 |
| | | `mobaos` (D walks the entities in place of X) | -1 100 (-900 to -1 300) | -850 | -550 / -425 |
| | | `mobptr` (D = the pointer's offset, loop control untouched) | -300 (0 to -600) | -250 | -150 / -125 |
| T2 far pointer as `[dp]`, 16-bit step | already in our generator (`QBE_NO_FARPTR_DP`); not re-measured | — | quote | | |
| T3 first argument in A | allocation decision; measured and stopped; quote | — | quote | | |
| T4 hardware multiplier as a typed operation | isolated pass (instruction selection); the slack use is scheduling, measured as a second pass | `mul` typed 8 x 8, waits as `nop` (`_t4n`) | -9 500 (-8 000 to -12 000) | -8 000 | -4 750 / -4 000 |
| | | `mul` typed, the wait filled with the `a + 3` store (`_t4s`) | -11 000 (-9 000 to -13 000) | -9 500 | -5 500 / -4 750 |
| | | `fixed` short s16 x u16 -> 32 inline (the shift chain kept) | -35 000 (-28 000 to -42 000) | -30 000 | -17 500 / -15 000 |
| T5 `mvn` stub for copies | delivered in the library; quote | — | quote | | |
| T6 neighbouring byte writes composed into word writes (FAR) | isolated pass (store composition) | `wpack` (both bytes of one word) | -18 000 (-15 000 to -21 000) | -14 500 | -9 000 / -7 250 |
| | | `wpair` (bytes of two arrays, composed by `xba`) | -17 000 (-14 000 to -21 000) | -13 500 | -8 500 / -6 750 |
| T7 one width per region, no `sep/rep` per byte | isolated pass (width regions; the 16-bit index arithmetic moves to X, as an 8-bit A forces) | `btab` | -9 500 (-8 000 to -11 000) | -7 500 | -4 750 / -3 750 |
| T8 jump table for a dense `switch` | delivered (L2); quote | — | quote | | |
| T9 the X/Y colour | allocation (delivered, lot XY); quote | — | quote | | |
| T10 free convention between `static` functions of a file | call convention, pays with the frames (L4); quote | — | quote | | |

Expected reading (hypothesis): T1 is small on the index forms and pays mainly where the structure arrives as a pointer; T4,
T6, T7 are large on their rows because each row is built around the shape, which says nothing yet about a game's busy time.

## References (every figure names its commit and its switches)

- **ours**: cc65816 built at `g1-r1` (`make clean && make`; compiler, library, templates and `make/` of `develop` `3139a9ef`),
  no `QBE_*` variable (`run.py`'s `env_for([])`). luna `1.44.0+c14ce34` (the pinned `v1.44.0`).
- **the rows**: `devtools/g1bench/tech.{c,asm,mk}` and the runner `tech.py`, committed in `1fc61a55` as measured (the run
  was taken with these files on top of `0c63f42b`, the commit `r1/tech.json` names); a second full run gave identical
  figures and checksums, both speeds, every row.
- **the control**: each `_cp` row (the compiler's text copied, labels renamed) reads its C row within 3 master cycles
  (refresh phase), both speeds; the C rows read R0's figures (`mobaos` 15 441 / 13 068, `btab` 34 634, `mul` 26 234,
  `fixed` 93 277 within 3). Every pass has its C row's checksum, both speeds.

## Measured (2026-11-15), the hand passes

Master cycles a call; the gain is the pass against `_cp`.

| technique | row | `_cp` slow | pass slow | gain slow | `_cp` fast | pass fast | gain fast | bytes `_cp` -> pass |
|---|---|--:|--:|--:|--:|--:|--:|--:|
| T1 | `mobaos` (D walks the entities) | 15 444 | 14 238 | **-1 206 (-7.8 %)** | 13 070 | 12 165 | -905 (-6.9 %) | 112 -> 101 |
| T1 | `mobptr` (D = the pointer) | 29 407 | 29 334 | **-73 (-0.2 %)** | 24 625 | 24 633 | +8 (0.0 %) | 203 -> 198 |
| T1 | `mobfp` (D = the `Mob *` argument) | 31 221 | 30 454 | **-767 (-2.5 %)** | 26 594 | 26 264 | -330 (-1.2 %) | callee only changed |
| T4 | `mul` typed 8 x 8, `nop` waits | 26 234 | 14 664 | **-11 570 (-44.1 %)** | 24 035 | 12 396 | -11 639 (-48.4 %) | 78 -> 80 |
| T4 | `mul` typed, slack used | 26 234 | 11 630 | **-14 604 (-55.7 %)** | 24 035 | 9 794 | -14 241 (-59.3 %) | 78 -> 67 |
| T4 | `fixed` s16 x u16 -> 32 inline | 93 277 | 55 298 | **-37 979 (-40.7 %)** | 78 994 | 46 835 | -32 159 (-40.7 %) | 223 -> 296 |
| T6 | `wpack` | 36 257 | 16 344 | **-19 913 (-54.9 %)** | 30 430 | 13 815 | -16 615 (-54.6 %) | 56 -> 31 |
| T6 | `wpair` | 38 325 | 19 732 | **-18 593 (-48.5 %)** | 31 974 | 16 545 | -15 429 (-48.3 %) | 59 -> 35 |
| T7 | `btab` | 34 634 | 24 480 | **-10 154 (-29.3 %)** | 28 085 | 20 170 | -7 915 (-28.2 %) | 102 -> 72 |

Against R0's hand yardsticks (SlowROM): `mul` 3.38 -> 1.50 with T4 (slack), `fixed` 8.91 -> 5.28 (the rest is the 32-bit
`>> 8` done as eight `ror` pairs, not a technique of this list), `btab` 2.31 -> 1.63, `mobaos` 1.45 -> 1.34. `mobfp` (the
pointer parameter) costs 2.02 times `mobaos` for the same work (31 221 / 15 441): R0's addendum reproduced on a neutral
row; T1 takes 2.5 % of it.

## Predicted against measured (slow / fast)

| technique, row | predicted (point) | threshold (half) | measured | held? |
|---|--:|--:|--:|---|
| T1 `mobfp` | -2 400 / -1 800 | -1 200 / -900 | -767 / -330 | **under prediction** |
| T1 `mobaos` | -1 100 / -850 | -550 / -425 | -1 206 / -905 | yes |
| T1 `mobptr` | -300 / -250 | -150 / -125 | -73 / +8 | **under prediction** |
| T4 `mul` `nop` | -9 500 / -8 000 | -4 750 / -4 000 | -11 570 / -11 639 | yes (above) |
| T4 `mul` slack | -11 000 / -9 500 | -5 500 / -4 750 | -14 604 / -14 241 | yes (above) |
| T4 `fixed` | -35 000 / -30 000 | -17 500 / -15 000 | -37 979 / -32 159 | yes |
| T6 `wpack` | -18 000 / -14 500 | -9 000 / -7 250 | -19 913 / -16 615 | yes |
| T6 `wpair` | -17 000 / -13 500 | -8 500 / -6 750 | -18 593 / -15 429 | yes |
| T7 `btab` | -9 500 / -7 500 | -4 750 / -3 750 | -10 154 / -7 915 | yes |

What did not hold, plainly: **T1 on the pointer forms**. The prediction priced `lda.l $00000k,x` -> `lda.b k` at two ROM
bytes (16 / 12 master cycles) and forgot that a direct page whose low byte is not zero adds one CPU cycle to every `.b`
access (6 master cycles); entities of 12 bytes put D off a page boundary almost always. Each access gains 10 (slow) / 6
(fast), and each direct-page temp that must go absolute while D is moved loses 8 / 6: on `mobptr`, whose loop control and
spills are all `tcc__lf`, the two cancel; on `mobfp` a third is left. On `mobaos` the index form already paid that idle
cycle (`abs,x` with a 16-bit index), so the byte saved is net and the prediction held. The port's -2.6 % on its own bench
is the order of `mobfp`'s -2.5 % (slow).

**T1 is not dropped** (step 3, the L4 lesson). Its dependency, to measure combined in R3: **the function's direct-page temps
leave the page** (into A / X / Y, or re-read as an entity field `.b` rather than spilled), i.e. T1 with the allocator of the
register bank; the measure is `mobfp` and `mobptr` with D on the entity and no `tcc__lf` in the body. Read on the text
(hypothesis, not measured): on `mobptr` that removes about 12 absolute temp accesses an active entity, 1 000 to 1 500
master cycles a call slow, T1 then in the 3 to 5 % range; the pointer parameter itself (x2.02) is the call boundary's, not T1's.

## The technique table

| technique | kind | already measured (figure, piece, date) | measured here (gain on its row, slow / fast) | who does it |
|---|---|---|---|---|
| T1 direct page on the entity | allocator (D is in the bank; the temps must leave the page) | a port by hand: 5 080 against 5 217 master cycles an entity at 20 entities (-2.6 %), its own bench (`reviews/2026-11-14_retrospective_generator.md`) | `mobaos` -7.8 / -6.9 %; `mobfp` -2.5 / -1.2 %, `mobptr` -0.2 / 0.0 %: **under prediction**, dependency above | nobody; R3 |
| T2 far pointer as `[dp]`, 16-bit step | in our generator (an allocation decision delivered as a pass) | lot E1, `b48ef350` (2026-11-14): `unpack` -53.4 %, `slots` -21.0, `col` -18.0, `ents` -10.4 %; game 1 -0.08 %, game 2 -0.25 % of busy time (`E1_far_pointer.md`); R0: `QBE_NO_FARPTR_DP` alone +2.6 % on the first twelve rows | not re-measured | ours (`QBE_NO_FARPTR_DP` turns it off) |
| T3 first argument in A | allocator (calling convention) | A1 S0, `0d9670bc` (2026-11-14), a census priced per site: 0.90 % (game 1) / 0.97 % (game 2) of busy time, under the 1.5 % rule, stopped (`A1_s0_count.md`) | not redone | nobody (stopped) |
| T4 hardware multiplier typed, slack used; short s16 x u16 -> 32 | isolated pass (selection); the slack is scheduling | helpers wait exactly 8 / 16 cycles, `6b643eed` (2026-10-12); zero partial products skipped, -25 % on 6 400 products, `2f14bdd6` (2026-10-27); a game's 32-bit multiply 11.6 % of one routine (retrospective) | `mul` typed -44.1 / -48.4 %, with slack -55.7 / -59.3 %; `fixed` -40.7 / -40.7 % | ours: the helpers only; the typed operation in place: nobody (a pass of today's generator could, outside NMI code: the helper tests `in_nmi_ctx` at run time, an inline unit cannot) |
| T5 `mvn` stub for copies | library | `e77a1906` (2026-11-11): `memcpy` 256 bytes 24 376 -> 15 350 (-37 %), 64 bytes -27 %, 4 bytes +3.6 % (the short path stays) | quote | ours (library) |
| T6 neighbouring byte writes into word writes (FAR) | isolated pass (store composition) | a game's hand rewrite in C, 1 890 524 -> 974 464 on one row (-49 %) (retrospective) | `wpack` -54.9 / -54.6 %, `wpair` -48.5 / -48.3 % | nobody in the compiler (the games, by hand) |
| T7 one width per region | isolated pass (width regions) | R0: `btab` 2.31 / 2.33 over the hand | `btab` -29.3 / -28.2 % | nobody (today's `BYTE_WORD` rule does not take this row) |
| T8 jump table, dense `switch` | in our generator | lot L2, `3450c401`, qbe `a0c9619` (2026-11-12): game 2 -1.73 %, game 1 -0.08 %, bench switch -24.6 % (`L2_jump_table.md`); R0: `QBE_NO_JUMP_TABLE` `state` +10.3 % | quote | ours |
| T9 X/Y colour | allocator (delivered) | lot XY, `bcb0f551` (2026-11-14), against `develop` `fc63dc0a` with `QBE_NO_XY_COLOUR=1`: game 1 -1.72 %, game 2 -1.69 %; leaf A 0.0 %, leaf B 20.3 % of the gap (`G1_ir_probe.md`); R0: the xy family +2.6 % on the first twelve rows | quote | ours |
| T10 free convention between `static` functions of a file | convention with the frames (L4) | L4 S3, qbe `a4dcb1b` branch `l4s23`, `SFRAME := 1` (2026-11-14): +0.54 % (game 1, slower) / -1.06 % (game 2) against -1.07 / -1.31 % required, stopped (`L4_s3_result.md`); pays with S4 | quote | nobody (stopped at S3) |

What the table says (the busy-time share of each shape in a game is not measured here):
- T4, T6 and T7 are **isolated passes**, large on the rows built around their shape (28 to 59 %); none needs a new
  allocator. They are retouch candidates for today's generator as much as for G1, which bears on the engagement rule:
  the prototype is judged against today's generator, and what an isolated pass gives there is taken from G1's margin.
- T1, T3, T9, T10 are the allocator / convention side: only T9 is measured positive on the games (-1.7 %), T3 and T10
  stopped near 1 %, T1 depends on the temps leaving the page.
- A bench gain does not transfer (E1: -53 % on `unpack`, -0.25 % on game 2): the share of each shape in each leaf is
  what R3's prediction lacks (below).

## Rewritten prediction for R3 (hypothesis, from the table and R0; not a measure)

Inputs: the leaves over the hand today (lot XY, `G1_ir_probe.md`): **leaf A 1.29** (16 367 410 / 12 707 660; its gap is the
allocation of A and 32-bit accumulators), **leaf B 1.45 alone, 1.29 with its caller**. R0: every switch off reads 6.07 (2.9
times the base), so R3 keeps the rules it does not subsume (verdict point 2); our formulation gap is 1.05 (structures by
index against field arrays), so formulation gives no room on index forms, the room is the pointer parameter (x2.02,
`mobfp`) and the function per entity (x1.51); inlining is inert on the function-per-entity shape, so the boundary is left
to the convention.

| leaf | today | R3, ABI kept | R3, free convention | within 1.1? | within 1.2 (the stop)? |
|---|--:|--:|--:|---|---|
| leaf A (no call) | 1.29 | 1.18 to 1.24 (allocation of A, accumulators in direct-page pairs; T4 if it multiplies) | the same (a leaf alone has no boundary) | **no** | borderline |
| leaf B alone | 1.45 | 1.28 to 1.36 (T7 / T1 shapes if it has them; allocation) | the same | **no** | **no** |
| leaf B with its caller | 1.29 | 1.20 to 1.25 | 1.12 to 1.18 (parameter in registers or as an index) | **no** | ABI kept borderline; free convention yes |

The written prediction: **condition 1 (within 1.1 of the hand on both leaves) is not reached in either column** with the
techniques measured here; the free convention counts only where a leaf is read with its caller. What would change it: a
leaf whose hot shape is T4 / T6 / T7 (28 to 59 % on their rows), which needs before R3 a census of each leaf by shape
(multiplies, neighbouring byte stores, byte regions, pointer parameters) on the rebuilt game, by its own session.

## Replay

```
make clean && make                                         # at g1-r1
export LUNA_BIN=<the pinned luna v1.44.0>
python3 devtools/g1bench/tech.py --json out.json           # the technique rows, both speeds: the tables above
python3 devtools/g1bench/tech.py --report devtools/g1bench/r1/tech.json   # the saved run of this note
python3 devtools/g1bench/run.py --config base              # R0's rows (tech's C rows read the same figures)
```
Each pass is a section `k_<row>_t<n>` of `devtools/g1bench/tech.asm`, with what changed written on top of it; the `_cp`
sections are the compiler's text (`build/g1bench/r1/tech/slow/tech.c.asm` after a run) with the labels renamed.

## Checkpoint verdict of the maintainer session (2026-11-15)

Replayed by me on the branch `g1-r1` (`devtools/g1bench/tech.py`, luna 1.44.0, `develop` `3139a9ef`'s compiler): the figures of `mul`, `fixed`, `btab`, `wpack`, `wpair`, `mobaos`, `mobptr`, `mobfp` equal the pair's to the cycle (or within the 3 of the `_cp` control). The prediction note was committed alone before any run (`0c63f42b`).

What it settles:
1. **Three of the techniques are isolated passes a generator could take without a new allocator**: a multiply typed to its operands with the wait used for work (T4: -44 to -59 % on `mul`, -41 % on `fixed`), neighbouring byte stores composed into word stores (T6: -48 to -55 %), one width per region instead of `sep/rep` per byte (T7: -29 %). They are bench rows, hand-written, equal checksums; their share of a game's busy time is **not** measured, and T4 inline cannot be used in code the NMI reaches (the library helper tests it at run time).
2. **The direct page on the entity (T1) is worth much less than its first hand figure** except where the indexed form already paid the unaligned-D cycle: -7.8 % (`mobaos`), -2.5 % (`mobfp`), -0.2 % (`mobptr`); each field access gains 10 / 6 master cycles, each direct-page temporary forced absolute while D is moved loses 8 / 6. Marked "under prediction", not dropped: it depends on the allocator keeping the function's temporaries off the direct page (R3).
3. **The pointer parameter alone costs 2.02 times the indexed form** (`mobfp`): the largest recoverable item of the entity shape, which T1 recovers only 2.5 % of; descending a pointer parameter to an index (the call-boundary work) is what pays.
4. **R3's rewritten prediction does not reach 1.1 in either column** (leaf A 1.18 to 1.24; leaf B with its caller 1.20 to 1.25 ABI kept, 1.12 to 1.18 free convention). With R0 (the debt of 6.07 to re-earn) this is the honest state: **the cheap path first**: T4, T6, T7 and the pointer-parameter descent are lots of the *current* generator (each a pass with a switch, a prediction, the games' census), measured on the games before any prototype of a new one; G1 stays open as the option that must earn what those passes leave, and its line (1.1 of the hand on the leaves AND -10 % of game 2) stays the owner's.

## Share of the games' busy time, as bounds (2026-11-15, the maintainer session; asked by the conductor)

Method (private gauge, reference `develop` `3139a9ef`, no switch, the frozen sources of 2026-11-11, per-label profile of the two games; busy = total minus the cycles the emulator marks idle; the bounds are **upper bounds of the time a pattern could touch, not predictions of a gain**; their product with the bench's relative gain is the ceiling written below):

| technique | what is read | game 1 | game 2 | ceiling of the gain (x the bench's relative gain) |
|---|---|--:|--:|---|
| T4 typed multiply, wait used | self time of the `tcc_*` / `__*` helpers (multiply and divide) | 0.23 % | 1.47 % (multiply 0.74, divide 0.42) | game 1 <= 0.1 %, game 2 <= 0.9 % (x 0.44 to 0.59; the divide is not in T4) |
| T6 + T7 byte stores composed, one width per region | time of the basic blocks that contain a `sep #$20` and a `rep #$20` (a round trip) | <= 2.5 % (20 % of the time is in labels not mapped to a C block: the NMI and library assembly) | <= 1.4 % (8 % unmapped) | game 1 <= 0.7 to 1.4 %, game 2 <= 0.4 to 0.8 % (x 0.29 to 0.55) |
| pointer parameter descended to an index | not readable from the assembly (a 4-byte pointer copied from the caller's frame); asked of the games | ? | ? | ? |
| T1 direct page on the entity | depends on the allocator | — | — | — |

Reading: **no isolated technique alone passes about 1 % of either game's busy time**, the same order as the last cheap lots (about 1 % each); the three together, if their eligible time were fully taken, stay under about 2.5 %. The pointer-parameter descent is the one whose share is unknown and whose bench cost is the largest (x2.02 on its row): its census belongs to the games' sessions. Two limits: the blocks with `sep` hold other instructions, so the share is an upper bound of the pattern's time; the unmapped part (20 % of game 1) is assembly and NMI code no pass of the C generator touches.

### The games' own census (2026-11-15, 02:12, game 1's session, its tools, NTSC with PAL in brackets, upper bounds)

- **Pointer or array parameter**: 1.6 % (1.5) of the busy time in the game's compiled code; **no caller of the hot path passes `&tab[k]`** (0 %): they pass the address of a named constant structure of the same file, and the match loop receives no pointer (global arrays indexed). So the pointer-to-index descent has **no hot-path site in this game**. The library's hand-written sprite placement, which also takes a pointer, is up to 11.8 % (11.1) of the time, itself 10.3 % (9.7): outside any generator, a library lever.
- **Neighbouring byte writes**: 0.35 % (0.37) strict, up to 0.7 % (0.4) with parallel-array copies; its heaviest case, a tile composition into a far buffer, was already rewritten by words by hand (-49 %).
- **Width switch `sep`/`rep`**: 2.5 % (2.0) for a round trip in the block, 1.6 % (1.2) in a loop; the game's own compiled code alone 1.9 % (1.5), the rest two library audio functions; heaviest: the sprite update 0.88 %.
- Not counted: the SDK's assembly functions (sprite placement, VRAM queue, NMI).
Reading (the game's): none of the three forms passes 3 %; the bulk is the call boundary and the library. This agrees with the bound read from the gauge (T6+T7 up to 2.5 % / 1.4 %), and **removes the pointer descent as a lever for game 1** (game 2's census is asked; the structure-pointer form belongs to a port's bench shape, not to the two games' hot path).

### Game 2's own census (2026-11-15, 04:18, its session, its tools, heavy level, frames 4 500 to 7 000, busy 509 M of 893 M master cycles, upper bounds, one scenario)

- **Pointer or array parameter**: 3.8 % of the busy time; strict (`&tab[k]` or `tab+k`, one file) **1.0 %**: two callers only (the history `memmove` 0.65 %, the VRAM queue push 0.4 %); the entities are addressed by a `u16` index into one array per field.
- **Neighbouring byte writes**: 0 % detected (separate arrays per field).
- **Width switch in a loop**: 30 % for the functions whose loop holds a `sep`/`rep` (a very loose bound: the whole function), 56 % for any function containing one; heaviest the entity update list 18.2 % (8 `sep` and 8 `rep` in its loop). Not measured: the share of the instruction, the real cost of the pointer in the game, other levels.

**Correction of my own bound (same day)**: the "blocks with a `sep`/`rep` round trip" bound above (<= 2.5 % / <= 1.4 %) is not an upper bound for a width region that spans several blocks; the tighter figure is the **direct cost of the executed `sep`/`rep`** (visits of each block from the profile's instruction count, times the number of switches in it, times 3 CPU cycles: 18 to 24 master cycles each): **0.40 to 0.53 % (game 1) and 0.26 to 0.35 % (game 2)** of the busy time, ~28 000 and ~36 000 executed switches in the window. So taking away **every** width switch saves at most about 0.3 to 0.5 % directly; what T7 can add beyond (byte operations replaced by word operations, fewer instructions) is not in that figure and is what the game-2 list update (18.2 % with 8 round trips in its loop) would show: to measure on that function, not to assume.

Reading across the two games: pointer descent 0 % of the hot path (game 1) and about 1 % (game 2); neighbouring bytes 0.35 to 0.7 % (game 1) and 0 % (game 2); width switches 0.3 to 0.5 % direct. **Each isolated pass stays at or under about 1 % of a game's busy time.**

## A fifth candidate of the cheap path (2026-11-15, from a port's status-bar case)

**Constant offset folded into the displacement of a long-indexed access** (and `b + r` computed once per block). Standalone reproducer (`FAR u16 tampon[512]; const u16 T[256];` and four `tampon[b + k*8 + r] |= T[w + k]`): the generator recomputes the doubled index for every access (`lda b ; clc ; adc #k ; asl a ; tax ; lda.l T,x`, five instructions for what `lda.l T+2k,x` with one `x = w*2` does), 65 instructions for four words against about 25 expected; the near-array path already folds (`QBE_NO_NEAR_INDEXED`), the long-indexed path does not. A port measured the pattern at **37 % of its busy time in its worst scene** (125 051 of 339 005 master cycles a frame, 33 % after a C rewrite that keeps X), 0 elsewhere; the neutral bench line it gave (`vF`, 32 instructions a row against 69) is the target of the pass. Class: an isolated pass of the current generator, no allocator; prediction to write before the lot: about 15 to 6 or 7 instructions a word, about 486 to 250 master cycles a word. Not started (the generator lots wait for the owner's A/B); the games' two sessions are to say how much of their time is in this pattern.

## A sixth candidate (2026-11-15, a port's scrolling ring): the carry into the bank byte of a runtime 24-bit pointer

`p[i]` through a `const u16 *` chosen at run time costs about 16 instructions per index addition: the generator forms the carry into the bank byte with `asl a`, then `lda r0 ; xba ; and #$FF ; lsr` seven times and a 24-bit add, at every access (`*q++` does not change it). A port worked around it by copying its tables to RAM and reading them as named arrays (`lda.l t,x`): 256 000 to 199 000 master cycles a step (-22 %, 100 steps). Candidate: keep the pointer as bank + 16-bit offset and carry only on a wrap (an object never crosses a 64 KB bank: the language already says so for the E1 far-pointer step, see its stated condition: exact only if the pointer is never compared one past the end at `$FFFF`). Census to ask of the games: the share of time in `xba / lsr` index chains. Not started.
