# C against assembly: the complete plan (2026-11-12)

Asked by the owner on 2026-11-11: every cost item measured, attributed to a
cause, an axis and an owner; the small items too, with their lesson; a path
step by step; no concession, nothing guessed. It replaces the order of
`c_asm_gap.md`, which stays as the journal of how we got here. The evidence
is in `tech/commercial_yardsticks.md` (three shipped games read) and
`tech/our_asm_on_real_games.md` (our output counted on two real games, four
perimeters rewritten by hand). The games' names and sources stay outside
this repository.

Words: **mclk** = master cycles. **Counted** = on the executed instructions
of a trace of the game, the cycles a hand would remove (an upper bound,
overlaps not removed). **By hand** = a routine of the game rewritten in
assembly, same data and algorithm, linked in its place, the game's state
proven identical at every game step. **Estimate** is written as such.

## 1. The two gauges, and where they stand

| | Before the first compiler lot | After it (2026-11-12) | Target |
|---|--:|--:|---|
| Bench, mean weighted by the games' profiles (`devtools/gapbench`); it started at 2.24 on 2026-11-11 | 2.24 | **2.00**; after L2 1.99 | under 1.5, no row above 2.0 |
| the same, every row equal / the first twelve rows | 2.74 / 2.55 | 2.41 / 2.20 | (for the record) |
| Game 1, busy mclk a frame on its scenario | 195 812 | **190 264** (-2.8 %); after L2 190 121 | falls at every compiler lot |
| Game 2, busy mclk a frame on its scenario | 214 012 | **211 672** (-1.1 %); after L2 208 020 (-1.7 % more) | falls at every compiler lot |
| The 65 examples, bytes of code | 2 393 055 | 2 385 016 | never larger |

The figure to quote is the busy time **a game step**, not a frame: when the
code gets faster fewer frames run late, so more frames do their full work
(the NMI handler stops skipping its transfers, the display pass runs once
more), and "busy a frame" mixes the cost of the code with the number of
frames treated. Measured on the first compiler lot: the handler's cost per
pass did not move (900 master cycles when the frame is late, 7 859 for a
full pass); only the number of full passes rose (463 -> 491 and 758 -> 768),
which predicts the handler's extra time within 3.2 and 0.5 %. A step: -2.98 %
and -1.09 %; frames late 219 -> 190 and 443 -> 433. And a profile by symbol is
not quoted before its instruction counts are compared: on a 4 MB LoROM the
emulator's profile credited code of one bank to data tables of another (asked
of the emulator's team).

Known bias of the games' figure (measured by the porting kit against a test
ROM, 2026-11-12): the emulator's idle count leaves out the memory refresh
that falls while the CPU sleeps, so "busy" is over-stated by about 2.5
points of a frame; the differences in master cycles between two builds are
right, the percentages of gain slightly under-stated. To be corrected when
the emulator gives the exact value.

What "1 for 1" is on a real game, by hand: **1.35 and 1.75** on two leaves,
**1.23 and 1.66** on two functions that call. Nothing of it needed knowledge
the C source does not carry. So the honest end of this plan is the games'
compiled C (81 and 93 % of their busy time) brought from about 1.5 to about
1.1 of a hand: of the order of a quarter of their busy time (extrapolated
from four perimeters: an estimate, to be replaced lot by lot by the gauge).

## 2. Every cost item, largest first

Shares are % of the busy time of game 1 / game 2 (compiled C only unless
said), counted on 160 frames of trace each; "bench" is the same count on the
bench's first twelve rows.

| # | Cost item | Share | Established cause | What shipped games do | Axis that attacks it | Expected gain | Owner |
|---|---|---|---|---|---|---|---|
| 1 | Loads and stores of the compiler's own temporaries | 27.2 / 37.1 (whole cost, measured) | no register allocation: every SSA value has a memory slot, A is a cache, X the only index | the current object sits in an index register for its whole update; 17 to 40 % of their memory operands are in the direct page (their data), 59 % of ours (our temporaries) | **L3** machine-level optimiser: values, indices and counters allocated to A / X / Y | by hand: half to three quarters of every rewrite (12 of 31 cycles a turn, 44 + 32 + 28 of 156); counted bound of the nameable part: 11.6 / 10.8 | compiler |
| 2 | of which on the stack, in functions that call | 9.2 / 18.1 (measured) | a value that crosses a call lives at `n,s`: not incrementable in place, not loadable into X | 0 to 0.5 % of their memory operands are stack-relative, 8.1 % of ours; the stack holds saved registers only | **L4** static frames in the direct page along the call graph; the callee known inside a file | by hand: a quarter of one handler's gain; estimate 4 to 7 % of busy time | compiler, link check |
| 3 | Calls from C: pushes, far call and return, stack put back, callee frame | 6.6 / 6.7 whole (226 mclk a call, 54 and 68 calls a frame); 4.1 / 4.1 counted avoidable | every call is far, every argument pushed, the caller rebuilds the stack | 58 to 86 % near calls; arguments in A, X, Y and a dozen fixed direct-page bytes; they call 10 to 35 times per 1000 instructions | **L4** near calls inside a bank, first arguments in registers for functions not reachable from assembly | by hand: about 30 % of what the convention-free versions gain; bound 4.1 | compiler |
| 4 | Dispatch by a dense `switch` | game 2: 14.8 % of busy time is its object dispatcher | no jump table is ever emitted: a binary search of compares, the value reloaded at each node; a table of function pointers in C costs more (measured by that game: +0.9 to +2.7 %) | a jump table indexed by type, or a code pointer stored in the object and called through a one-instruction trampoline; the commonest state tested first by a plain branch (490 and 281 mclk a dispatch in two of them: not fast either) | **L2** jump table for a dense `switch` | by hand: 1 567 -> 620 mclk an object, **-4.8 % of game 2's busy time** | compiler |
| 5 | Loop counter in memory, index recomputed or reloaded | 4.1 / 2.0 counted (part of 1) | the loop rules keep a counter in X or Y only in the shapes they know | the counter is the index, runs down to zero and loops on the decrement's flag; one game pushes its counter each turn for want of a register (65 mclk a turn) | L3 (the first lot took the Y case: bench `col` 4.48 -> 2.57, `row` 2.97 -> 2.00) | in 1 | compiler |
| 6 | Signed compares, materialised booleans, `cmp #0` after a load | 2.1 / 1.5 counted | `sbc / bvc / eor / bmi` for every signed compare; a range test is two of them; an `&&` chain made a 0/1 value | a range test is a bias and one unsigned compare; the carry is used as data | **L5** range test as one unsigned compare, compare against a constant without the overflow fix when the range allows, flags reused | by hand: 34 of 156 cycles a moving actor; bound 2.1 / 1.5 | compiler |
| 7 | Library assembly in the games' frames | game 1: 18.7 % of busy time is library assembly: world sprite placement 9.7 to 10.0 %, VRAM queue flush 3.3 %, NMI handler 2.9 %; game 2: NMI handler 2.5 % | written for reach, never timed against a shipped game; the yardsticks emit an OAM entry in 426 to 643 mclk, ours places a world sprite in 1 671 | an OAM entry emitted in 426 to 643 mclk; a fixed OAM slot per object slot in one; the NMI defers and budgets its transfers; one presents everything in one VBlank | **L7** the three routines against their yardstick figure, each with a `libbench` row | estimate: a third of each; to be measured row by row | library |
| 8 | Long accesses to `FAR` RAM and ROM tables | 0.2 / 2.4 counted (767 accesses a frame in game 2) | the data bank is never set for a loop; every access is long | the data bank is set once for the game or for a module, tables read absolute-indexed; 2 to 14 % long operands, 12 % in our C | **L8** the data bank set once for a loop whose accesses are all in one bank | by hand: 4 and 9 cycles a turn; bound 2.4 | compiler |
| 9 | Long branch whose target is out of reach | 3.0 / 2.6 whole cost | functions long enough that a loop's end cannot reach its head with a short branch | not counted in the games | shrinks with every lot that removes instructions; nothing of its own | not claimed | compiler |
| 10 | Runtime multiply and divide | 0.1 / 1.1; 11.6 % of one boss scene of game 2 (its own measure: 82 000 mclk a step); bench 11.6 | every product takes the general 16x16 or 32x32 routine with stack arguments; `>> 8` of a long is eight one-bit shifts through memory; a long built as `high << 16 | low` goes through extension temporaries (11 instructions for 4) | **L6** entries sized to the operands, shifts by whole bytes as byte moves, a long built in place | bench `fixed` 8.9 and `mul` 3.4 to under 2; game 2's boss scene to be measured | compiler, runtime |
| 11 | Bytes: width switches around a byte access | 0.7 / 0.6 counted before the first lot | a `sep` / `rep` pair and a mask per byte read | 22 width switches per 1000 instructions in two games, 52 in the third; our C 75 at the start: one width a loop, a byte only tested is read as a word and masked | done (lot 1, rule 1) for reads; byte stores remain (item 14) | measured in lot 1's -2.8 and -1.1 % | compiler |
| 12 | Block copies and fills | 0.2 % (memcpy) | the library moved a word at a time | one game fills and copies with the block move (48.7 mclk a byte: compact, slower than unrolled stores); another has no block move at all | done (lot A7-1): 256 bytes 24 376 -> 15 350; in game 1 its memcpy -33 % | done | library |

Bench rows that no game weighs but that say how the code is written (they
are in the weighted mean at weight 1 to 4):

| Row | C/asm now | Cause | Axis |
|---|--:|---|---|
| `unpack` | 12.2 | a byte copied through two walked far pointers, a 32-bit add each turn; the assembly uses the block move | L9 a copy or fill loop becomes the block move or unrolled stores; L3 for the pointers |
| `fixed` | 8.9 | item 10 | L6 |
| `slots` | 4.3 | a structure pointer walked as 32 bits, the bank word added each turn, the state byte read through it | L3 with a 16-bit walk when the object is in plain RAM |
| `mul` | 3.4 | item 10 | L6 |
| `fptr` | 3.2 | a call through a pointer: 19 instructions, a 32-bit pointer staged, two temporaries | L2 (the same emission as the jump table) and L4 |
| `col`, `blocks`, `chain`, `call`, `ents`, `fill`, `row` | 2.0 to 2.6 | items 1, 3, 5 | L3, L4 |

## 3. The small items, each with its lesson

| Item | Share (game 1 / game 2) | Lesson |
|---|---|---|
| A jump to a jump | 0.28 / 0.07 | the branch pass shortens reaches, it does not thread targets: a `jmp` whose target is a `jmp` or a `rts`-like tail is retargeted in the same pass |
| `cmp #0` when N and Z already come from A through a store or a transfer | 0.35 / 0.06 | flags are tracked only across a load; they survive `sta`, `tax`, `tay` |
| X shuttling between two indices | 0.16 / 0.44 | two indices alive at once want X and Y; lot 1 put the counter in Y, the general case is L3 |
| An operand passed through a temporary (`lda G / sta T / lda Y / op T`) | 1.62 / 0.82 | the operand of a commutative operation can be the memory itself; the existing fusion knows indexed operands only |
| A doubled index kept in a temporary (`asl / sta T / tax`) | 1.08 / 1.57 | a value used once after a transfer needs no slot |
| An index fetched from the stack (`lda n,s / tax`) | 1.21 / 2.10 | there is no `ldx n,s` on this CPU: a value used as an index must not live on the stack (item 2) |
| A long branch within reach taken through its `jmp` | 0.08 / 0.10 | the reach is an upper bound computed once; a second pass after the other text rules would shorten more |
| Byte stores: `sep / lda #k / sta / rep` in a loop | in 11 | a constant byte stored in a loop wants the 8-bit region around the loop, or `stz` for zero |
| A reload kept because the next line is `sep` | bench | a width switch is not a reason to forget what A holds in its low byte |
| The high word of a pointer parameter loaded and thrown away | bench | a parameter half nobody reads has no load |
| Two macros documented as faster than the call they replace are slower (2 588 against 2 030, 1 916 against 1 327) | library | a sentence saying "faster" carries its figure or is not written; the macros are to be fixed by lot L7 or removed |
| `oamClear` 143 000 mclk a call; a full map refresh 797 000 (more than two frames) | library | routines that run "only at load" run on a camera jump too: each gets a `libbench` row |
| `luna profile` credits code run from RAM (the block move written on the stack) to the nearest data label | tooling | a profile by symbol is checked against the sum before a figure is quoted (asked of luna) |
| The first bench weighed bytes at 4.2 % where the games have 0.7 % | method | an instrument is weighted on the measured thing before its mean is quoted |
| A speed measured without proof of the result (the first `mvn` copied to the wrong bank) | method | every bench row carries a checksum of what it wrote |
| A superproject pushed before the fork its submodule names | method | the pre-push hook asks the fork's remote for the pin |

## 4. What is missing, and who is asked

| Missing | For | Asked of | State |
|---|---|---|---|
| Inclusive profile by routine, with or without symbols | every measurement of a routine and what it calls (today: 68 runs per bench table) | luna (thread, prototype attached) | asked 2026-11-11 |
| Static disassembly of a ROM range at given widths | reading code a scene does not execute | luna | asked |
| Width switches, 8-bit share, addressing modes, calls per 1000 instructions of a window, without writing the trace | the width-discipline figure followed at every lot (shipped games 22 to 52, our C 75 at the start) | luna | asked |
| Code run from RAM never credited to a data label | every profile of a ROM that calls `memcpy` | luna | asked 2026-11-12 |
| A check of game 2 against its model that does not depend on the ROM's speed | its green as a proof at every compiler lot | game 2 (granted by the conductor) | to do on its side; meanwhile the proof is its test suite plus the state at every game step read by our gauge |
| The gauge's proof replayed by the games themselves at every compiler lot | the second gauge as a criterion | the two games (one thread a lot) | lot 1: one answer in, one awaited |
| A console | every figure here is an emulator's (two agree to the master cycle on the bench) | the owner, through the conductor | not asked yet: to ask with the first cartridge build |
| Another 65816 C compiler on the bench | a compiler's target, not a hand's: is our gap that of C on this CPU, or ours | decided by the owner (2026-11-12): a PRIVATE measurement through the one non-commercial compiler that has sources and a SNES target, after L2; nothing redistributed, nothing published, its author not written to; if the gap is flagrant, the two compilers' assembly compared routine by routine and each difference attributed to a cause in ours. Criterion for any comparison tool: it runs natively on macOS, Linux and Windows (to check for that compiler when the measurement starts). Two commercial compilers are ruled out on that criterion (one Windows-only with an unpublished price, one made for another machine); a third is closed and x86-only. Checked 2026-11-12 for the non-commercial one: official binaries for Windows and Linux x64; on Linux arm64 its compiler, driver, optimiser and assembler build from source without error, its linker does not (the current linker's sources are not reachable at their origin that day, the public copies are two years older and cannot read its link scripts); macOS not established. The measurement waits for one of three ways, asked of the conductor | after L2; blocked on the linker |
| An assembly column that is not ours | the bench's first admitted weakness | public homebrew in 65816 assembly under MIT or zlib licences (read 2026-11-12: a decompression library with a published throughput, an entity / metasprite / collision engine with unit tests, a finished game already in our assembler's syntax): their routine taken as it is with its notice, our C twin written on its data; the invitation to outside programmers stays with the owner (a public act) | to do as bench rows, first the decompressor against `unpack`; commercial disassemblies and GPL or unlicensed projects are read only |
| Whole-file call information in the backend; link-time metadata | L4 | us | to build in L4 |
| Jump-table emission in the backend | L2 | us | to build in L2 |

## 5. The path, step by step

One lot = one axis. Before it starts: the thinker contests it (what would
make it not pay, what it could break, what the yardstick says), in writing
in the lot's note. It is written in its own worktree, re-validated by this
session on every gate (Class A: full rebuild, the 65 ROMs against those
built before, both differential tests beyond their gate, `make tests`),
measured on both gauges with the games' state proven identical at every
step, then the games' sessions confirm on their own build, and a lesson is
written. No lot starts before the previous one is measured. An axis is
dropped only on a written proof that it gives nothing or cannot be done.

| Lot | Axis | Attacks | Before starting, measure | Done when |
|---|---|---|---|---|
| done | bytes read as words, the counter in Y, loads of what A holds, constants into operands, one mask (compiler); `memcpy` / `memset` on the block move; the map engine, sprite hiding, an attribute-ready sprite entry (library) | 11, 12, part of 5, three library findings | | bench 2.24 -> 2.00; games -2.8 and -1.1 %; map engine 940 -> 380 mclk an entry |
| **L2** | **a jump table for a dense `switch`**, and the same emission for a call through a pointer | item 4, bench `state`, `fptr` | how many `switch`es of the games and the examples are dense, and their executed weight (count on the traces); code size of a table against the compare tree | game 2's dispatcher within 1.2 of the hand's (620 mclk an object); `state` under 1.2; no example larger |
| **L3** | **the machine-level optimiser**: the function's text parsed once into instructions with what each reads and writes; liveness of A, X, Y, flags and direct-page slots; values, indices and counters promoted to registers over regions; the existing text rules re-expressed as its cases | items 1, 5, the small items of temporaries | the ceiling: the two rewritten leaves give 1.35 and 1.75; a design note first, with the passes in order and the proof obligation of each (a peephole mistake is silent: 20 000 seeds a rule) | the two rewritten leaves compiled within 1.1 of the hand; bench weighted mean under 1.6. **Contested and counted 2026-11-13 (M0): values in X/Y would give 1.1 and 2.2 % of busy time (high bound 2.7 / 6.6), under the 3 % rule: not opened as written, L4 first; note `gap_lots/L3_machine_optimiser.md`** |
| **L4** | **values that cross a call off the stack; near calls; first arguments in registers** for functions assembly cannot reach | items 2, 3, bench `chain`, `call` | the share of calls whose callee is in the same file, and of functions provably non-reentrant, in the two games | the two rewritten calling functions within 1.2 of their hand version; a link refused, not miscompiled, on a cycle through a static frame |
| L5 | signed compares as range tests, flags reused, booleans not materialised | item 6 | the share of signed compares whose operands have a provable range | counted share under a third of today's |
| L6 | multiply entries sized to the operands, shifts by bytes, longs built in place | item 10 | game 2's boss scene profiled (the one scene where the runtime weighs) | `fixed` and `mul` under 2; that scene's runtime share halved |
| L7 | the library's hot routines against their yardstick: world sprite placement, VRAM queue flush, NMI handler; `oamClear`, the full map refresh; the two "fast" macros fixed or removed | item 7 | each routine's `libbench` row and the yardstick's figure for the same work | game 1's library share from 18.7 % to under 14 % |
| L8 | the data bank set for a loop | item 8 | which loops of the games read one bank only | game 2's long-access count halved |
| L9 | copy and fill loops as block moves or unrolled stores; unrolling asked by the programmer | bench `unpack`, `fill`, `copy` | code-size budget per function | `unpack` under 3; no game slower |
| **S1** | **the 8-direction scrolling brick** (the second half of the owner's lot): a tile ring, a new row and column built once and sent in grouped transfers, the VBlank budget measured, 16x16 blocks, one present | the yardsticks' architectures (`tech/commercial_yardsticks.md`): 256 to 315 mclk a tilemap entry with block expansion, worst diagonal 39 002 mclk of building and 264 bytes in the blank | after L2, so that it is not written twice | an example, a luna test, its figure against the yardsticks; the port that asked for it tries it |
| **R1** | **a light pseudo-random the way shipped games do it** (owner, 2026-11-12), after L2 and without delaying it: (1) what shipped games do, sourced (routines documented in public, those of the games read outside this repository as patterns in our words: mixing with the frame counter and the player's input, precomputed tables); (2) the library's `rngNext` measured against them: master cycles a draw, bytes, and "invisible to a player" (visible patterns, period); (3) a bounded draw without the runtime's division (`rngNext() % n` calls the software divide today, and the documentation gives that as its example; the hardware multiplier or a mask is enough); (4) how many draws a frame the two games make and at what cost | a cost nobody measured: one draw and one bounded draw | the figures of (2) and (4) | a `libbench` row, the bounded draw in the library, a lesson |
| S2 | an SDK that makes "less work a frame" the easy path: active lists, dirty marks on what is sent, staggered work, one present; the examples rewritten where they redo per frame what did not change | what the shipped games win by | each against its naive form on an example | per proposal |

Order of L2 to L4 is by measured weight on real game code: the jump table
is the largest single item and the cheapest; register allocation is half to
three quarters of every hand gain and needs only the function; the
convention and the stack need the file. L5 to L9 follow by counted share.
After L2, L3 and L4 the plan is reviewed against the gauges: if the two
leaves are not within 1.1 of the hand, the question of a new code generator
behind the same IR is decided on that measurement, not before.

## 6. What is not in this plan, and why

- **Game routines rewritten in assembly**: measured, one routine gives its
  game 1 to 2.5 %; the same factor from the compiler applies to all the C.
- **Another C compiler**: none of the living ones has a licence we can ship
  on or a 16-bit 65816 back end (read 2026-11-11); "another compiler" means
  writing ours, decided after L3.
- **A search for shorter sequences, profile-guided optimisation**: kept as
  approaches (`c_asm_gap.md`, B3, B4); they enter the plan when a lot's
  measurement shows the hand-written passes have stopped paying.

## 7. What we propose that was not asked

- **The machine-level optimiser (L3)** instead of more text rules: the
  owner asked for the gap to come down; the measurement says the gap is
  register allocation, and the backend has none. It is a redesign of the
  layer between instruction selection and the assembler, kept behind the
  same differential tests.
- **A link that refuses** a static frame it cannot prove safe (L4), in the
  line of every other silent failure this SDK turned into a refusal.
- **The compiler steered by the emulator's profile**: hot loops unrolled and
  inlined, cold code kept small, `switch` cases ordered by frequency. No
  65xx compiler we read does it; we own a cycle-exact profiler and
  deterministic replays. Proposed after L4, on the gauge.
- **A search for the best loop with the emulator as judge**, for the few
  loops a profile names; its results kept as rules.
- **Width discipline as a third gauge**: width switches per 1000 executed
  instructions of compiled C, against the shipped games' 22 to 52; followed
  at every lot once luna gives the figure without a trace.
- **"Less work a frame" as an SDK feature (S2)**: what shipped games win by
  is architecture, not instructions; the library can make it the default
  (active lists, dirty marks, staggered work, one present).
- **The bench kept honest by construction**: weighted on real profiles, every
  row with a checksum, replayed on a second emulator, and owed a row for
  every pattern a shipped game or a real game shows.
- **The second gauge as a tool**: the two games rebuilt outside their
  repositories on any compiler, their busy time and the proof of identical
  state at every game step in four minutes; it is what lets a compiler lot
  be judged before anyone else is asked.
