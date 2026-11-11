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
