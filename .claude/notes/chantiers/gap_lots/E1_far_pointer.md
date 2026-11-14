# E1: a far pointer read where it lives, stepped on 16 bits

First short lot taken from the comparison with another 65816 compiler (a
private measure of the gapbench C compiled by it; only neutral figures leave
it). Written before the code; the measured column is filled when the lot ends.

## The axis

On `unpack` (two far pointers walked a byte at a time) the other compiler
spends 0.34 of what we spend. Its literal loop is nine instructions:
`sep / lda [p] / sta [d] / rep / inc p / inc d / dex / txa / bne`. Ours is
twenty-six, for two reasons:

1. **Every access copies the pointer into `tcc__r9`** (`lda.b lf+N / sta.b
   tcc__r9 / lda.b lf+N+2 / sta.b tcc__r9+2`, 16 CPU cycles) before
   `lda [tcc__r9]`. The pointer's own direct-page slot (`tcc__lf+N`, in
   bank $00's direct page like `tcc__r9`) is already a valid `[dp]`
   operand. `tcc__r9` is a cache of ONE pointer, so a loop that alternates
   two pointers stages at every access (`peephole_r9_loop` keeps one
   pointer there for a whole loop, never two).
2. **Every `p++` is a 32-bit addition** (`lda / clc / adc #1 / sta / lda /
   adc #0 / sta`, 24 cycles). The other compiler documents that a far
   object never crosses a bank, so it adds on the low 16 bits only and the
   bank byte never moves (`inc dp`, 7 cycles).

Part 1 is always exact: `[tcc__lf+N]` reads the same three bytes the staging
copied, as long as nothing writes them in between and the direct page does
not move. Part 2 is exact only under a guarantee, established below.

## Safety of the 16-bit step (established before the code)

Facts, each checked on this tree:

- **A C object never crosses a bank.** The backend puts each C object in its
  own section (`compiler/qbe/emit.c`, `wla_emitdat`): `.far.N` RAMSECTION
  `BANK $7E SLOT 2`, `.bss.N` / `.data.N` `BANK 0 SLOT 1`, `.rodata.N`
  SEMISUPERFREE in the asset banks. Every memory map the SDK ships
  (`templates/memmap*.inc`) gives SLOT 2 = `$2000-$FFFF`, SLOT 1 =
  `$0000-$1FFF`, a ROM slot of one bank. Measured with wlalink: a `FAR`
  object of `$E001` bytes is refused ("No room for RAMSECTION ... in RAM
  bank 126"), two objects that do not fit together are refused (never
  spilled into `$7F`), a const object of `$8001` bytes in LoROM is refused
  ("a section cannot span banks").
- **But an object may END at the last byte of a bank**: measured, `FAR u8
  other[16]` after a `$DFF0`-byte object sits at `$7E:FFF0-$FFFF`. Its
  one-past-the-end pointer, valid in C, is `$7F:0000`; stepped on 16 bits it
  becomes `$7E:0000`. Dereferencing it is undefined, but comparing it is not:
  `for (p = other; p < other + 16; p++)` would never end. So a stepped
  pointer must never be compared, stored, passed, returned or converted:
  only dereferenced, or stepped again.
- **A pointer from elsewhere carries no guarantee.** An argument, a call's
  result, a pointer loaded from memory, an `extern` object (assembly may
  define a buffer across `$7E:FFFF`, or in a bank of its own map): refused.

The rule, therefore (to be implemented exactly):

- A **root** is the address of a data object DEFINED in this translation
  unit (`$sym`, `$sym + k`): the backend placed it, in one bank.
- A **member** is a 32-bit temporary defined by a copy or a phi of members
  and roots only, or by `add` / `sub` of exactly one member or root and an
  integer (a constant, a 16-bit value, its extension, a product of it by a
  constant).
- Every use of a member is the ADDRESS of a load or a store, or the pointer
  operand of another member's definition. Anything else (a compare, a store
  of the pointer itself, an argument, a return, a truncation) removes it,
  and the removal propagates (greatest fixed point).
- Then the value of every member is its true address modulo 2^16 with the
  bank of its root, which is the true address wherever C allows a
  dereference. Only a member defined by `add` / `sub` whose pointer operand
  is a member TEMPORARY takes the 16-bit form; a root plus an index keeps
  the full sum.

What is refused, written: pointers from arguments, calls, memory, `extern`
objects, function addresses; any pointer that is compared, tested against
null, stored, passed or returned; an access outside the object the pointer
came from (undefined in C) that crosses a bank wraps inside the bank where it
used to reach the next one.

The provoked faults that must turn the tests red (`QBE_FARPTR_FAULT`): (1)
an `extern` object accepted as a root, the object placed in assembly across
`$7E:FFFF`; (2) the use rule ignored, a pointer compared with the end of an
object ending at `$7E:FFFF`.

## Predicted, before measuring

Cycles counted on the emitted text (SlowROM, about 7.8 master cycles a CPU
cycle on these lines). Three earlier counts of this kind were too
optimistic: the ranges lean to the prudent side.

| Gauge | Now | Predicted | Why |
|---|--:|--:|---|
| `unpack` C (SlowROM, mclk a call) | 603 353 | 280 000 to 350 000 (about -49 %) | a literal or copied byte: 117 CPU cycles -> 47 (two stagings, two 32-bit steps, a reload gone); plus the token's two stagings |
| `unpack` C, FastROM | 517 898 | same ratio | the instructions executed, not where they are fetched |
| `slots` C | 27 765 | about 22 100 (-20 %) | the walked slot pointer's high half (3 instructions) gone in each of 64 turns |
| `ents` C | 28 253 | about 25 400 (-10 %) | the same, 32 turns of a longer body |
| `col` C | 16 078 | about 13 300 (-17 %) | the same, 32 turns, the pointer kept in `tcc__r9` |
| `fill`, `copy`, `glide`, the others | | unchanged (0 cycles) | no staging and no stepped pointer in their loops |
| bench, weighted mean | 1.91 | about 1.86 | slots 4, ents 6, col 2, unpack 1 of 87 |
| bench, geometric mean of 20 rows | 2.32 | about 2.19 | |
| game 1 and game 2, busy mclk a step | 368 399 / 610 292 | 0 to -0.2 % | their hottest functions (90 % of the C time) have no staging and no `adc.w #0`: the games walk near pointers and index |
| game 1 and game 2, bytes of code | 116 169 / 135 154 | -300 to -1 200 each | 192 and 329 stagings in their units, 20 and 32 high-half steps; perhaps half removable |

## Cost and risk

Part 1 is a text rule run LAST in `relax_branches` (no later rule reads
`[tcc__lf+N]`; every earlier rule assumes a direct-page slot is not reached
through a pointer, `pp_slot_live_at` says so in words), with its own backward
liveness of `tcc__r9`. Part 2 is an IR analysis before emission and a
shorter Kl `add` / `sub`. A mistake in either is silent: the differential
tests (2000 fresh seeds each, 52001-54000), the two fixtures and their
faults, the corpus at equal frame, the games' state at every step.
`QBE_NO_FARPTR_DP=1` turns both off.

## Measured

(filled at the end of the lot)
