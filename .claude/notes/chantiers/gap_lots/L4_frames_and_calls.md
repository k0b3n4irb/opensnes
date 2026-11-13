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

(filled by M1)
