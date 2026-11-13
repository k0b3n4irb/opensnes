# L4: values that cross a call off the stack, near calls, first arguments in registers

Lot of `../c_asm_gap_plan.md` (items 2 and 3, bench `chain`, `call`). L3's count (M0, 2026-11-13) put it first: stack
slots `n,S` are 9.8 and 19.2 % of the two games' busy time in compiled C, the largest class of temporaries. That is the
whole cost of those accesses, not what L4 would save. This note is the count that decides, its rule written before it.

## The decision rule (written 2026-11-13, before any figure of this count)

The count (M1) measures on the same traces as M0 (the gauge's builds, 4 x 40 frames a game) what each part of L4 would
save, every saving being the measured cost of an executed instruction minus the cost of what replaces it:

1. **Static frames**: each executed `n,S` access of compiled C, as the same access to a direct-page slot (1 cycle less a
   load, store or ALU operation in 16 bits), plus what a direct-page slot allows and a stack slot does not: `lda n,s / tax`
   -> `ldx d` (the transfer goes), `lda n,s / inc|dec a / sta n,s` and `lda n,s / clc / adc #1 / sta n,s` -> `inc|dec d`;
   plus the frame made and undone (`tsc / sec / sbc #N / tcs` and its epilogue) where the function would keep no stack slot.
2. **Near calls**: each executed `jsl` from compiled C to compiled C in the same file as `jsr` (2 cycles less, `rtl` and
   `rts` the same), the callee being the one the trace shows.
3. **First argument in a register**: for each executed C-to-C call in the same file with arguments, the push of its first
   argument and the callee's first read of it from the stack, as nothing (it arrives in A); the stack put back after the
   call shrinks by the same amount.

**Rule**: L4 opens if the sum of the three parts is **at least 3 % of busy time on at least one game**; each part enters the
lot only if it gives at least 0.5 % on one game. Under 3 % on both games, L4 does not open as written and the plan is
reviewed against the next largest measured axis (from M0: operands read by `cmp T` / `sbc T`, 14.5 and 9.6 % of busy time
in whole cost, S4 and L5). The figure is an upper bound (it assumes every function may have a static frame: no recursion,
no function called both from the NMI and from the main loop); the count also reports how much of it comes from functions
the call graph shows to be recursive or reached from the NMI handler, which a static frame cannot take.

### Recount for the functions a compiler can decide alone (2026-11-13)

A function whose every caller is in its own file (a `static` function, address never taken) is the only kind whose call
graph the compiler sees whole. Part 1 restricted to them: **2.70 % (game 1) and 3.25 % (game 2)**, of 4.12 and 6.19. Their
frames are small: median 10 and 8 bytes (90 and 174 functions), the hot ones 4 to 30 bytes (game 2's dispatcher 18, its
emitter 30; game 1's two actor steps 18 and 12). With the first argument in A (0.6), L4 within the file is about 3.3 and
3.9 % of busy time at most.

## The design (draft, 2026-11-13; to be contested in writing before it starts)

### Where a static frame lives: the direct page, for a reason other than speed

A static frame in plain RAM is not safe: an exported function called both from the main loop and from an NMI callback
(nothing at compile time says it is not) would enter the static frame of its `static` callee twice. In the **direct page**
it is safe by construction, the way `tcc__lf` already is: the NMI handler runs C on its own direct page
(`tcc__nmi_registers`), so the same `.b` operand names another byte there. So static frames go to the direct page, and the
NMI page grows to cover them (160 -> 256 bytes, 96 bytes of RAM). IRQ handlers are assembly.

The direct page has no free room today: `.registers` `$00-$3F`, SNESMOD's block `$40-$7F` (the linker puts plain C
globals there when SNESMOD is not linked), `tcc__lf` `$80-$9F`, then plain C globals from `$A0`. L4 reserves a window
(`$A0-$FF`, 96 bytes) for static frames; the globals that sat there move above it (they are addressed `.w`, nothing else
changes; the plain band loses 96 bytes).

### Which functions

A function gets a static frame when (1) it is `static` and its address is never taken (`addrtaken()`, as for the entry's
`rep #$20`), so every caller is in the file; (2) it calls (a leaf keeps `tcc__lf`); (3) it is on no cycle of the file's
call graph; (4) its frame is at most what the window gives the file. Everything else keeps its stack frame, unchanged.

### Where in the window: an overlay per file, decided by the compiler

In a file, two static functions share bytes when neither can be active while the other is: a function's frame starts at
the end of the deepest frame among its static callers in the file (the classic overlay of the 8051 compilers, here on a
call graph the compiler has whole). The file's static frames are one section `.sframe.<file>` of the size of its deepest
chain. Across files, two files' sections never overlap (a `static` function of file A can be active under a `static`
function of file B through an exported function between them): the linker places the sections side by side in the window.
A link-time overlay across files would share more bytes; it needs the whole program's call graph and is left for later.

### What cannot be seen in a file, and how it is refused, not miscompiled

A cycle through files: `static g` in A calls an exported function of B that calls back an exported function of A that
calls `g` again: `g`'s frame is overwritten while it is active. The compiler cannot see it. The link check
(`opensnes-rom check`, compiled, run after every link) reads, per object, the calls each static-framed function makes out of
its file (a side table the compiler emits), builds the program's call graph (indirect calls reaching every function whose
address is taken), and **refuses the link** on a cycle through a static frame, naming it. A window too small for the sum of
the files' sections is a link error (a `.b` operand out of the page), also a refusal; the message says which files, and a
per-file switch keeps a file on stack frames.

### Steps

| Step | What | Proof | Predicted |
|---|---|---|---|
| F0 | the window reserved, the NMI page 256 bytes, globals moved; no function uses it yet | the 65 ROMs differ only by addresses (`diff_corpus` frame for frame); `nmi_budget`, WRAM oracle re-captured with the reason | 0 |
| F1 | static frames for the functions above, per-file overlay, accesses `.b` | fixture: a static frame used by the main loop and by the same file's functions called from an NMI callback (D isolation), values against a host model; the differential statement test with static functions (its odd seeds are `static` already); 20 000 seeds; the 65 ROMs A/B; both games' state at every step | the recount's part 1 for static functions, about 2.7 and 3.25 % at most, less what does not fit |
| F2 | the link check for cycles through static frames | a fixture whose link must be refused (a cycle through two files), and one that must pass | 0 (safety) |
| F3 | first argument in A for `static` callees | as F1 | about 0.6 % |

Near calls are out (M1: 0.3 %). The `n,S` traffic of functions that are not `static` (1.4 and 2.9 % of part 1) stays for a
later step that needs the program's call graph at link time.

## Contested (Fable sub-agent, 2026-11-13: the written contest of a compiler lot's plan, the one use the owner allows)

**Verdict: go with changes.** The NMI argument holds, but the overlay rule and the link check as written leave a
miscompilation, and the 96-byte window cannot hold per-file sections. (From reading code and the note; nothing built.)

**(a) Soundness.** The direct-page claim holds in every context the SDK allows: `nmiSet` and `nmiSetLag` callbacks run after
`lda #tcc__nmi_registers / tcd` (`templates/crt0.asm` ~1130, 1184, 1441) and never in the same NMI; IRQ handlers are
assembly only (`interrupt.h` ~157-171); code that moves D (`tile.asm`, `sprite_oamset.asm`) is assembly that calls no C;
the Super FX owned-cart NMI keeps the interrupted D but calls only `gsu_present_step` (not traced to its end); SA-1 runs
assembly with D = `$3000`; `tcc__fp` frames never move D; `splice_fn` makes the inlined body's slots the caller's; an NMI
nested in an overrunning callback already breaks `tcc__r*` and `tcc__lf` (not new).
**The hole is re-entry into the file, with no cycle.** File A, static `f` and `h`, each called from an exported function:
neither has a static caller, so both are at offset 0. `f` calls `B.x`, which calls exported `A.e1`, which calls `h`: `h`
overwrites `f`'s live frame. No function is on a cycle: the link check as written passes it. The same inside one file (`f`
calls exported `e1`, which calls `h`), and through any indirect call (a dispatcher reaching address-taken handlers that call
the file's static helpers). So: **the criterion is overlap, not cycles** (for each call made while a static frame is live, no
function it can reach may own bytes overlapping the frames live at that point), and **F2 is a precondition of F1**, not a
later safety step. A refusal would also fire on ordinary C (callbacks) with only a per-file switch as remedy: a hidden quirk
to weigh against `PHILOSOPHY.md`.

**(b) Would it pay?** Parameters stay on the stack (read at `framesize + PARAM_OFFSET + n`, caller-pushed): they are inside
M1's 2.70 / 3.25 % and F1 does not remove them. Only cross-call temps, `tcc__lf` overflow and stack objects are left to win
(temps that cross no call are already in `tcc__lf`); a function with an address-taken local keeps its prologue, and the
local's address would have to be `tdc + off`. **The window does not fit**: 96 bytes over about 20 files is 4.8 bytes a file,
under one median frame; game 2's dispatcher and emitter (18 + 30) take half of it; side-by-side sections mean link errors
from the first real game. F0 costs 192 bytes of plain band in every ROM (window and NMI page), 16 % of the tight game's
1 170 free. Uncounted upside: a far pointer in a direct-page slot is dereferenced in place (`lda [slot],y`).

**(c) Alternatives.** Link-time placement: take it now (the compiler emits `lda.b sframe_f+N` as it does `tcc__lf+N`, plus
its side table; the link tool assigns offsets from the program's call graph; the window holds the deepest chain of the
program, not the sum of files; the same pass does the overlap check; exported functions can join later). Arguments stored
into the callee's static frame: replaces F3 (`sta.b` costs what `pha` does; the callee's parameter reads become direct-page;
the caller's stack restore after the call goes, 9 cycles; every argument, not the first). D moved per function: no
(`phd/tcd/pld` 11 cycles a call, a non-aligned D costs a cycle per access, `tcc__r*` and `tcc__lf` out of reach). Plain-RAM
frames for functions proven NMI-unreachable: the proof fails (the callback is a run-time pointer). More `tcc__lf`: nothing
for temps that cross a call.

**(d) Ordering.** F0 first is wrong (192 bytes and a WRAM re-capture for a predicted 0). Spike first, in count mode: the
compiler dumps per static candidate its slot kinds and its calls, no codegen change; then the link-time placement and the
overlap check together; then F1. The window's size comes from the measured deepest chain, not a fixed 96.

**(e) Changes, ranked.** 1. Overlap criterion, its check shipped with or before F1. 2. Link-time placement over the whole
program; no per-file sections. 3. Arguments in the callee's static frame (F3 folded in). 4. Window sized from the
measurement; F0 after the spike. 5. Decide what a refusal offers beyond a per-file switch. **The measurement before F0**: the
static candidates' executed `n,S` traffic by slot kind (parameter read, cross-call temp, `tcc__lf` overflow, stack object),
the deepest static chain of each game in bytes, and the share in functions the overlap criterion refuses. **If what remains
after parameters and refusals is under the rule's 3 %, L4 as written does not open and change 3 becomes the lot.**

### Taken (2026-11-13)

All five changes are taken. Already measured, the parameters (dynamic, M1's traces; a parameter is an `n,S` access beyond
the frame and the return address): in `static` functions, whole cost of their own frame slots **4.53 % (game 1) and 7.38 %
(game 2)**, of their parameter reads **1.88 and 1.87 %**; in the other functions 2.83 / 0.61 and 9.59 / 0.35. Next step,
**M2** (a count, no code change): the rest of the split by slot kind (cross-call temp, `tcc__lf` overflow, stack object; the
compiler's count mode or the `.c.asm`'s slot map), the deepest static chain in bytes, the share refused by the overlap
criterion with indirect calls reaching every address-taken function, and the saving of change 3 (every argument stored in
the callee's frame, the stack restore gone). Rule unchanged: under 3 % on both games after parameters and refusals, the
static frames do not open and change 3 alone is weighed.

## Measured

### M1, the count (2026-11-13)

Same traces and tooling as M0 (private, `dumps/opensnes-etalons/l3m0/m1.py`; the gauge's builds, today's compiler
`develop` `0a9b9fa0`, 4 x 40 frames a game). Each saving is the executed instruction's measured cost minus its replacement's:
an `n,S` access as a direct-page access is one internal cycle less (6 mclk: `lda sr,S` is opcode, operand, internal, data;
`lda d` with D page-aligned is opcode, operand, data); `(n,S),y` -> `(d),y` two; `lda n,s / tax` -> `ldx d` the transfer;
`lda / inc a / sta` on the same slot -> `inc d` (50 mclk); the frame made and undone, all of it; `jsl` -> `jsr` 14 mclk
(the bank byte fetched and pushed); the first argument's push, all of it.

| % of busy time | game 1 | game 2 |
|---|--:|--:|
| **1. static frames** | **4.12** | **6.19** |
| - the cycle of each `n,S` access (needs the DIRECT PAGE) | 1.65 | 3.29 |
| - `lda n,s / tax` -> `ldx d` | 0.64 | 1.13 |
| - frame made (prologue) + undone (epilogue) | 0.64 + 0.79 | 0.75 + 0.90 |
| - `lda / inc / sta` -> `inc d` | 0.40 | 0.13 |
| 2. near calls (`jsl` -> `jsr`, callee in the same file) | 0.30 | 0.31 |
| 3. first argument in A (its push) | 0.58 | 0.61 |
| **sum** | **5.01** | **7.11** |
| of which functions a static frame cannot take (recursive, or reached from the NMI and the main loop) | 0.00 | 0.00 |

(1 recursive function and 1 called from both sides in game 1; 2 and 6 in game 2; none of them carries stack traffic in the
windows.)

**Decision, by the rule written before the count: L4 opens.** The sum is above 3 % on both games. Part 1 enters (4.1 and
6.2), part 3 enters at the edge (0.58 and 0.61), **part 2, near calls, does not** (0.30 and 0.31, under 0.5 on both): left
out of the lot.

What the count says about the design:
- A third to a half of part 1 (1.65 and 3.29) exists only if the static frames are in the **direct page**: a static frame
  in plain RAM is addressed absolute, `lda abs` costs the same four cycles as `lda sr,S` (a ROM fetch instead of an internal
  cycle). The rest (no frame to make, `ldx`, `inc`) holds anywhere. The direct page is shared (`.registers` `$00-$3F`,
  SNESMOD `$40-$7F`, `tcc__lf` `$80-$9F`): how many bytes a static frame scheme gets there, or whether D moves per function
  (`tcd`, its cost), is the first question of L4's design.
- Part 1's frame make and undo (1.4 and 1.65 %) is what every function with a stack slot pays at every call; a function
  whose slots all go static pays none.
- The figure is an upper bound: every function is assumed to get a static frame. L4's design note says which do not
  (reached from assembly, address taken, recursion, the NMI) and recounts.
