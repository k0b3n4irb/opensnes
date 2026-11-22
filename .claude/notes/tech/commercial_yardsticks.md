# What shipped games do: three commercial yardsticks (2026-11-11)

Approach A9 of `chantiers/c_asm_gap.md`. Three commercial games of the
console, written in assembly, were read by execution trace on luna
(`luna state --trace-log`, exact on register widths) in scenes of play:
**E1** a platformer, **E2** an exploration game with
8-direction scrolling, **E3** a top-down action game with 8-direction
scrolling. The disassembly, the addresses and the titles stay outside this
repository; here are only patterns in our words and figures measured by us.
Figures are master cycles (mclk); E1 and E2 run SlowROM, E3 FastROM, each
compared with the same column of `devtools/gapbench`. One scene per game.

## The finding that changes the target

**Shipped assembly is not faster than our hand-written assembly; it is
often slower. It wins by doing less work a frame.** An instruction costs
the same everywhere (26 to 28 mclk in the games, 24 to 28 in our C): the
gap between our C and assembly is a number of instructions, not their price.

| Job (per unit) | E1 | E2 | E3 | our C | our assembly |
|---|--:|--:|--:|--:|--:|
| OAM entry, emission alone | ~640 | 643 | 426 | 1 238 to 1 492 (slow), 2 002 a placed sprite with culling (fast) | 592 to 715 (slow), 838 (fast) |
| tilemap entry of a new row or column, block expansion included | 304 | 256 to 274 | 315 | 472 (row, fast) to 903 (col, slow), no expansion | 169 to 202, no expansion |
| the same through our library's map engine | | | | 938 an entry (`map.asm`, measured on `examples/maps/map_scroll`) | |
| box against box | 865 a pair rejected on the first axis; ~2 600 a full test | | 522 a rejected pair | 460 (fast), 647 (slow) a full test | 322 / 388 |
| empty object slot | 612 | 248 (137 for a projectile slot) | 222 to 288 a pass, three passes | 262 (index) to 442 (walked pointer), fast | 166 |
| hide an OAM entry | 34 | | 111 | `oamHideRange` 307 | |
| a table recomputed per element (byte table indexed by a word, four byte stores) | | | 501 | 1 446 (measured by a partner port) | |
| position += velocity, sub-pixels | ~808 an axis, no product | | none: whole pixels | 5 898 with a 16x8 product (slow) | 655 |
| decompression, per byte produced | 404 | 276 | | not measured at equal work | |

Read with care: "object", "OAM entry" and "box" do not cover the same work
on both sides; rows written on the games' patterns are owed to the bench
(below). What holds: on boxes our C already beats two shipped games; on an
OAM entry it is 2 to 3 times theirs; on a tilemap entry 1.5 to 3 times; our
library's map engine is 3 times theirs; on a per-element table 2.9 times.

## How the code is written (counted on the traces)

| | E1 | E2 | E3 | our C | our assembly |
|---|--:|--:|--:|--:|--:|
| `rep` + `sep` per 1000 instructions | 22.2 | 22.9 | 52.2 (39.8 outside one loop) | 75.3 | 36.2 |
| instructions with A in 8 bits | 86.7 % | 6 to 7 % | 22.9 % | 10.2 % | 9.9 % |
| instructions with X/Y in 8 bits | 96.5 % | 7 to 8 % | 0.5 % | 0 | 0 |
| calls per 1000 instructions | 35.5 | | 9.7 | 4.0 | 6.6 |
| share of near calls (`jsr`) | 85.7 % | 58 to 64 % | 82.4 % | 0 % | 100 % |
| memory operands in the direct page | 40.1 % | 17 to 25 % | 25.7 % | 59.0 % | 17.7 % |
| absolute (mostly indexed) | 57.9 % | 78 % | 38.3 % | 20.6 % | 55.9 % |
| long | 2.0 % | | 13.7 % | 12.3 % | 19.5 % |
| stack-relative | 0 % | 0.1 % | 0.5 % | 8.1 % | 0 |
| busy share of a loaded frame | 79 % (NMI 8 %) | logic 49 to 60 %, NMI 26 to 33 k | 3 frames a game step | | |

- **One width a loop.** A game born 8-bit and one born 16-bit switch width
  equally rarely (22 per 1000); our C switches 75 times while doing half as
  much 8-bit work. The rule is not "live in 8 bits", it is "choose a width
  for the routine". A byte that is only tested is read as a word and masked.
- **The current object sits in an index register for its whole update,
  subroutines included; a field is an absolute-indexed offset.** Our C walks
  a 32-bit pointer in memory (59 % of its operands are its own temporaries).
- **Arguments in A, X, Y and a dozen fixed direct-page bytes; the stack
  holds saved registers only.** Calls are near inside a bank. The bench has
  9 times fewer calls than E1: it under-weights the calling convention.
- **The data bank is set once**; nobody switches it per element.
- **The state of an object is a code pointer stored in it, or a jump table
  indexed by type**; the commonest state is tested first by a plain branch.
- **Early outs everywhere**, work spread over frames by parity, slots found
  empty in 15 to 21 instructions.
- **The hardware multiplier** has its first operand written once before a
  loop and its product read after useful work; one game also uses the
  signed 16x8 multiplier of the video chip. Two of the three have no fixed
  point at all: speeds are table or script values.

## What the SDK lacks or does worse (to do)

| # | Where | Finding | Belongs to |
|---|---|---|---|
| 1 | `lib/source/map.asm` | 938 mclk an entry against 256 to 315 with block expansion in the games; switches the data bank per entry; no 16x16 mode, where an entry is a mask and a shift | library (A7, and the scrolling brick) |
| 2 | `oamHideRange` | 307 an entry against 34 to 111 (unrolled stores) | library |
| 3 | `oamSet` | rebuilds the attribute byte from seven stack arguments at every call (~400 mclk); an entry taking the composed attribute | library |
| 4 | `NmiHandler` (`templates/crt0.asm`) | five width switches for the OAM transfer alone | templates |
| 5 | compiler | a byte read costs a `sep` / `rep` and a mask; width chosen per access | A6a (running), then width per region |
| 6 | compiler | an object pointer is 32 bits walked in memory; fields read long-indexed | A5 / A6a |
| 7 | compiler | arguments on the stack, far calls only | A5 |
| 8 | compiler | a dense `switch` is a chain of compares that reloads the value at each node; no jump table is ever emitted | new: jump tables |
| 9 | compiler | range test as two signed compares; an `&&` chain made a 0/1 value | A6a |
| 10 | compiler / runtime | `u8 * u8` and 16x8 through the general routines | A7 |
| 11 | library | no depth sort offered for sprites (one game sorts by a binary tree at 821 mclk an insertion; a bucket sort on y would be cheaper) | library, to propose |
| 12 | library | sending is per module; the games present tilemap, OAM, sprite page and scroll in one VBlank, only when the step is done | the scrolling brick, `gameloop` |
| 13 | library | the VRAM queue counts its cost ahead; one game re-reads the beam after each entry and stops at the end of the blank | option of `vramQueueFlush` |

## The scrolling architectures (for the brick)

- **E2**: ring of 64x32 tilemap entries; a new column is 16 blocks, a row 17,
  each block expanded through a table of four words into two parallel
  buffers; the wrap is resolved when the buffer is built; four transfers a
  strip. Column 17 563 mclk, row 17 422; worst frame (diagonal) 39 002 of
  building, 8 transfers and 264 bytes in the blank.
- **E3**: 16x16 background tiles, tilemap 32x32 (a torus of 512 px), the
  level in RAM as one word a block (number low, terrain flags high: display,
  collisions and AI read the same map), a full RAM mirror of the tilemap,
  four bounds saying which 32 rows and columns are loaded; a new line of 32
  is built only when the camera leaves the loaded window (hysteresis: 7 rows
  in 600 frames of walking), 10 094 mclk; a queue of dirty lines rebuilds a
  row when a block changes. It resends the 15 visible rows (960 bytes) at
  every step, moving or not: simple, and wasteful.
- For us: keep the mirror, the window with hysteresis and the atomic
  presentation; send only the new line (what our map engine does); offer
  16x16 tiles.

## What the games do worse than we do

Positions split in two byte arrays and re-joined for every 16-bit test (E1);
three passes over a hundred slots of which ninety are empty, no active list
(E3); a navigation grid recomputed every step, 38 % of the logic of the
scene (E3); every sprite tile and the visible tilemap resent each step (E3);
the OAM high table rebuilt whole each frame (E1); one subroutine call a
source byte in the decompressor (E1); busy waits instead of `wai` (all
three); a loop counter pushed on the stack each turn for want of a third
register (E3). One ordinary enemy costs 735 instructions a frame in E1 and
7 062 mclk in E2.

## Rows the bench owes to these games

An empty slot and an off-screen object; dispatch by state (code pointer in
the object, jump table by type, dense `switch`); a multi-tile object to the
OAM with depth order; a row and a column with 16x16 block expansion from a
map of words; a block-map collision probe; decompression per byte produced;
a per-element byte table indexed by a word feeding four byte arrays; a call
chain of small functions (the games call 10 to 35 times per 1000
instructions).

## Limits

One scene per game, on an emulator (luna; Mesen2 agrees to the master cycle
on our bench), not on a console. Gains quoted per pattern in the private
reports are cycle counts, not measurements. E3's column path and ninth x bit
were not executed in the scene. Not looked at yet: sound, HDMA tables,
level loading.
