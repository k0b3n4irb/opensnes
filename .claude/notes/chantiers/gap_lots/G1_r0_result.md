# G1 R0 — the bench and its truth (result)

Branch `g1-r0` from `develop` `7608299d` (compiler, library, templates and `make/` identical to `3139a9ef`: `git diff --stat
3139a9ef 7608299d -- compiler lib templates make` is empty). Nothing of this enters `develop`.

## Prediction (copied from `G1_generator_prototype.md`, section "Plan update (2026-11-15, conductor's go) and R0", before any measure)

Predictions (hypotheses, written before the run): (a) the new base reads **2.15 within 3 %** on the first twelve rows (the probe's
figure, `G1_ir_probe.md`); (b) the other compiler's figure in the same unit is **within 10 % of 2.56**, the sign of its six better
rows unchanged (`unpack`, `call`); (c) the structure form and the field-array form of the entity loop differ by **1.3 to 1.6** (a
port measured 1.33 and 1.61 at N = 20); (d) switching off every text rule at once raises the twelve-row mean from 2.15 to **2.6 to
3.4**, and no single family accounts for more than a third of the rise. Stop: if (a) is missed by more than 5 % nothing after item 2
is read until the cause is named.

Measures follow in the next commits.

## References (every figure below names its commit and its switches)

- **ours**: cc65816 built in this worktree, `make clean && make` at `g1-r0` (`develop` `7608299d` plus this note and
  `devtools/g1bench/`; the compiler, library, templates and `make/` are those of `3139a9ef`, the commit the plan names).
  No `QBE_*` variable in the environment unless the row says so. luna `1.44.0+c14ce34` (the pinned `v1.44.0`).
- **the other 65816 compiler**: its Release 2, model "tiny" (`-mtiny -O=-1 -speed -unsigned-char -mask-opt -snes-muldiv`, the
  switches its own driver passes), and the same with `-inline-size=0` ("noinl"); built and run privately with the private
  folder's scripts. Only figures here.
- **hand**: the assembly twins (`devtools/gapbench/kernels.asm`, `devtools/g1bench/forms.asm`), measured in our ROM.
- **the old reference** of 2.20 / 2.56: our C at `develop` `927b8f20` (2026-11-13), no switch; the other compiler as above.

## Item 1 — the method that gave 2.56

Read in the private scripts: the 2.56 is **master cycles measured on luna of the built ROM**, the method of `gapbench/run.py`
itself (the same driver loop rebuilt for the other assembler, 16 calls between `bench_mark_a` and `bench_mark_b`, minus the
empty routine, checksums of `bench_sum` compared with ours), **SlowROM only**, each row divided by our hand twin's figure. It is
**not a static cost weighted by block counters**: point 5 of the 2026-11-14 contest is wrong on that premise (the 2026-11-13
contest's point 3 had proposed a static weighted cost; the measure actually made was the luna one).
Re-run at the present state: the other compiler's ROM rebuilt today is **byte-identical** to the ROM of 2026-11-13 (`cmp`), and
its 24 rows read the **same master cycles and checksums** on luna 1.44.0 (both models, tiny and noinl). The first twelve rows of
`gapbench/kernels.c` are unchanged since that build (only `callst` / `chainst` were added). So every move between the old table
and today's is in our column.
FastROM for the other compiler: not measured (its link script places code in bank `$00` at `$8000` and its startup does not set
`MEMSEL`: a FastROM figure would need its startup and link script changed, outside a private bench of its release).

## Item 2 — the new base (ours, no switch), and the two neutral families

All checksums agree, both speeds (31 rows: gapbench's 26 and the 5 forms rows). The harness's `base` build of the gapbench ROM
is byte-identical to `gapbench/run.py`'s own build (`cmp`, both speeds), and its figures are gapbench's to the cycle;
`gapbench/run.py --check` passes (nothing above `baseline.json`).

Geometric mean of C / hand, first twelve rows: **2.094 SlowROM, 2.115 FastROM** (weighted 1.850 / 1.860; every main row
2.176 / 2.194). Master cycles a call:

| row | C slow | hand slow | C/hand slow | C fast | hand fast | C/hand fast | C bytes |
|---|--:|--:|--:|--:|--:|--:|--:|
| copy | 22236 | 13842 | 1.61 | 18522 | 12236 | 1.51 | 25 |
| fill | 32380 | 16177 | 2.00 | 26536 | 13445 | 1.97 | 32 |
| oam | 41486 | 22893 | 1.81 | 34334 | 18937 | 1.81 | 336 |
| col | 13178 | 6452 | 2.04 | 11206 | 5412 | 2.07 | 89 |
| row | 12848 | 6454 | 1.99 | 11008 | 5412 | 2.03 | 87 |
| box | 17992 | 12408 | 1.45 | 15146 | 10298 | 1.47 | 207 |
| fixed | 93277 | 10474 | 8.91 | 78994 | 8772 | 9.01 | 223 |
| mul | 26234 | 7772 | 3.38 | 24035 | 6398 | 3.76 | 78 |
| ents | 25320 | 14740 | 1.72 | 20976 | 12344 | 1.70 | 169 |
| call | 64494 | 32642 | 1.98 | 56115 | 27880 | 2.01 | 74 |
| soa | 36377 | 24865 | 1.46 | 30624 | 20809 | 1.47 | 286 |
| glide | 93704 | 67682 | 1.38 | 78638 | 56049 | 1.40 | 242 |
| slots | 21928 | 7486 | 2.93 | 17990 | 6194 | 2.90 | 75 |
| state | 19566 | 16161 | 1.21 | 15878 | 13185 | 1.20 | 297 |
| fptr | 52486 | 16628 | 3.16 | 45111 | 14132 | 3.19 | 98 |
| blocks | 24412 | 10673 | 2.29 | 20816 | 8967 | 2.32 | 200 |
| probe | 29955 | 21351 | 1.40 | 25358 | 17932 | 1.41 | 135 |
| chain | 83210 | 37106 | 2.24 | 71777 | 31857 | 2.25 | 65 |
| sprites | 93266 | 51216 | 1.82 | 77894 | 42647 | 1.83 | 545 |
| unpack | 281071 | 51738 | 5.43 | 246234 | 44475 | 5.54 | 225 |
| mobaos (form) | 15441 | 10640 | 1.45 | 13068 | 9067 | 1.44 | 112 |
| mobptr (form) | 29410 | 10640 | 2.76 | 24628 | 9067 | 2.72 | 203 |
| mobsoa (form) | 14661 | 10640 | 1.38 | 12450 | 9067 | 1.37 | 108 |
| mobsoaf (form) | 22162 | 10640 | 2.08 | 19204 | 9067 | 2.12 | 43 |
| btab (form) | 34634 | 15004 | 2.31 | 28085 | 12078 | 2.33 | 102 |

(The variants `fillu coli rowi entsi callst chainst` are in `devtools/g1bench/r0/families.json`, unchanged from gapbench's
README.) The forms (`devtools/g1bench/forms.c`, written for this step, nothing of a game): `mob` is 20 entities of six words
(move, bounce on two bounds, count down, lose a life), 16 passes; `mobaos` an array of structures by index, `mobptr` a pointer
walk over the same structures (`for (m = mob; m < mob + NM; m++)`), `mobsoa` one array per field with the loop inline,
`mobsoaf` the same with a (non-static) function per entity on a `u16` index; one hand yardstick on the field arrays. `btab`: 32
objects, a frame index each, four bytes of a `const` table (ROM, read long) written to four byte arrays.

**Formulations** (C / C, same work, same checksum):

| pair | SlowROM | FastROM |
|---|--:|--:|
| structures by index / field arrays (`mobaos / mobsoa`) | **1.053** | 1.050 |
| pointer walk / field arrays (`mobptr / mobsoa`) | 2.006 | 1.978 |
| pointer walk / structures by index (`mobptr / mobaos`) | 1.905 | 1.885 |
| function per entity / inline (`mobsoaf / mobsoa`) | 1.512 | 1.542 |

Read in the emitted assembly (`build/g1bench/base/forms/slow/forms.c.asm`, `k_mobptr_c`), not measured apart: the pointer walk
keeps the 32-bit pointer in `tcc__lf`, compares bank then offset each turn into a 0 / 1 value then `cmp #0`, reloads X before
each field and reads every field `long,x` (`lda.l $00000A,x`); the index forms read `abs,x`.

## Item 3 — three generators, SlowROM (master cycles a call; the hand figure is ours, the same for the three)

| row | hand | ours | other | other noinl | ours/hand | other/hand | other noinl/hand | other/ours |
|---|--:|--:|--:|--:|--:|--:|--:|--:|
| copy | 13842 | 22236 | 46169 | 46169 | 1.61 | 3.34 | 3.34 | 2.08 |
| fill | 16177 | 32380 | 66168 | 66168 | 2.00 | 4.09 | 4.09 | 2.04 |
| oam | 22893 | 41486 | 55250 | 55250 | 1.81 | 2.41 | 2.41 | 1.33 |
| col | 6452 | 13178 | 16801 | 16798 | 2.04 | 2.60 | 2.60 | 1.27 |
| row | 6454 | 12848 | 16207 | 16207 | 1.99 | 2.51 | 2.51 | 1.26 |
| box | 12408 | 17992 | 22493 | 22493 | 1.45 | 1.81 | 1.81 | 1.25 |
| fixed | 10474 | 93277 | 74452 | 74455 | 8.91 | 7.11 | 7.11 | 0.80 |
| mul | 7772 | 26234 | 28826 | 28823 | 3.38 | 3.71 | 3.71 | 1.10 |
| ents | 14740 | 25320 | 28794 | 28794 | 1.72 | 1.95 | 1.95 | 1.14 |
| call | 32642 | 64494 | 27335 | 47359 | 1.98 | 0.84 | 1.45 | 0.42 |
| soa | 24865 | 36377 | 56173 | 56173 | 1.46 | 2.26 | 2.26 | 1.54 |
| glide | 67682 | 93704 | 139846 | 139848 | 1.38 | 2.07 | 2.07 | 1.49 |
| slots | 7486 | 21928 | 21206 | 21209 | 2.93 | 2.83 | 2.83 | 0.97 |
| state | 16161 | 19566 | 37756 | 37754 | 1.21 | 2.34 | 2.34 | 1.93 |
| fptr | 16628 | 52486 | 56937 | 56937 | 3.16 | 3.42 | 3.42 | 1.08 |
| blocks | 10673 | 24412 | 41083 | 41086 | 2.29 | 3.85 | 3.85 | 1.68 |
| probe | 21351 | 29955 | 41909 | 41909 | 1.40 | 1.96 | 1.96 | 1.40 |
| chain | 37106 | 83210 | 57153 | 65594 | 2.24 | 1.54 | 1.77 | 0.69 |
| sprites | 51216 | 93266 | 117225 | 117225 | 1.82 | 2.29 | 2.29 | 1.26 |
| unpack | 51738 | 281071 | 215892 | 215892 | 5.43 | 4.17 | 4.17 | 0.77 |
| mobaos (form) | 10640 | 15441 | 44801 | 44801 | 1.45 | 4.21 | 4.21 | 2.90 |
| mobptr (form) | 10640 | 29410 | 22142 | 22142 | 2.76 | 2.08 | 2.08 | 0.75 |
| mobsoa (form) | 10640 | 14661 | 25270 | 25273 | 1.38 | 2.38 | 2.38 | 1.72 |
| mobsoaf (form) | 10640 | 22162 | 25273 | 24770 | 2.08 | 2.38 | 2.33 | 1.14 |
| btab (form) | 15004 | 34634 | 46406 | 46406 | 2.31 | 3.09 | 3.09 | 1.34 |

Every row of the other compiler has our checksum (the forms included). Geometric means of x / hand:

| | ours | other | other noinl |
|---|--:|--:|--:|
| first twelve rows | **2.094** | **2.558** | 2.678 |
| twenty main rows | 2.176 | 2.599 | 2.690 |
| five forms rows | 1.927 | 2.734 | 2.724 |

Rows where the other compiler is faster than ours today: `call` 0.42, `chain` 0.69, `unpack` 0.77, `fixed` 0.80, `slots` 0.97,
and the pointer walk `mobptr` 0.75. `ents` has turned (0.94 on 2026-11-13, 1.14 today) and `slots` nearly (0.66 to 0.97), by our
lots since `927b8f20`, not by the other column. The other compiler reads its field arrays and its pointer walk at about the same
cost (25 270 / 22 142) and its structures by index at 1.77 times its field arrays: the formulation penalty does not have the
same shape in the two generators.

## Item 4 — the starting debt (ours, switches off by family)

Every `QBE_NO_*` of `compiler/qbe` (45, `grep -rhoE 'QBE_NO_[A-Z0-9_]+' compiler/qbe`; `run.py --list` checks none is left
out), grouped:
- **peephole** (the text rules): `PEEPHOLE PEEP_FLOW PEEP_WIDE PEEP_AND2 PEEP_AFLOW PEEP_LOOPY PEEP_LOOPX PEEP_XFLOW
  PEEP_FWDSTORE PEEP_SLOTDEAD PEEP_DECLOOP PEEP_FUSE PEEP_FUSE_SRC PEEP_R9LOOP PEEP_ENTRY PEEP_LATE`
- **xy** (X / Y colour): `XY_COLOUR X_CACHE`
- **dp** (direct-page temps, frames, slots): `DP_TEMPS DP_FRAME LEAF_SPLIT SLOT_COLOR PHI_SHARE OPND_SHARE KL_SHARE
  COPY_SHARE NARROW_KL NONLEAF_ALIAS RETSTORE_ALL FARPTR_DP`
- **near** (near / absolute addressing): `NEAR_ABS NEAR_INDEXED NEAR_FIELD NEAR_DEREF`
- **inline**: `AUTO_INLINE`; **jumptable**: `JUMP_TABLE`; **loops**: `IVAR REASSOC_LOOP SINK_LOAD`
- **others**: `BYTE_WORD LOAD_ADD_FUSE MUL_HIGH_DEAD ROTATE REASSOC SHORT_BRANCH`

All 31 rows keep their checksums in every configuration below, both speeds (no row without a figure). The `base` figures were
taken in three separate runs and are identical to the cycle.

| config | first twelve slow | vs base | first twelve fast | vs base | largest row loss, any row (slow) | largest among the twelve (slow) |
|---|--:|--:|--:|--:|---|---|
| base | 2.094 | | 2.115 | | | |
| peephole | 2.597 | +24.1 % | 2.634 | +24.5 % | +70.1 % (unpack) | +53.7 % (col) |
| xy | 2.147 | +2.6 % | 2.172 | +2.7 % | +31.2 % (btab) | +9.5 % (ents) |
| dp | 3.762 | +79.7 % | 3.890 | +83.9 % | +233.0 % (unpack) | +204.7 % (col) |
| near | 3.028 | +44.6 % | 3.057 | +44.5 % | +124.8 % (btab) | +105.7 % (soa) |
| inline | 2.094 | 0 | 2.115 | 0 | 0 (ROMs byte-identical to base) | 0 |
| jumptable | 2.094 | 0 | 2.115 | 0 | +10.3 % (state) | 0 |
| loops | 2.261 | +8.0 % | 2.292 | +8.4 % | +149.6 % (fillu) | +49.0 % (fill) |
| others | 3.738 | +78.5 % | 3.791 | +79.3 % | +468.6 % (fillu) | +196.0 % (fill) |
| **all** | **6.065** | **+189.7 %** | 6.263 | +196.1 % | +542.6 % (fillu) | +407.9 % (copy) |
| all but branch shortening | 6.036 | +188.3 % | 6.233 | +194.7 % | +542.6 % (fillu) | +404.3 % (copy) |

Share of the rise (base to all, +3.971 on the first twelve, SlowROM) taken by each family alone: dp 42.0 %, others 41.4 %, near
23.5 %, peephole 12.7 %, loops 4.2 %, xy 1.3 %, inline and jumptable 0: they sum to 125 %, the families are not additive.
`inline` changes nothing on these rows (no call site meets the inlining rule: every callee is visible or has two sites).

Single switches, to name what is inside the two largest families (first twelve, SlowROM, against 2.094): `SHORT_BRANCH` 3.496
(+67.0 %), `SLOT_COLOR` 3.156 (+50.7 %), `DP_FRAME` 2.571 (+22.8 %), `PHI_SHARE` 2.440 (+16.6 %), `NARROW_KL` 2.366 (+13.0 %),
`BYTE_WORD` 2.259 (+7.9 %), `ROTATE` 2.178 (+4.0 %), `MUL_HIGH_DEAD` 2.168 (+3.5 %), `FARPTR_DP` 2.148 (+2.6 %), `NEAR_ABS`
2.146 (+2.5 %), `DP_TEMPS` 2.103 (+0.4 %), `REASSOC` 2.098 (+0.2 %, but `fillu` +149.6 %); `LOAD_ADD_FUSE`, `LEAF_SPLIT`,
`NONLEAF_ALIAS`, `RETSTORE_ALL` 0 on these rows. `PEEPHOLE` alone equals the whole peephole family (2.597).

**Byte-identical sanity**: a clone of `develop` `7608299d` built from scratch outside this worktree (`make`), against this
worktree's build: the 65 example ROMs are `cmp` identical (65 compared, 0 differ). The compiler binaries themselves differ
(build paths); their output does not.

## Predictions against measures

| | predicted | measured | held? |
|---|---|---|---|
| (a) new base, first twelve rows | 2.15 within 3 % | 2.094 SlowROM (-2.6 %), 2.115 FastROM (-1.6 %) | **yes** (stop rule not triggered) |
| (b) other compiler, same unit | within 10 % of 2.56; sign of its six better rows unchanged (`unpack`, `call`) | 2.558 (-0.1 %); `unpack` 0.77 and `call` 0.42 still better; of the six, `ents` turned (1.14), `slots` 0.97, `chain` 0.69, `fixed` 0.80 | **yes** for the figure and the two named rows; **no** for all six (`ents` turned) |
| (c) structures vs field arrays | 1.3 to 1.6 | 1.053 / 1.050 (by index); the pointer walk is 2.01 / 1.98 times the field arrays | **no** |
| (d) every text rule off | 2.6 to 3.4, no family above a third of the rise | 6.065 (6.036 keeping branch shortening); dp 42 % and others 41 % of the rise | **no**, both parts |

What did not hold, plainly:
- (c): on this generator the array of structures read by index costs only 5 % more than the field arrays; the expensive
  formulations are the **pointer walk** (x2.0) and the **function per entity** (x1.5). The port's 1.33 / 1.61 were not
  reproduced by this neutral loop; which formulation the port called "structure" is not known here (hypothesis: a pointer form).
- (d): the debt is about **2.9 times** the base, not 1.2 to 1.6: what today's generator owes to its switches is mostly the
  direct-page slot work (`SLOT_COLOR`, `DP_FRAME`, `PHI_SHARE`, `NARROW_KL`) and branch shortening, not the text peephole (24 %
  alone). A new allocator that starts from the IR without them starts near 6.0 and has those slot decisions to earn back before
  it gains anything (input for R3's re-estimate; not a verdict on R3).
- The premise of the contest's point 5 (the 2.56 a static weighted cost) is false: it was luna master cycles, SlowROM.

## Replay

```
make clean && make                                        # at g1-r0
export LUNA_BIN=<the pinned luna v1.44.0>
python3 devtools/gapbench/run.py --check                  # gapbench's own table; nothing above baseline.json
python3 devtools/g1bench/run.py --config base             # item 2 table and formulations
python3 devtools/g1bench/run.py --config base,peephole,xy,dp,near,inline,jumptable,loops,others,all,all_but_branch \
        --json out.json                                   # item 4 (one config per run also works; merge the JSON)
python3 devtools/g1bench/run.py --config base,QBE_NO_SHORT_BRANCH,QBE_NO_SLOT_COLOR    # single switches, any list
python3 devtools/g1bench/run.py --report devtools/g1bench/r0/families.json             # the saved runs of this note
```
The other compiler (item 3): privately, its build script on the gapbench rows and on `forms.c` (the include replaced by the six
typedefs and `FAR`, the driver variables moved to its driver as for the gapbench rows), then its luna runner, model tiny and
tiny with `-inline-size=0`; the three-generator table divides by our hand column of the base run.
Saved runs: `devtools/g1bench/r0/{families,single,all_but_branch}.json` (taken at `bcdbcf46` plus the two config-only edits of
`run.py` committed with this note; luna 1.44.0+c14ce34).

## Checkpoint verdict of the maintainer session (2026-11-15)

Replayed by me on the branch `g1-r0` (code stays there, nothing of G1 enters `develop`; this note does): the base reads 2.094 SlowROM / 2.115 FastROM, the peephole family 2.597, the direct-page family 3.762, as the pair measured; the prediction note was committed alone before any measure (`52bb4c83`).

What R0 settles for the plan:
1. **The base holds** (2.15 predicted, 2.094 / 2.115): R0's stop rule did not fire, and the premise of the 2026-11-13 contest's point 3 (the other compiler's figure as a static weighted cost) is wrong: it is master cycles on the emulator, now rebuilt byte-identical and 2.558 at the present state; **every change in the gap since 2026-11-13 is on our side** (the older reference was our C at `927b8f20`).
2. **The debt is large: 6.07 with every switch off**, 3.76 with the direct-page family off, 3.74 with "others" off, 3.03 with near addressing off; alone, `SHORT_BRANCH` +67 %, `SLOT_COLOR` +51 %, `DP_FRAME` +23 %, `PHI_SHARE` +17 %; the families do not add up (125 %). A generator that replaces these rules starts near 6, not near 3: **G1 cannot be "cut the text rules and allocate better"**; contest point 9 (keep branch shortening) is confirmed and must extend to the direct-page and slot work: the plan keeps the rules the allocator does not subsume, and R3's prediction is written against that.
3. **The formulation gap is not 1.3 to 1.6 with our compiler** (prediction (c) missed: 1.053 / 1.050): structures read by index cost 5 % more than one array per field; the costly forms are the pointer walk (x2.01) and the function per entity (x1.51). The other compiler has the opposite profile (its pointer walk is cheapest, its structures by index 1.77 times its field arrays). The port's 1.33 / 1.61 does not reproduce on neutral rows: either its bench differs by the byte fields and the NMI contract, or its loop body does (to ask, with its `.asm`). Until then the "formulation moves the cost more than an advance" claim stands only for the pointer walk and the call per entity.
4. **Inlining is inert on these rows** (`QBE_NO_AUTO_INLINE` changes nothing): the call boundary of a function per entity is not recovered by today's inliner on this shape, which is where the call-boundary work of R3 (free convention in a file) has room.
5. Prediction (b)'s six better rows did not all hold (`ents` worse than ours at 1.14, `slots` 0.97), the figure did.

Next: R1 as contested (the technique table, "already measured" column first, isolated passes only), with R3's prediction rewritten on points 2 and 4.

### Addendum (2026-11-15, 01:40): where a port's 1.28 for structures comes from

A port that measured its structure form 1.33 to 1.61 above one array per field separated its causes: the byte fields turned into words change it by only 3 %; **the rest (1.28) comes from the pointer passed to the functions**: a structure form whose functions take `Ent *e` and read `e->field` compiles to 26 long-addressed accesses (`lda.l`, X = the 4-byte pointer, copied from the caller's frame, a stack frame opened with `tsa / sec / sbc / tas`), where the array form takes a 16-bit index and reads `lda.w field,x` (162 instructions against 225, no long access). R0's neutral row compared an array of structures read by index with one array per field (1.05) and its pointer walk (x2.01) is of the same family. So: **the cost is the pointer parameter, not the layout**. For G1: a structure pointer parameter whose callers all pass `&ent[k]` of one file could come down to an index (or an offset on a known base), or the direct page placed on the structure (that port's hand measure: 0.97 of the indexed yardstick): the call-boundary work of R3 applied to a pointer parameter, to be written as a technique with its own prediction, and a row of the bench with a function taking a structure pointer.
