# C against hand-written assembly: the plan of approaches (2026-11-11)

Goal (owner): bring the cost of C compiled by cc65816 as close as possible to
1 for 1 with 65816 assembly written by hand. This page compares the
approaches, gives the starting factor per routine and the factor each
approach aims at. Nothing is fixed before it is reviewed; then one cause,
one fix, one measurement, and this page gets the measured column.

## Where we start (A1, done: `devtools/gapbench`, `make bench-gap`)

Master cycles a call on luna, SlowROM (FastROM gives the same factors).

| routine | C | asm | C/asm | where the extra cycles go (read in the compiler's output) |
|---|--:|--:|--:|---|
| copy | 22 276 | 13 842 | 1.61 | the loop is already the hand loop; assembly has `mvn` |
| fill | 40 928 | 16 177 | 2.53 | the value reloaded every turn (4 of 20 cycles), no unrolling |
| oam | 47 734 | 22 893 | 2.09 | structure through a 32-bit pointer, bytes one `sep`/`rep` each, signed compares |
| col | 28 912 | 6 452 | 4.48 | a far pointer walked in memory (32-bit add a turn, re-staged a turn), counter in memory, no unrolling |
| row | 19 152 | 6 452 | 2.97 | counter in memory (X is taken), value through a temporary, `sep`/`rep` + two masks a byte, no unrolling |
| box | 20 712 | 12 408 | 1.67 | signed compare as `sbc / bvc / eor`, the `&&` chain made a 0/1 value, counter in memory |
| fixed | 94 370 | 10 474 | 9.01 | `(s32)a * b` calls the general 32x32 multiply (four pushes, sixteen partial products' worth of work for a 16x8 need); `>> 8` is eight one-bit shifts through memory |
| mul | 27 126 | 7 772 | 3.49 | `u8 * u8` calls the general 16x16 routine (it does use the hardware multiplier, three products and the interrupt-context test) with its arguments on the stack |
| ents | 34 021 | 14 738 | 2.31 | `Ent *e` is a 32-bit pointer stepped in memory, fields read `lda.l $0000NN,x`, bytes one `sep`/`rep` each |
| call | 69 380 | 32 642 | 2.13 | three pushes, `jsl`, the stack put back by `tsa/adc/tas`, the caller's own values on the stack across the call |
| soa | 40 262 | 24 865 | 1.62 | one index register for word and byte arrays, signed compares, temporaries |
| glide | 106 399 | 67 680 | 1.57 | one index register, the sign extension of a table byte, temporaries |
| **geometric mean** | | | **2.55** | from 1.57 to 9.01 |

The assembly column is ours: it is a floor we wrote, not the state of the
art. A9 replaces it by what shipped games do.

The 1.7 to 1.9 quoted before this bench was one routine (the copy). On
twelve, three families stand out: what the 65816 or the console has and C
has no word for (block move, hardware multiplier: copy, fixed, mul), what a
register allocator would do (counter and second index in registers, no
temporaries: every row), and what the ABI costs (call, pointers of 32 bits).

## The approaches

Gain: the factor aimed at on the rows named, an estimate from counting the
cycles of the instructions the approach removes; the measured column is
filled lot by lot. Cost and risk: S / M / L.

| | Approach | Aims at | Estimated | Cost | Risk | Measured |
|---|---|---|---|---|---|---|
| A9 | **The yardstick of commercial games**: four to six successful games of the console, chosen so that their routines cover our use cases (scrolling, sprite list and OAM, DMA and HDMA, main loop and dispatch, sound, decompression). Each is disassembled on the machine, outside this repository; a routine of theirs is set beside what the SDK produces for the same use case in our examples (library assembly and compiled C both); the output is the table of what the SDK lacks or does worse. Here: only the patterns in our own words, a stand-alone example or bench row per pattern, and our measurements; no code, address or title | every row, the library, and what no row covers yet (data layout, dispatch, VBlank scheduling) | unknown until the first routine is read; it sets the ceiling the other approaches aim at, in place of our own hand assembly | M per game (reading), S per pattern | S for this repository (nothing of a game enters it); the titles and ROMs are the owner's decision | |
| A1 | **The bench against hand assembly**, kept as a gate (`--check` in `make tests`): no routine may get slower | all | the instrument | done | none | 2.55 |
| A6a | **Backend peephole and register use**: Y as a second index (catalogue A11), the loop counter in X or Y whatever else the body does, a value kept in A across an index load, a byte read as a word read + mask where the address is plain RAM or ROM, one mask not two, invariant reload out of the loop | row, col, fill, box, soa, glide, oam, ents | each of them down by 20 to 35 %: 1.6 -> 1.2-1.3, 3.0 -> 2.0 | M, a series of small rules | M: a peephole mistake is silent; each rule gets its difftest hunt (20 000 seeds) | |
| A6b | **Bounded search for shorter sequences**: every straight-line window of 2 to 4 instructions the corpus emits, replaced by the shortest sequence with the same effect on A, X, Y, flags read later and memory, found by exhaustive search on a 65816 model and checked on luna | all, a few % each | 3 to 8 % overall (hypothesis until the first run) | M | M: the model of "flags read later" must be exact; rules land through the same hunt | |
| A5 | **Calling convention**: first arguments in A / X / Y or direct-page slots for functions not reachable from assembly (the test already exists: `addrtaken`, not exported), `jsr`/`rts` inside a module, no frame rebuild after a call, 16-bit pointers for plain RAM objects where the type says so | call, ents, oam, mul | call 2.13 -> 1.3; ents and oam down by 15 % | L: ABI, library assembly keeps the stack ABI at its boundary | L: every asm/C boundary; Class A with the A/B corpus proof | |
| A7 | **The library and the runtime in assembly where it is hot**: multiply entries sized to the operands the compiler can prove (8x8 inline on the hardware multiplier, 16x8, 16x16 -> 32 widening; today every product takes the general 16x16 or 32x32 routine), `>> 8` / `>> 16` as byte moves, `memcpy` / `memset` on `mvn` and unrolled stores, sprite list and row / column builders as library routines | fixed, mul, copy, fill | fixed 9.0 -> 1.5-2; mul 3.5 -> 1.5; `memcpy` under the C loop instead of above it | M | M: the hardware multiplier is shared with interrupt callbacks (the runtime tests `in_nmi_ctx` and takes a software path there; an inline sequence must keep that test); Class A/B | |
| A6c | **Idioms the compiler can recognise**: a copy loop becomes `mvn`, a fill loop unrolled stores, a counted loop with a small body and a constant bound is unrolled (by 2, 4, 8 or whole), widening multiply recognised from `(s32)a * b` | copy, fill, col, row, fixed | copy 1.61 -> 1.0-1.1; fill 2.5 -> 1.2; col / row to under 2 | M-L | M: code size (a budget per function, measured on the 65 ROMs) | |
| A8 | **What the programmer can say**: a documented set, each measured on the bench: `static` (already inlined at one site), locals for globals a store may alias, `FAR` only where needed, near pointers, an attribute to put a global in the direct page, an `inline` asm-free intrinsic header for the multiplier and block move (`mul8x8()`, `memcpy`), tables over shifts | all, at the source | 5 to 20 % on a routine rewritten with the guide; the intrinsics give A7's gain where the compiler cannot see the idiom | S-M | S | measured today, source changed only: an index instead of a walked pointer, ents 2.31 -> 1.62, col 4.48 -> 3.54; the 16x8 product split by hand, fixed 9.01 -> 4.03; a fill unrolled by hand in C gets SLOWER (2.53 -> 3.03) and so does row with an index (2.97 -> 3.11): two findings for A6a |
| A3 | **State of the art**: what llvm-mos, cc65, KickC, Oscar64, vbcc and the 65816 compilers (Calypsi, WDC, ORCA/C) do that we do not, from their documentation; feeds A5, A6, A8 with techniques already proven elsewhere | ordering of the others | no gain of its own | S (documentary) | none | done, see below |
| A2 | **Idioms of a professional assembly game**: a partner port sets the routines of a commercial game beside our compiler's output for the same job and sends generic patterns in words; each pattern is reproduced in a stand-alone example here and measured on the bench | new rows, new rules for A6a / A8 | unknown until the patterns arrive | S on our side per pattern | S (nothing of that game enters this repository) | |
| A4 | **Profiles of real games**: `luna profile` on the games built on the SDK, their ten heaviest routines, each given a stand-alone twin in the bench when its shape is not there yet (two are: soa, glide); the gain of every lot is also read on those games, by their sessions | keeps the bench honest | no gain of its own | S per routine | none | 2 of 12 rows |

## What the others do that we do not (A3, read 2026-11-11)

Read in the sources and manuals named; no project publishes a gain per
technique, and none publishes C against hand assembly. The one public
comparison (c-bench-64, 6502, seconds) puts the compilers that allocate the
zero page and pass arguments in registers 2 to 10 times ahead of cc65.
llvm-mos has no 16-bit 65816 code generation (its issues 320 and 321 are
open); the 65816 references are Calypsi, WDC and ORCA/C.

| Technique | Who | Goes into |
|---|---|---|
| A pointer walked by a loop stays put in the direct page and Y steps (`lda [dp],y`); pointers live where the indirect modes reach them without a copy | llvm-mos (`MOSIndexIV`, `MOSCallingConv.td`) | A6a |
| 8-bit accesses grouped so the width switches once a region; a byte read as a word read | Calypsi 65816 guide, 20.3.3; Oscar64 narrows 16 to 8 bits by value ranges and takes `__assume()` | A6a, A8 |
| First argument in A (opt-in: `simple_call`, fastcall), then arguments in direct-page pseudo-registers, some kept across calls | Calypsi (`_Dp[0-7]`, `_Dp[8-15]`), llvm-mos (`RC`/`RS`), vbcc (`r0..r7`), cc65 | A5, in that order: the opt-in form does not touch the library's assembly |
| Static frames from a whole-program proof of non-reentrance, overlapped along the call graph | llvm-mos (`MOSNonReentrant`, `MOSZeroPageAlloc`), Oscar64, cc65 `--static-locals`, KickC | A5 (first step: the `static` functions of a module) |
| Load/store runs and repeated stores turned into `memcpy` / `memset`; block move for structure copies | llvm-mos (`MOSCombiner`) | A6c |
| Unrolling asked by pragma (`unroll(full)`), forced inlining, constant parameters propagated into the callee | Oscar64, Calypsi | A6c, A8 |
| Peephole rules found by exhaustive search and proven (solver or shared emulator) | DeiMOS, 6502-enumerator, Massalin 1987 | A6b |
| Register choice by search on the cycle cost; optimal allocation by tree decomposition | KickC (`uplift`), SDCC (Krause 2013) | not planned: the heaviest step, after everything else |

Corrected 2026-11-12: two 65816 compilers do use the console's multipliers (one through library routines behind an option, one with a SNES target whose manual does not say whether inline or by routine); the first reading had missed a 65816 back end of vbcc with a SNES target. The
PPU one (`$211B` -> `$2134`) is signed 16x8 with no wait, but unusable while
Mode 7 is displayed.

Sources: llvm-mos `llvm/lib/Target/MOS/*` (github.com/llvm-mos/llvm-mos),
Calypsi 65816 guide 5.18 (github.com/hth313/Calypsi-tool-chains), Oscar64
manual (github.com/drmortalwombat/oscar64), cc65 documentation
(cc65.github.io), vbcc 6502 documentation, KickC manual
(gitlab.com/camelot/kickc), ORCA/C `Gen.pas` (github.com/byteworksinc/ORCA-C),
c-bench-64 (github.com/thred/c-bench-64), snes.nesdev.org wiki
(Multiplication, MVN and MVP block copy). Not readable that day: the
llvm-mos wiki, Krause's figures, the rules of PVSnesLib's 816-opt.

## Order

0. **A9 as soon as the ROMs are there**, one use case at a time, in parallel
   with everything below: it corrects the targets and adds rows.
1. **A3 and A4 first, they cost little and steer the rest** (A3 done, above;
   A4: ask each game for its current top ten).
2. **A7 runtime multiply and shifts**: the largest factor (9.0, 3.5), a
   closed piece of assembly, no change to code generation.
3. **A6a**, rule by rule, most frequent pattern first (counter and second
   index in registers, byte reads, temporaries): it moves every row and
   every game.
4. **A6c** block move and unrolling, then **A8** written from what 2-4
   taught (the guide says what the compiler now does alone).
5. **A5** last among the code-generation lots: the widest change, and its
   gain is measured better once the loops are tight.
6. **A6b** when the hand-written rules stop paying; **A2** whenever patterns
   arrive.

Target, to be confirmed after lots 2 and 3: geometric mean under 1.5, no
row above 2.0, with unchanged or smaller code on the 65 examples. 1.0 on
every row is not promised: `col` and `row` at 1.0 mean full unrolling
(505 bytes against 122).

Every fix: gain on the bench, `make tests`, both differential tests, the 65
ROMs rebuilt (identical or differences explained and checked on luna), no
size or speed regression elsewhere (`sdkbench`, `libbench`, `cyclecount`),
and a lesson per cause in the porting kit.

## State (kept current)

- 2026-11-11: plan reviewed by the conductor. Target granted: geometric mean
  under 1.5, no row above 2.0, the 65 examples no bigger; reviewed again
  after the first two lots. Order until the ROMs of A9 arrive: A6a, then A7,
  then A5 (first argument in A).
- Running, each in its own worktree: **A6a lot 1** (a byte read as a word
  read, the loop counter in Y when X is taken, a load of what A already
  holds dropped across blocks, a constant added to an index folded into the
  operand, one mask not two); **A7 lot 1** (`memcpy` / `memmove` on the block
  move through a thunk built on the stack, `memset` on unrolled
  absolute-indexed stores).
- After each measured advance: one thread to luna and to the two games that
  build on this tree, asking for their factual return on their real case
  (ROM rebuilt on `develop`: size, cycles of their hot routines, hashes,
  their tests). Everybody else is on hold (owner, 2026-11-11).
- Rows to add to the bench once lot 1 has landed (A4, shapes sent by the
  games; stand-alone equivalents, nothing of theirs): a walk of a linked
  list by index with a chain of tests then a dense `switch`; a loop of
  sub-steps on three 32-bit accumulators with a table read through a pointer
  derived from an angle; up to 50 objects whose fields are 16-bit arrays
  indexed by a byte, box and visibility tests, sprite emission inlined; a
  byte table indexed by a word feeding four byte arrays (measured by a
  partner port at 2.7 to 2.9 against a commercial routine on that pattern).
- Agreed outside yardsticks beyond A9: the twelve routines through the other
  living 65816 C compilers (after reading their licences; figures stay
  private if a licence forbids publishing them); whole-frame profiles of the
  commercial games without disassembly; the bench replayed on a second
  emulator with luna. Inviting outside assembly programmers to beat our
  assembly column is with the owner (a public act).
- Disassembly for A9 lives outside every repository, in a folder the
  conductor named; nothing leaves it but patterns in our words.

## A9, first results (2026-11-11)

Three commercial games read; the note is `tech/commercial_yardsticks.md`.
Shipped assembly is not faster than our hand assembly, often slower; it wins
by doing less a frame. So the assembly column of the bench is a harder
target than the games, and what the games add is (1) a figure for width
discipline (22 `rep`/`sep` per 1000 instructions in two games, 52 in the
third, 75 in our C), (2) the weight of the calling convention (they call 10
to 35 times per 1000 instructions with arguments in registers and the direct
page; the bench 4 times), (3) thirteen things the SDK lacks or does worse,
four of them in the library (the map engine at 3 times their cost per
tilemap entry, hiding sprites at 3 to 9 times, `oamSet`, the NMI handler's
width switches), and (4) the rows the bench owes to their patterns.

## Our own approaches, beyond A1 to A9 (draft, to be completed and costed when the A9 analysis is in)

The owner opened every door on 2026-11-11: a redesign, assembly used
massively, another C compiler, if the measurement justifies it. What stays
untouchable: the showcase, the rights (tool licences, no code of the
commercial ROMs), the proofs (tests, the 65 ROMs rebuilt, the factual
returns of the emulator and the two games). Candidates, none measured yet:

- **B1. A machine-level optimiser instead of text rules.** The backend has
  no register allocator; what stands in for one is a dozen peephole passes
  over the emitted text, each with its own hand-made liveness. Parse the
  emitted function once into a structured 65816 form (instruction, operands,
  what it reads and writes of A / X / Y / flags / width / direct-page slots /
  memory classes), run generic passes on it (liveness, copy propagation,
  dead stores, promotion of a direct-page slot to X or Y over a region,
  placement of `rep` / `sep` as a dataflow problem, branch shortening), emit.
  Every existing rule becomes a case of a general pass; the same optimiser
  could read the library's hand-written assembly. Aim: what a register
  allocator gives, without rewriting instruction selection.
- **B2. Width as a property of the code, not of each access.** The compiler
  computes everything in 16 bits and narrows at each byte access. Commercial
  games of the era live in 8 bits for whole routines (to be confirmed by the
  A9 traces). A width analysis on the IR (value ranges, `u8` arithmetic that
  cannot overflow visibly) and regions of 8-bit code chosen by cost.
- **B3. Optimisation guided by the emulator's profile.** We own a
  cycle-accurate profiler and deterministic replays: feed the profile of a
  game's scenario back to the compiler (hot loops unrolled and inlined, cold
  code kept small, switch cases ordered by frequency, the direct page given
  to the hottest globals). No 65xx compiler we read does this.
- **B4. Search for the best loop, the emulator as the judge.** For the few
  loops a profile names, a bounded stochastic or exhaustive search over
  instruction sequences, equivalence checked on a model and on luna with the
  differential tests' inputs; the result is kept as a rule (general) or as a
  per-function replacement (specific).
- **B5. Assembly bricks with a register calling convention, for the hot
  jobs every game has** (object loop iterator, sprite list from parallel
  arrays, row and column builders, box batches, fixed-point batches): the
  game stays in C, its hot paths are library calls measured against the
  commercial routines. Precedent: `oamPlaceWorld` (1.68 against the C loop).
- **B6. Another code generator.** No living 65816 C compiler has a licence
  we can ship on (read 2026-11-11: one is hobby-only and closed, one is
  Windows-only and restricted; llvm-mos has no 16-bit 65816). So "another
  compiler" means writing ours: either a new back end behind the same IR
  (instruction selection by patterns with a cycle cost model, registers
  A / X / Y and direct-page pseudo-registers allocated), or B1 grown until
  it is one. To be decided on the measured ceiling of B1 on the bench.
- **B7. Whole-program knowledge**: static frames from the call graph,
  arguments in direct-page slots, 16-bit pointers where the program never
  leaves bank `$7E` low RAM (A5 taken to its end at link time).
- **B8. An SDK that makes "less work a frame" the easy path.** What the
  shipped games win by is not their instructions, it is what they do not do:
  empty slots skipped in fifteen instructions, an active list instead of a
  pass over every slot, work spread over frames by parity, a new tilemap
  line only when the camera leaves a loaded window, nothing resent that did
  not change, everything presented in one VBlank when the step is done. To
  propose, each measured on an example against its naive form: an object
  pool with an active list and a per-object code pointer; a "dirty" mark on
  what the library sends (OAM ranges, tilemap lines, palettes) so an
  unchanged frame costs nothing; a stagger helper (every Nth frame by
  index); fixed OAM slots a pool; one present call. And the examples
  themselves rewritten where they redo per frame what did not change.

Target since 2026-11-11 (conductor): the assembly column stays the target
(mean under 1.5, no row above 2.0), and a column "shipped games" is added
where the work is comparable, so the table says, row by row, when the C
passes under the level of a commercial game. The bench gets the rows it owes
to the games' patterns and a realistic weight of calls.

## Our output on two real games (2026-11-11): the order changes

`tech/our_asm_on_real_games.md`. Counted on executed instructions: every
pattern of the list fixed gives a real game at most 20 % (the bench's 2.55
is about tight loops on our data); 27 to 37 % of a game's busy time is the
traffic of the compiler's temporaries, a third to a half of it on the stack
in functions that call; calls cost 6.6 % whole, 4.1 % avoidable; bytes 0.7 %
(the bench: 4.2); runtime multiply 0.1 to 1.1 % (the bench: 11.6).
So, after the two lots that run: **the next compiler lot is values that
cross a call off the stack and the call itself (A5 with B7's static
frames), then the machine-level optimiser (B1)**; A7's multiply entries and
A6c's unrolling stay, as bench work with a small effect on the games. The
gauge of every lot is double: the bench, and the two games' busy master
cycles a frame on their scenario.

## Design notes for the next compiler lot: values that cross a call, off the stack (draft, 2026-11-11)

Granted by the conductor as the next lot, with the second gauge as a
criterion (the two games' busy master cycles a frame: 195 933 and 214 089
at the start, measured by us on each lot from their sources built on this
tree outside their repositories, confirmed by their sessions).

What the counts allow (estimate, to be replaced by a measurement): the
`n,s` traffic is 9 and 18 % of the games' busy time; a direct-page slot
saves one cycle in five on each access (2 to 4 %), ends "index fetched from
the stack" (1.2 to 2.1 %), the frame set-up (about 40 master cycles of the
226 a call costs: 1 %) and the counters kept on the stack (0.9 % in one
game): 4 to 7 % of busy time. Arguments in registers and near calls: up to
4 %. Not more: the lot is worth doing, and it is not the whole gap.

Two ways, and why the first alone is not enough:
- **Callee-saved direct-page slots** (as one 65816 compiler does): safe
  with recursion and interrupts, no whole-program knowledge; but saving and
  restoring a slot costs about 15 CPU cycles a call, and at 54 to 68
  non-leaf calls a frame with two or three slots each that is what the lot
  would gain. Only as the fallback.
- **Static frames along the call graph** (as the 6502 compilers do): a
  function that is not recursive keeps its cross-call values at fixed
  direct-page addresses, laid out by depth so that a caller's and a
  callee's never overlap, nothing saved. In the direct page, because the
  NMI handler runs on its own direct page: a function called from the main
  loop and from an interrupt callback is then safe by construction, which
  absolute addresses would not give. Room: `$A0-$FF` of the page, 48 words
  for the deepest chain, costliest values first (the existing cost: use
  count weighted by loop depth), the overflow on the stack as today; the
  NMI's mirror page grows from 160 to 256 bytes.
  What must be proven, and by whom: inside a module the call graph of the
  `static` functions is known (the inliner already walks it); for exported
  functions and calls through pointers it is not. So the compiler writes,
  per function, its callees and the slots it holds across each call, and
  the link check (`opensnes-rom check`, which already refuses a link) builds
  the whole graph and FAILS the link on a cycle through a function with a
  static frame or on two live frames that overlap; a function may be marked
  reentrant in the source and keeps the stack. Refused, never miscompiled.
- The measurement that decides the ambition: the hand rewrite of a hot
  routine of each game (running). If a hand gets 1.3 on a real routine, this
  lot and the machine-level optimiser are the plan; if it gets 2, the
  question of a new code generator (B6) is open.

The bench is to be re-weighted on the games' profiles (calls, stack, bytes)
before its mean is quoted again; the old mean stays beside it.

## Measured on real game code by hand (2026-11-11): what the order should be

`tech/our_asm_on_real_games.md`, last two sections. Four perimeters of the
two games rewritten in assembly by hand, equivalence proven at every game
step: leaves 1.35 and 1.75; functions that call 1.10 to 1.13 with the body
alone and today's ABI, 1.23 and 1.66 with their callees and a convention of
the programmer's. Nothing of it needed knowledge the C source lacks. Order
by measured weight, to be confirmed in the complete plan:
1. **a jump table for a dense `switch`** (function by function; 4.8 % of one
   game's busy time by itself, 2.53 on the dispatch);
2. **register allocation of values, indices and counters** (function by
   function; half to three quarters of every hand gain): the machine-level
   optimiser, with the two rewritten leaves as its test: done when the
   compiler is within 1.1 of the hand on them;
3. **values that cross a call off the stack, near calls, arguments in
   registers** (the file, then the program): about a third of what the
   convention-free versions gain;
4. signed compares as range tests, the data bank set for a loop.
Rewriting game routines by hand is not the way: one routine gives its game
1 to 2.5 %; the same factor from the compiler applies to all the compiled C.
