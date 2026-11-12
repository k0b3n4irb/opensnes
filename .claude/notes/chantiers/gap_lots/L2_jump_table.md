# L2: a jump table for a dense `switch`

Lot of `../c_asm_gap_plan.md`. Written before the lot starts; the measured
column is filled when it ends.

## The axis

A dense `switch` compiles to a binary search of compares, the value
reloaded at each node; no jump table is ever emitted. One game's object
dispatcher (a `switch` of about a hundred cases, 14.8 % of its busy time)
was rewritten by hand: 1 567 -> 620 master cycles an object, of which the
table itself is 117 of the 162 CPU cycles saved (the rest: values re-read
from the stack, which is L4).

## Contested before starting (the thinker)

- **Why it could not pay**: a table costs a fixed 24 CPU cycles or so
  (`lda / sec / sbc #min / cmp #range / bcs / asl / tax / jmp (table,x)`); a
  `switch` of three or four cases is cheaper as compares. So: only from a
  number of cases and a density to be measured on the bench, not chosen.
  And a game whose hot `switch`es are small gains nothing: the first game's
  counted "chain of compares" share is 0.14 % of its busy time.
- **What it could break**: QBE's control flow has two successors a block.
  A table is a block with N. So the compare tree STAYS in the IR, where
  every pass understands it, and the table is an emission of it: the
  backend recognises a tree of blocks that only compare one value against
  constants and branch, and emits the table in place of those blocks. What
  must hold: every block of the tree is entered from the tree only; no move
  sits on an edge from the tree to a case (or the case is reached through a
  stub that makes it); the value is one 16-bit temporary, unchanged across
  the tree; the table is in the same section as the `jmp`, read through the
  program bank.
- **What the yardstick says**: shipped games dispatch through a table
  indexed by type or a code pointer in the object, in 281 and 490 master
  cycles a dispatch: a table at 24 cycles of our own (about 190 master
  cycles) is at their level or better.
- **How we would notice a mistake**: a wrong target is a wrong program:
  the differential test of statements generates `switch`es (to check: with
  enough cases to reach the table; if not, the generator is extended
  first); a compiler check pins the table and its `_off` twin the tree;
  the 65 ROMs against those built before; the two games' state at every
  game step.

## Predicted, before measuring

| Gauge | Now | Predicted | Why |
|---|--:|--:|---|
| bench `state` (a dense `switch` of 8 against a hand jump table) | 1.41 | under 1.2 | the dispatch is most of the row |
| bench, weighted mean | 2.00 | 1.96 to 1.98 | `state` weighs 8 of 87 |
| game 2, busy master cycles a frame | 211 672 | -3 to -4 % | 117 of 162 cycles of a dispatcher measured at -4.8 % by hand |
| game 1, busy master cycles a frame | 190 264 | 0 to -0.3 % | its counted chain-of-compares share is 0.14 % |
| the 65 examples, bytes of code | 2 385 016 | not larger | two bytes a case against a compare and a branch |

## Cost and risk

Size: one recognition pass and one emission in the backend, a threshold, a
switch to turn it off, two compiler checks, perhaps an extension of the
statement generator. A few hundred lines. Risk: medium; a wrong table is a
silent miscompilation, caught by the gates above; the tree stays in the IR
so no middle-end pass is touched. Second step, only if the first is
measured: the same indexed indirect emission for a call through a table of
function pointers (bench `fptr`, 3.16).

## Measured (to fill)

| Gauge | Predicted | Measured | Gap between the two, and what it teaches |
|---|--:|--:|---|
