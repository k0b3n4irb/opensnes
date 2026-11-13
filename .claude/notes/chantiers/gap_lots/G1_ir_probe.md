# G1 probe: a third colour "lives in X / Y" in `color_slots`

Opened 2026-11-13 by the owner's answer "go for B" (relayed by the conductor): the IR-side probe asked by L3's contest
(change 2, `L3_machine_optimiser.md`) and by G1's contest (taken item 4, `G1_generator_prototype.md`), never made. One
day, Opus alone (no sub-agent, the conductor's instruction). Nothing of it is pushed; it enters `develop` only if the
conductor says so.

## Question

Does an X / Y colour given by the existing slot colouring, on the IR, take most of what a new generator would take on the
hot loops (values, indices, counters in registers)? If yes, the G1 prototype is not needed. If it stops on the X the
emitter takes for itself (the `tax` of every indexed access), that is the evidence that justifies G1.

## What the probe is (written before the code)

- A temp (16 bits: a Kw, or a 32-bit temp whose high half nobody reads), or a web of them joined by a phi or a one-word
  copy, is coloured X or Y instead of a frame slot. Its definition stores with `tax` / `tay`, a read loads with `txa` /
  `tya`; as the index of a plain array (`sym,x`, near) it costs nothing in X and is `sym,y` in Y (abs,y exists for the
  loads, stores and ALU forms; there is no `long,y`, so a FAR or const array indexed by a Y value takes `tyx`).
- Candidates: costliest first (`fn->tmp[].cost`), never live across a call (`cross`), never interfering with another
  candidate of the same register (the colouring's own graph).
- **How it meets the emitter's X**: the function is emitted with the colour, and the emission is the proof. Each IR
  instruction's text is read back; a write of X (or Y) that is not the colour's own, while a value of that colour is live
  after it or read by it, refuses that value; so does any emit path that would read the value's slot (an ALU operand, a
  high half, a phi cycle), a store with an 8-bit accumulator, and a `tax` whose N / Z flags something reads. The function
  is emitted again without the refused values until nothing is refused (the set only shrinks). No splitting: a value is
  in its register for its whole life or not at all.
- The loop rules (`peephole_loop_x`, X and Y passes) do not run on a register the colour uses in that function: they
  assume the register free at the loop's edges.
- `QBE_NO_XY_COLOUR=1` turns it off; with it the output is byte-identical to today's.

## Prediction (before the code)

Leaves: the two hand rewrites (`tech/our_asm_on_real_games.md`) put three quarters of the gain in register allocation:
leaf A (game 2) 23 of 31 cycles a sub-step (values 12, counter 11), leaf B (game 1) 104 of 156 a moving actor (values
44, index in X 32, counter 28). The bench twins of the two hot loops are `glide` (the sprite pass of game 2, its heaviest
common loop) and `soa` (the actors of game 1 in parallel arrays, leaf B's shape).

| | predicted share of the C - hand gap taken | why |
|---|--:|---|
| `glide` (gap 31 134 mclk, SlowROM) | 8 to 11 % | the counter (i x 2) in Y indexes the word arrays as `sym,y`: the `tyx` before each access goes; the second index (`id`, step 4) cannot be X (the easing table's `tax`) and stays in memory |
| `soa` (gap 12 217) | 0 to 10 %, possibly negative | the doubled index in Y saves its reloads, but the counter then loses the register the loop rule gave it (the table read `frame_off` takes X): without splitting, the colour holds one of the hand's two indices |
| leaf B (game 1) | at most 20 % | part of the 32 cycles of the index; nothing of the 44 of values (ALU operands: `adc T`, `cmp T`); the counter is already the loop rule's |
| leaf A (game 2) | at most 5 % | its values are 32-bit accumulators (out of the colour), its counter already in a register |
| gapbench, the twelve routines | weighted mean 1.99 -> 1.95 to 1.99 | |

Games (% of busy time saved, game 1 / game 2): **0.2 / 0.2, range 0 to 0.6 each**. L3's M0 counted 1.1 / 2.2 (middle) for
direct-page slots rewritable while X or Y is free over their span, with the loop rules on. The colour is narrower than
that count on three sides: the emitter's own X (every indexed access whose index is not the coloured value writes X, and
game 2's long-indexed reads of FAR RAM, 10.9 % of its busy time as `lda long,X`, cannot take a Y index); no splitting;
ALU operands refused. And the lesson of lot LF+M5 applies: a count that classifies values for a decision the compiler
takes statically gives a ceiling (`tcc__lf` +4 words: predicted 0.64 / 1.14, measured 0.00 / 0.24).

**Rule for the verdict, written before the measurement.** The probe "takes most" if, on both leaves, it closes at least
half of the gap that register allocation explains (A: 74 % of the gap, B: 67 %), that is **at least 37 % of leaf A's gap
and 33 % of leaf B's** (on the bench twins when the real leaves cannot be rebuilt). Then the prototype is not needed. Under
that, and if the refusals are mostly the emitter's X, the probe is the evidence that justifies G1's allocator (which says
nothing yet of G1's 10 % line, G1-M0). Expected, from the table: **under the line on both; the reason the X of service**.

## Measured

(filled after the code)
