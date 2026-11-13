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
