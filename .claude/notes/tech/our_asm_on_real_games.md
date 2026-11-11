# Our compiler's output on two real games, counted (2026-11-11)

Part of `chantiers/c_asm_gap.md`. Two games built on the SDK were profiled
and traced on luna in their loaded scene (160 frames of trace each, the
share of C agreeing with the profile within one point), and every costly
pattern of the compiler's output was counted on the EXECUTED instructions
of compiled C. "Lost" is an upper bound: the measured master cycles of the
instructions a hand would remove, minus what it would write instead. The
same count was run on `devtools/gapbench` (the twelve base routines). The
games' sources, names and traces stay outside this repository.

Both games: compiled C is 81 % and 93 % of the busy time; 196 000 and
214 000 master cycles busy a frame.

## Where the cycles go (lost, % of busy time; overlaps possible)

| Family | game 1 | game 2 | bench |
|---|--:|--:|--:|
| calls that registers, near calls and no frame would avoid | 4.1 | 4.1 | 2.5 |
| temporaries and reloads (`sta T … lda T`, a load of what A holds, across a branch) | 3.4 | 3.9 | 3.5 |
| loop counter in memory, index recomputed or reloaded | 4.1 | 2.0 | 2.3 |
| index fetched from the stack (`lda n,s / tax`) | 1.2 | 2.1 | 0.3 |
| doubled index kept in a temporary (`asl / sta T / tax`) | 1.1 | 1.6 | 0.6 |
| operand passed through a temporary (`lda G / sta T / lda Y / op T`) | 1.6 | 0.8 | 0.2 |
| signed compares, materialised booleans, `cmp #0` after a load | 2.1 | 1.5 | 1.1 |
| pointers and long accesses | 0.2 | 2.4 | 2.1 |
| bytes (`sep` / `rep` around a byte) | 0.7 | 0.6 | 4.2 |
| X shuttling between two indices | 0.2 | 0.4 | 0 |
| loop control an unrolled loop would not have | 0.0 | 0.1 | 4.0 |
| runtime multiply and divide (whole cost) | 0.1 | 1.1 | 11.6 |
| **sum of the bounds** | **19.0** | **19.8** | **20.8** |
| measured, not a loss: every `lda` / `sta` of a compiler temporary (direct page + stack) | 27.2 | 37.1 | 29.7 |
| of which on the stack (`n,s`) | 9.2 | 18.1 | 3.2 |
| measured: whole cost of calls from C (push, `jsl`, `rtl`, stack put back, callee frame) | 6.6 | 6.7 | 4.1 |
| `rep` + `sep` per 1000 instructions of C | 17.3 | 13.9 | 68.6 |

A call costs about 226 master cycles around the work (`jsl` 56, `rtl` 44,
pushes 44 to 51, stack put back 34 to 39, callee frame 40); the games make
54 and 68 calls a frame from C, 1.5 arguments each.

## What it says

1. **Fixing every pattern on the list gives a real game at most 20 %.** The
   bench says 2.55 against hand assembly; that figure is about tight loops
   on our data, not about a game. The games' own C is already careful.
2. **The bench weighs the wrong things for a game**: bytes (4.2 against
   0.7), unrolling (4.0 against 0.1) and the runtime multiply (11.6 against
   0.1 to 1.1) are over-weighted; calls (2.5 against 4.1), the stack
   (index from the stack, 0.3 against 1.2 to 2.1; `n,s` traffic 3 against 9
   to 18 % of the time) are under-weighted. The games' C switches width 14
   to 17 times per 1000 instructions, fewer than the commercial games (22
   to 52): bytes are not their problem.
3. **The mass is the traffic of temporaries**: 27 to 37 % of a game's busy
   time is loading and storing the compiler's own temporaries, a third to
   a half of it on the stack, in functions that call. A value that crosses
   a call lives at `n,s`: it cannot be incremented in place, nor loaded
   into X without passing through A. This, not a list of peephole rules,
   is what separates the output from a hand's.
4. **Order for the games**: (a) values that cross a call off the stack
   (static frames for functions proven non-reentrant, or direct-page slots
   saved by the callee), with the call itself (arguments in registers, near
   calls); (b) temporaries, counters and indices in registers (the
   machine-level optimiser); (c) signed compares and booleans; (d) long
   accesses to `FAR` RAM (game 2: 767 a frame, 2 % lost: the data bank set
   for a loop); (e) the rest.
5. **Two gauges from now on**: the bench (tight loops, against assembly) and
   the games' busy master cycles a frame on their own scenario, rebuilt on
   each lot by their sessions.

## The real "1 for 1": two hot routines rewritten by hand (2026-11-11)

In a working copy of each game (outside its repository), one hot routine of
the profile was rewritten in assembly by hand, same data layout, same
algorithm, the SDK's ABI at its boundary, linked in place of the C, and
measured on the game's loaded scene. Equivalence proven: the game's state
read at every one of 700 and 341 game steps is identical byte for byte, the
frame hashes at four steps too, and the first game's 30 tests pass on the
modified ROM.

| | A (game 2) | B (game 1) |
|---|---|---|
| The routine | a path follower: sub-steps on four 32-bit accumulators, a sine table in ROM; a leaf its author had already bent to the compiler | the move and publication of 18 actors in parallel word arrays, clamps, a ROM table; a leaf written naturally |
| Share of the game's busy time | 5.4 % | most of 11.5 % |
| Master cycles, C -> hand | 17 153 702 -> 12 707 120 | 10 704 344 -> 6 103 146 |
| **Factor C / hand** | **1.35** (loop 1.24, prologue 1.87, epilogue 1.54) | **1.75** (moving actor 1.84, still actor 2.4) |
| The game's busy time | -0.95 % | -2.5 % |

Where the gain comes from (cycles on the common path, checked against the
measured total), A per sub-step of 31 gained, B per moving actor of 156:

| Cause | A | B | Can the compiler learn it? |
|---|--:|--:|---|
| values kept in a register instead of temporaries | 12 | 44 | yes: register allocation |
| the index held in X instead of reloaded from its memory cell | | 32 | yes |
| loop counter in a register, direct loop-end branch, no jump to a jump | 11 | 28 | yes |
| general signed compares replaced by one unsigned range test (exact on the whole domain) | | 34 | yes: idiom |
| ROM table read absolute: data bank set once, second index in Y | 4 | 9 | yes, with the link map |
| a value off the stack | 2 | | yes |
| `cond ? k : 0` materialised when `k` is the bit tested | | 9 | yes: algebraic identity |
| a `clc` the value's range makes useless | 2 | | yes: range analysis |

Outside its loop (47 % of A's gain): each 32-bit value built as
`high << 16 | low` goes through extension temporaries (11 instructions for
4), the index kept on the stack is re-read, the frame is made and undone.

**What it says.**
1. On real game code the factor is **1.35 to 1.75**, not the bench's 2.2 to
   2.7: 1.35 when the programmer has already bent the C to the compiler,
   1.75 when it is written naturally.
2. **Nothing of the gain needed knowledge the C source does not carry**: the
   "perfect compiler" version is the hand version within 2 %. The target is
   reachable by a compiler in principle.
3. Rewriting one routine by hand, even the first of the profile, gives the
   game 1 to 2.5 %; the same factor obtained by the compiler applies to all
   the compiled C, 81 and 93 % of the busy time. This is the measured
   answer to "assembly massively, or a better compiler": the compiler,
   with assembly for the library's hot generic jobs.
   (In B the hand version is called where the C was inlined into its caller,
   and the caller, recompiled without the loop, changed cost too, by -7 %:
   caller and routine together give 1.47.)
4. The order of causes is register allocation first (values, index,
   counter: three quarters of the gain), then signed compares, then the
   data bank. Two leaves only: nothing here about functions that call.
