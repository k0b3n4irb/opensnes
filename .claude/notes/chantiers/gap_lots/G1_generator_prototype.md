# G1 — a bounded, throwaway prototype of a code generator designed for the 65816

Opened 2026-11-13 by the owner's answer "A" (the conductor's decision log, same day), after the short lot and alongside L4.
Written before any code: cost, rule, plan. Nothing of the prototype enters `develop`; the current generator stays the yardstick.

## Question
Do the compiler's retouch axes (all counted, 6 to 9 % at most for the games) leave a gap that a generator designed for the
65816 closes? "Designed" means: allocation of A, X, Y and direct-page slots as one register bank, static frames and the calling
convention decided at the start, instruction widths chosen by real cost.

## What is measured, not guessed
Four perimeters rewritten by hand with equivalence proven at every game step (`tech/our_asm_on_real_games.md`): two leaves (1.35
and 1.75 over today's C) and two functions that call (1.23 and 1.66 with their callees and a free convention; 1.10 to 1.13 body
alone under today's ABI). The perimeters, the traces and the proofs exist; the prototype reuses them as they are.

## Engagement rule (written before the count, as given by the conductor)
The prototype commits the project to a generator only if, on the same traces and with equivalence proven, **both**:
1. it is within **1.1 of the hand** on the two leaves; and
2. it saves **at least 10 % of game 2's busy time** over the current generator.
Otherwise: retouch the current one (L3/L4 lots) and stop. The figures go to the conductor; any commitment beyond is the owner's.

## Scope (what the prototype is, and is not)
- In: a stand-alone pass that takes the QBE IR of ONE function after the existing optimisations and emits 65816 text for it with
  its own allocator (graph colouring or linear scan over a bank of A, X, Y and direct-page words, chosen by measurement on the
  two leaves first), own instruction selection by width, and the SDK's ABI at the function boundary (so it links in place of the
  function the current generator would emit). Functions it cannot handle fall back to the current generator, and the fallback
  count is reported.
- Out: a new frontend, a new IR, the library, the assembler, any change to `develop`. Static frames are L4's; the prototype
  consumes L4's convention when it lands, it does not duplicate it (hypothesis: the two overlap on the same slots).

## Cost (estimate, hypothesis until the first step is measured)
A pair Opus / Opus, in days of the pair: (1) harness, equivalence proof on the four perimeters, the two hand versions as targets:
1 to 2; (2) allocator and selection on the two leaves, within 1.1 or a written reason: 3 to 4; (3) the two functions that call,
with the ABI at the boundary: 2 to 3; (4) the traces of both games, the whole-program count against the 10 % line: 1 to 2.
Total **7 to 11 days**, with a stop after (2): if the leaves are not within 1.2 after step 2, the prototype stops there and says
so (an early verdict costs 4 to 6 days, not 11).

## Order and gates
1. Contest this plan (Fable, written, before step 1): what is missing, which cost is underestimated, which yardstick is not
   looked at (the other 65816 compiler's measure, private, is the first: `dumps/…/vbcc`).
2. Step 1 harness; step 2 stop/go on the leaves; step 3; step 4 verdict to the conductor with the table.
3. Each step: state identical, the prediction written before, the lesson in the kit.

## Open
- Whether the allocator should work on the IR before or after the existing peephole rules (hypothesis: before; the rules become
  its cleanup).
- How the prototype and L4's static frames share the direct page (to settle once L4's window is measured).
