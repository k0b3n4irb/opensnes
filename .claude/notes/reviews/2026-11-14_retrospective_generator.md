# Retrospective, January to November 2026, and what it says about a generator designed for the 65816

Written 2026-11-14 at the owner's request, before any G1 code. Sources: the git history (2 238 commits; the public history
starts 2026-01-01, there is no earlier commit to date), `CHANGELOG.md`, `.claude/STRUCTURAL_DEFECTS.md`, the notes under
`.claude/notes/`. Three bounded readings were made (compiler arc, console knowledge, departure from PVSnesLib and method);
the figures below were taken from the notes they cite, and the commit hashes were checked to exist. "Game 1 / game 2" are
the two consumer games, as everywhere in these notes. Neighbours' answers (emulator, corpus, kit, the three games) are added
in a second revision of this note; nothing below depends on them yet.

## 1. Why starting over paid, and when

- The departure was the compiler: tcc816 was replaced by cproc, QBE and a w65816 backend (`30fc652f`, 2026-01-05;
  `docs/MIGRATING_FROM_PVSNESLIB.md`). The original reasons for leaving tcc816 are not written in any note: not claimed here.
- It did not pay at once. On 2026-10-08 the SDK was 17 to 18 % faster in total, and slower on anything that indexes an array
  or follows a pointer (sort +63 %, collide +13 %, list +7 %; `chantiers/beat_pvsneslib.md`). It paid through a staged
  programme (S0 to S7), each stage with an exit criterion. On 2026-11-01: 23 workloads, cycles -63.2 %, code -52.3 %, faster and
  smaller on all 23 (`reviews/2026-11-01_benchmark_pvsneslib_4_6_0.md`, `5eea78f3`).
- One thing the old toolchain did better stayed visible for months: stack depth (every temporary owned a stack slot where
  PVSnesLib uses direct-page pseudo-registers); fixed by the leaf frame in the direct page (`tcc__lf`).

## 2. What worked

| What | Measured | Piece |
|---|---|---|
| A prediction written before the code, and a stop rule | A1 stopped at 0.90 / 0.97 % (bar 1.5 %), L4 after S3, L3 S3 and the `tcc__lf` axis did not open at 3 % | `gap_lots/*` |
| Jump table for a dense `switch` | game 2 -1.73 %, game 1 -0.08 %, bench switch -24.6 % (predicted -3 to -4 %) | `L2_jump_table.md` |
| X/Y colour in the IR | game 1 -1.37 %, game 2 -1.78 %, one leaf -20.3 % (predicted 0.2 %) | `G1_ir_probe.md`, `013189dc` |
| Frameless leaves | benchmark -37.6 % (2153 to 1343) | `tech/compiler_optimizations.md` |
| Hardware multiply and divide waited exactly | 8 and 16 CPU cycles, the helpers had waited 14, 22, 15 | `6b643eed` |
| Skip empty partial products of a 16-bit multiply | -25 % on 6 400 products | `2f14bdd6` |
| Block move for `memcpy` (stub built on the stack) | 7 cycles a byte against 11.5, from 12 bytes | `e77a1906` |
| FastROM addressing of code in the `$80` mirror | the header bit alone changed 32 master cycles in 21.4 M | `a061a2db` |
| A corpus oracle at every stage (ROM, WRAM, audio) with a byte-identical switch per pass | every stage reports N/N MATCH | `beat_pvsneslib.md` |

## 3. What gave nothing, or cost

- **L4 static frames**: correct, dormant, byte-identical on 65 ROMs, and no gain: +0.54 % (game 1) / -1.06 % against bars of
  1.07 / 1.31 %. The `sf.*` operands blinded the text rules (`peephole_xflow`, `lda S / sta S`, entry reload) that had been doing
  the work silently (`L4_s3_result.md`). Frames, convention and peephole are one design.
- **First argument in A**: counted 0.90 / 0.97 %, 1.44 / 1.26 % even with free entry stores (`A1_s0_count.md`): stopped.
- **Widening `tcc__lf`**: predicted 0.64 / 1.14 %, measured 0.00 / 0.24 %: a temporary live after a call on any path stays on
  the stack (static colouring).
- **Far pointers**: four attempts at the dereference; one passed 404/404 by luck (the bank byte truncated by a 16-bit load) and
  was reverted. A 4-byte pointer ABI landed later (`f6a72281`); a far pointer in the direct page is worth about 0.25 % on
  the games, -53 % on its own bench routine (`E1_far_pointer.md`).
- **Inlining**: the unit budget decided a third of game 2's gain (32 KB to 128 KB, `I1_s2_result.md`); a clone label ending in
  a size suffix linked the wrong function in silence (`4188c813`).
- **Source rewrites** (a hand-unrolled fill) got slower (2.53 to 3.03, `c_asm_gap.md` A8).
- A mistaken claim survived five weeks in the canonical docs on one observation (a SuperFX "bug" of another emulator;
  `conventions/feedback_dont_canonicalize_early_observations.md`). A conditional-`jsl` miscompile vanished without a known
  cause and stays open (`archive/cc65816_conditional_jsl_codegen_bug.md`).

## 4. What the console taught (the mechanical rules)

1. The bank is part of every address; plain C RAM lives below `$2000`, `FAR` above; a cast that drops the bank is the one
   silent path (`KNOWN_LIMITATIONS.md`).
2. FastROM: code and data are addressed at `$80+`; MEMSEL alone is not enough.
3. The multiplier is exact (product 8 cycles after `WRMPYB`, quotient 16 after `WRDIVB`), not reentrant: an NMI or IRQ that
   multiplies needs the software path (about 330 cycles worst case) or must not use the unit (`tech/nmi_context_hardware_muldiv.md`).
   A sized multiply (8x8, 16x8) matters more than the general one.
4. A byte read is a word read and a mask, except on registers (`$2137`, `$2139`, `$213A` have effects): `volatile` or a constant
   register address stays 8-bit.
5. The direct page is the register file, and the NMI has its own mirrored window; `$2180-$2183` is not safe in NMI; the
   link walks the call graph from the NMI roots.
6. VRAM, CGRAM and OAM writes outside VBlank or forced blank are lost and shift what follows; the generator cannot see it, a
   counter in the library can.
7. Shared-latch registers (scroll and Mode 7 matrix, MPY) need an ordering rule; vertical scroll is written `y - 1`.
8. SA-1 and SuperFX are separate backends with their own link checks; neither takes compiled C beyond the main CPU's side.
9. An instruction costs the same in shipped assembly and in our C (24 to 28 master cycles); the gap is the **instruction
   count**, its width switches (22 per 1 000 in shipped code against 75 in our C) and its operand mix
   (`commercial_yardsticks.md`).

## 5. What the method taught

- A green test can test nothing: the liveness gate that passed a dead NMI, the DMA check that missed a transfer ending in
  display (`18ed12f4`, `5d275a63`). Every new gate ships with a negative control that must go red.
- Traces of games and benches prove the program, not the compiler: the X/Y probe passed both games and failed 59 of 2 000 fresh
  differential seeds (`G1_ir_probe.md`). Equivalence needs a differential test, a model and fault injection built before the
  optimisation.
- The measured gain of one axis on the games has been 1 to 2 %. Nothing counted alone has reached the 10 % line.
- Silent failure is the SNES's dominant failure: refuse at build and link time, or count at run time, never skip.

## 6. The yardstick: another 65816 compiler

Over the twelve bench routines the geometric mean is 2.56 times the hand for vbcc against 2.20 for ours; vbcc is better on six
rows (`unpack` 0.34 of ours, `call` 0.41); taking the better of both on each row gives 1.99 (`G1_generator_prototype.md`).

## 7. What this says about the generator (the part that counts)

1. **One design, not a pass**: calling convention, frame placement and register bank decided together (L4 failed by retrofit;
   `tcc__lf` stops at the call). A generator that keeps today's convention and frames only re-does the allocator and will
   reproduce the 1 to 2 % results.
2. **X, Y and direct-page words are allocatable classes** with index, counter and compare operand as constraints; the IR colour
   probe already gave -20 % on a leaf.
3. **The games spend 58.9 / 74.4 % of their time in functions that call** (G1-M0): the gain sits in the call boundary (first
   argument in A, stack traffic, `jsl` against `jsr`, inlining under a byte budget), not in leaves alone. The bench shape and the
   game shape differ.
4. **Instruction count and width switches, not instruction price**: one width per region, jump tables, `mvn` for copies, sized
   multiplies, hardware unit as a typed operation with its waits as slack.
5. **The bank is in the type**: far pointer with provenance on every pointer-producing operation, `FAR` objects, FastROM
   addressing; every dropped-bank cast refused.
6. **The console as rules, each measured**: registers `volatile`, 8-bit reads on registers, no hardware multiplier in NMI/IRQ
   context, no `$2180-$2183` in a handler call graph, VBlank accounting stays in the library.
7. **The proof comes first**: the corpus oracle, a byte-identical switch per pass, a differential fuzzer with a model, fault
   injection, a negative control per gate, prediction and stop rule before each step. Without it, any figure is void.
8. **Honest ceiling**: from the counts, no single technique carries the 10 % line; a generator pays only as the sum of
   several (convention + allocation + call boundary + selection), which is why its cost is 7 to 11 days and why it stops
   after the leaves and after a profile-drawn sample.

## 8. Neighbours' answers (revision 2, in progress)

**A 2D action game of about 100 KB of C (first answer, all figures in master cycles per frame step on luna)**:
- The per-entity loops dominate (about 400 000 of 600 000 in a heavy level). The sprite display loop is 85 instructions, 2 236
  per sprite, about 35 by hand. Fields as arrays: a byte costs sign extension, `sep/rep` and a second index; as words the
  step is 14 % shorter (a source-level workaround the generator could make unnecessary).
- A 32-bit multiply weighed 11.6 % on one routine (31 calls a step); the real need is signed 16 by unsigned 16 to 32 bits,
  never signed by signed. Five 16-bit divisions in a housekeeping routine pushed a VBlank DMA late: a divide in a deadline
  routine should be a compile-time diagnostic.
- A cliff: past 16 words of locals a function goes to the stack (a 19-word leaf; fixed on the leaf, -23.7 %).
- Cumulative advances so far on that game: jump table -1.86 %, `tcc__lf` at 20 words -0.69 %, X/Y colour -2.26 %, far pointer in
  the direct page -0.49 %, skipped empty partial products -0.4 %. Retired as no gain: a 16-tile memo, a 16-bit shot, a
  closed form of an angle, a short path of the 32-bit multiply.
- Asks: byte entity fields without the extension cost, a short s16 x u16, a deadline diagnostic for divisions, locals beyond 16
  words in the extended direct page; keep the jump table and the far pointer in the direct page.
- Its own challenge: the gain comes mostly from the shape of the C (-14 %) and from advances of a few percent each, so a
  generator must be worth more than about +3 % on that game to repay its risk (hypothesis, to prove on the same bench).

**The porting kit (what the ports learned from compiled C)**:
- An entity loop over a table at 20 entities: array of structures 11 180, one array per word field 8 419 (-25 %), hand assembly
  5 217: factor 2.14 then 1.61. An assembly iterator that copies the entity to a fixed address for absolute-addressed C made it
  worse (16 541, +96 %).
- Losses the compiler caused the games: a leaf
  that gains a call on a cold branch loses its direct page (+1 800 a frame); a loop that calls, or indexes a `u8` without a
  constant step, is not turned into an index; a variable shift made the peephole liveness give up on the whole function; one
  miscompilation (a counter whose address is taken, a stale copy) lasted four days unseen.
- A bench gain does not transfer: the same lot gave -2.26 % on one game and -0.08 % on the other; a bench gain is read only with the
  `.asm` of the real case. A 1993 commercial game written in assembly holds its step in 2.12 frames: a factor 1.0 to 1.1 would
  be needed to match it, 1.61 is measured.
- Asks: `field,x` on global arrays at the hand's price; loop-body variables kept in registers without copy or reload; an index
  kept when the body calls; integration and direct page that stay stable when the code grows; no 8/16 switch per byte; correctness
  before gain. Its own challenge: judge G1 on the games' hot shapes, not on the bench, and keep correctness as the first criterion.

**The reference corpus (what it settles, and its holes)**:
- Time is not a number of cycles: a memory access costs 6, 8 or 12 master cycles by region and MEMSEL, an internal operation 6; FastROM
  only for banks `$80+`. So the cost model counts master cycles per access, by the region of each access.
- `+1` cycle when m=0 (`+2` for read-modify-write), `+1` when the low byte of D is not zero, `+1` on an indexed page cross; `rep` /
  `sep` 2 bytes 3 cycles (a round trip is 6 cycles, 4 bytes); the multiplier 8 and the divider 16 CPU cycles; DMA 8 master
  cycles a byte plus 8 a channel, DRAM refresh 40 per line; `mvn` 7 per byte.
- Holes, costliest first: the state of the art of 6502-family code generation is absent (llvm-mos, Calypsi, vbcc's 65816 back end,
  the WDC compiler), no reference emulator CPU core, the bus-level step tests only as a README, the 1993 commercial assembler has
  no document (a measure on a ROM). Documents may be captured privately with their licence written in a provenance card.
- Its challenge: "the target is the SNES, not only the 65816" holds (the same access costs 6 or 8 master cycles by placement); the
  5A22 multiplier is an instruction choice of the compiler outside interrupts; only the PPU multiplier (shared with Mode 7)
  stays with the library.

**A port's entity-loop benches (identical step by step to a model; against a 1993 hand-written assembly and a second console's)**:
- Entity loop, one array per word field, one small function per entity on a `u16` index: 8 419 against 5 217 by hand, factor 1.61
  (needs 1.5 or less to change its verdict). What it pays: a `sta.w f,x` then `lda.w f,x`, an `and` stored in the direct page and
  never read, the per-entity call.
- A byte table read through an index, four bytes written to four arrays: 913 against 510 to 540, factor 1.69 to 1.79 (2.7 to 2.9
  earlier in the month). What remains: an `and.w #$00FF` before each byte store, a `sep/rep` per byte instead of staying 8-bit for
  the four, a constant offset recomputed, a direct-page intermediate.
- Formulation weighs as much as an advance: array of structures 11 180 against 8 419 by field (1.33), a grid of pointers 1 447 against
  913 indexed (1.58), an iterator that copies the entity into a global 16 541 (3.17).
- A technique to measure, hypothesis: put the direct page on the entity (`tcd` to its base) and read each field with `lda.b`, so a
  structure stays a structure; to be measured by hand before G1.
- Four invariants a generator must never break: order and number of accesses to console registers (VRAM port, multiplier waits,
  DMA and HDMA registers); size or position of the VBlank code; layout of an array the DMA sends as it is; bank or calling
  convention of a function called from assembly or the NMI.
- Challenges: the 1993 yardstick holds routine by routine, not game by game (a large part of the largest gains is algorithmic and
  belongs to the game); the bench of G1 must count several natural formulations of the same C, or G1 fits our workarounds.

**The emulator (costs measured by a versioned script, `tools/console-costs.py` of the emulator at `d577a67`; master cycles, each figure the
difference of two runs to a program counter)**:
- A CPU cycle costs 6 (internal, fast ROM, registers `$2000-3FFF` and `$4200-5FFF`), 8 (WRAM, slow ROM, SRAM) or 12 (`$4000-41FF`, pads).
  WRAM is always 8, even with FastROM: RAM is slower than fast ROM. `NOP` 14 / 12 (slow / fast ROM); `LDA dp` 24 / 20; `LDA abs` WRAM
  32 / 26; `LDA long` WRAM 40 / 32; `PHA` 22 / 20; `PLA` 28 / 26. `MVN` 7 cycles a byte (52 / 46 master).
- General DMA: 8 a byte whatever the ROM, plus a fixed part (32 on the probe); 256 bytes 2 080, 1 024 bytes 8 224; the CPU is halted.
  DRAM refresh 40 a line, 10 480 of 357 364 a frame (2.9 %). Multiplier right 8 CPU cycles after the `$4203` write (2 NOPs read a
  partial value), divider 16.
- What a bench in instructions gets wrong: a byte of instruction costs a read cycle, so the time of a straight line is in its **size**
  (FastROM saves 2 master cycles per ROM access, 15 to 20 % and no more); DMA, HDMA and refresh steal time the CPU does not see; the
  multiplier and divider are one shared unit (a write while it works clears the result).
- Its challenges: bench in master cycles per window and separate slow and fast ROM; a technique that lengthens the code or moves
  bytes into RAM can lose; the compiler must know the 8 / 16 cycle delay (to schedule useful work in it and guarantee no interrupt
  shares the unit), against the objection that the multiplier stays with the library; the PPU multiplier shared with Mode 7 is not
  modelled by the emulator (a hole: a measurement on a cartridge).
- Limit to keep: these costs are a model faithful to a reference emulator (CPU validated on the public single-step vectors), not a
  capture from a console, and the corpus holds no measurement on hardware. **A gain the generator announces must be confirmed on a
  physical cartridge before it is declared.**

**A sports game (busy time 368 101 master cycles a step NTSC, 392 619 on the PAL 3/4 frame; 340-step reference scenario)**:
- Workarounds written in its C and what they bought: composing by words instead of bytes 1 890 524 to 974 464 (-49 %); a cheap
  reject before the computation 3 500 to 1 100 a pair; a ring in `u16` not `u32` 43 000 to 19 000 a step; glyphs shifted outside the
  console, no variable `>> s` (a menu from 80 to 37 frames); `(x<<2)+(x<<1)` for `x*6` (about 2 400 saved a product); a leaf that
  sets a flag and a caller that calls (+1 800 a frame avoided). The cost in readability: the source now carries the layout of the
  planes in the tile, a precomputed mask, two branches duplicated by phase.
- What it cost: a miscompilation of four days (a counter whose address is taken kept a stale copy); the peephole gave up on a
  whole function because of a shift loop (4 to 9 instructions a word once integrated); two measurement errors on its side (a budget
  read on the wrong ROM, a renamed wait label reading "100 % busy").
- Six things a generator must never do: delete or delay a write into a buffer whose address went to the VRAM queue or an HDMA table;
  assume an assembly routine or a `wai` touches no global; lengthen the path after `WaitForVBlank` (6 frames of sprites fit, 7 overflow);
  read a `volatile` once in a wait, or reorder two PPU register writes; put an ordinary datum above `$1FFF` (read with `lda.w` in bank
  `$00`, wrong silently); inline, rename or remove an entry point used as a probe (the step entry, `main`, the wait label), or keep
  in a register across them a global the tests read by symbol.
- **A challenge to our gauge**: it followed the game's measures on advances 3, 4, 5 (-2.83 % against -2.84 % a frame; predicted 0 to -0.3 %,
  measured -0.072 %; 1.42 % against -1.35 %), but predicted -1.72 % for the X/Y colour lot where the game measured -0.08 % in NTSC.
  We attributed the gap to the source freeze of 2026-11-11; the game disputes it (between the two versions the C gained only the 3/4
  frame, under an `#ifdef`; the NTSC path barely moved). **Cause found at 00:00 by the game itself**: the gauge's reference was the compiler with the colour switched off, not the lot's parent (which already coloured): the lot's own part is -0.35 %, the whole colour -1.65 %; the freeze and the method were not at fault. Consequence for
  G1: the arbiter is the rebuilt game replayed at each stop, not a snapshot or twelve routines.
- On "the target is the SNES": on that game the multiplier, divider and hardware copies can return only 0.23 %; the 19 % of assembly in
  the library is a library lever. What G1 must bring there: allocate A, X, Y and the direct page around calls, stop rescaling a known
  index, inline stably and report when an inlining decision flips (a good practice, **without a case as proof**: see the correction below), compose neighbouring byte writes by words in FAR.

**Correction (2026-11-14, the game's own, 22:07)**: the claim that a small function "was no longer inlined since 2026-11-04, about 3 %", and the kit's "a `static` with one caller goes back to `jsl` when its caller grows (+11 000 a tick)" built on it, are **false**: re-measured, the function was never inlined since its caller loop was created (2026-10-10), on three rebuilt chains; the compiler refuses it for room (`AUTO_INLINE_TMP_ROOM` 1 800, the caller having absorbed two other functions in source order); forcing it gains 0.04 % of busy time. It is neither a regression nor a lever; the cause was a figure of October copied without re-measuring. The case of the room rule goes to its own thread, low priority. The need for a build-to-build integration report stays as a practice, with no proof case. Removed from the synthesis.

All eight answers are in. The contests to carry into the plan: the gauge's 1.72 against 0.08 (open), several natural formulations in
the bench, correctness before gain, the cartridge check, the multiplier delay known to the compiler, invariants with fault tests.

## Open in this note

Neighbours' replies (emulator costs, corpus sources and gaps, the kit's C lessons, the three games' hot shapes and what the
generator must never do) are to be added; the owner's direction (one bench for all, best of each world, the whole console as
target) is contested in the step plan, not here.
