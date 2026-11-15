# G1 R3-0: the paper rules, frozen (2026-11-15)

Committed alone, before any paper text (`G1_r3_design.md`, R2.6 step 1). The agent that writes the paper text of `soa` and
`path` gets **this file and the IR of the two functions, nothing else** (not the predictions of the design note). A rule
changed after the first paper line is a new file with a new date; the paper already written is then void.

## What is written

For each function, whole (loops and the code outside them): the 65816 text the designed allocator would emit, by hand,
under the rules below only. The IR idioms (range test, `cond ? k : 0`) are applied as the IR pass would, once, before the
allocation. The text carries its `; sframe fn` line. Public boundary kept (`rep #$20` at entry, arguments on the stack left
to right, return in A, `rtl`); a leaf keeps `tcc__lf`.

## Locations (each value is in exactly one at a time)

1. **A** (16 bits): the value an ALU chain works on. Every `adc sbc cmp and ora eor`, shift and load-op needs it in A.
   A stays live across instructions that do not write A (X/Y moves and steps, stores, branches that keep A).
2. **X, Y** (16 bits): indices, counters, values only moved, compared or stepped. No `long,y`. `sym,x` takes an unsigned
   16-bit index. Any indexed access whose index is another value writes X: that is a cost, not a refusal.
3. **Direct-page slots**: everything else. `tcc__lf` (`$80-$A7`) and the `tcc__r*` words outside a helper call's live range;
   never `$40-$7F`.
4. **D moved** and **DBR set**: allowed only under the conditions of the design note 2.1 (entity in bank 0 below `$2000`;
   no `.b` `tcc__lf` access nor helper call while D is moved; DBR restored before return and no plain-RAM absolute access
   while it is moved). Each use is named in the text's comment.
5. 32-bit values: a pair of slots, never split between register and memory.
6. No value is held across a helper call in A, X, Y, `tcc__r*`.

## Allocation (the algorithm of 2.2, as a procedure the writer can follow)

1. Take loops innermost first. In a loop body, assign every value to A, X, Y or a slot; try the assignments in the order of
   the **cost** below and keep the cheapest legal one (by hand: enumerate the assignments of the values live in the loop,
   at most a few dozen; write the count tried).
2. Order independent instructions of a block only to lengthen the stretches a value stays in A (the local schedule).
3. Outside loops: a linear scan, the loop decisions fixed at the loop edges (moves on the edges).
4. Bytes are read as word and masked (no 8-bit width regions); a byte store keeps `sep` / `rep`.

## Cost (the unit of 3 of the design note)

Master cycles of each instruction from its bytes and its accesses: ROM byte 8 (SlowROM) / 6 (FastROM); WRAM, direct page,
stack 8; internal cycle 6; I/O 6. Penalties: D low byte not zero +1 CPU cycle per `.b` access; `abs,x` / `abs,y` with a
16-bit index and a page cross, and the write forms, +1; `long,x` one byte more than `abs,x`. Block weights are those of a
luna run of today's build of the same source (given with the IR). Both tables, slow and fast, are reported.

## Refusals (the paper refuses where the prototype would)

Functions with a jump table; a function whose address is taken and whose caller is not the prototype's, for the free
convention (public ABI stays); inline assembly; any construct for which no rule above gives a location. A refusal is
written as a refusal with its reason, never worked around by hand.

## What the paper may not do

Use a trick that no rule above permits; read today's generated text of the same function after starting; tune a choice
because of the ratio it would give. The second agent writes the text, then the measurement is made by another step.
