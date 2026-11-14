# gapbench — what C costs against assembly written by hand

Twenty routines (and four of them written a second way in C) of the kind a game runs every frame, each written twice on the
same data: in C (`kernels.c`, compiled by cc65816) and in 65816 assembly by
hand (`kernels.asm`). `run.py` builds one ROM, SlowROM and FastROM, and reads
from luna the master cycles of 16 calls of each, interrupts off, minus the same
16 calls of an empty routine. Both versions leave a checksum of everything they
wrote; a row whose two checksums differ gives no figure.

```
make bench-gap                              # the table
python3 devtools/gapbench/run.py --check    # fails if a figure is above baseline.json (in make tests)
python3 devtools/gapbench/run.py --update   # after a measured gain: rewrite baseline.json, and the table below
```

## The routines

| Routine | What it does | What the assembly allows itself that C does not say |
|---|---|---|
| `copy` | 128 words from low RAM to a `FAR` buffer | the block move (`mvn`) |
| `fill` | 256 words of a `FAR` buffer set to one value | eight stores a turn |
| `oam` | 32 objects (structures) to a sprite list: camera, culling, the ninth x bit, the rest hidden | two range tests as one unsigned compare each, an entry as two word stores |
| `col` | 32 tilemap entries down a column of a 64x64 byte map, through a table | no loop: the count is fixed, 32 times four instructions |
| `row` | the same along a row | the same |
| `box` | one box against 32, a flag each and the count | a signed compare read from the N flag (the coordinates are small) |
| `fixed` | 16 positions += velocity (8.8) x scale (0.8), signed | the hardware multiplier (`$4202`), two 8x8 products an element |
| `mul` | 16 products of two bytes | the hardware multiplier, the loop in 8 bits |
| `ents` | 32 structures of 12 bytes: move, count down, turn, die | the structure in X, a field as an offset |
| `call` | 64 calls of a three-argument function | arguments in A, Y and the direct page, a near call |
| `soa` | 18 actors in parallel arrays: bound, move, record as sprites with a depth | a flag used as the amount it stands for |
| `glide` | 48 objects placed between two positions through an easing table, into the OAM mirror | nothing but register choice |
| `slots` | one pass over 64 object slots of 16 bytes, 56 of them empty, a counter in the 8 others | the state byte read in 8 bits, four slots a turn at fixed offsets |
| `state` | 32 objects, a state 0..7 each, eight small handlers that change the object and sometimes its state: a dense `switch` | a table of jumps indexed by the state, the object in Y for the whole update, a signed compare as an unsigned one (x stays above zero) |
| `fptr` | the same handlers, the state being a function pointer kept in the object | a 16-bit code address called through a one-instruction trampoline: the handlers share a bank and return near |
| `blocks` | a row of 16 blocks of 16x16 expanded to two rows of 32 tilemap entries in a ring, from a map of words (block number, flags above) and four words a block | no loop: 16 times the same instructions, the map row at fixed offsets |
| `probe` | 24 points in pixels against that map: the cell, two flags, a result byte each and the count | the cell built from masks kept in place (the map is 64 wide: no multiply, and the two fields do not overlap), the flags read from N |
| `chain` | 16 actors, an update each that calls four or five functions of a line or two between a few lines of its own | near calls, the actor in X across them, arguments in A and Y, a result in the flags, a range test as one unsigned compare |
| `sprites` | 12 objects of 1 to 4 tiles from a frame table, culled a tile, the ninth x bit, written in depth order (an insertion pass on a kept order), the rest hidden | the place in the list in Y from the first tile to the last, two range tests as one unsigned compare each, an entry as two word stores, y compared unsigned (it stays above zero) |
| `unpack` | 512 bytes out of a byte-wise LZ stream (runs of literals, copies from the output) | the block move for literals and copies, the data bank set once, a token and the byte after it read as one word |

`soa` and `glide` have the shape of the heaviest routines in the luna profiles
of two games built on the SDK (their sources are not here: these are
stand-alone equivalents). The C is what a careful programmer writes (locals
for what a store could alias, a table rather than a variable shift), not C
bent to the compiler.

The eight rows from `slots` on are written on patterns read in shipped
games (`.claude/notes/tech/commercial_yardsticks.md`): new code on a generic
shape each, nothing of a game. What they are there for:

- `slots`: what an empty slot costs. 465 master cycles a slot in C, 97 in
  assembly (FastROM): a game pays it for every slot of every pass.
- `state` and `fptr`: the two ways a game dispatches an object. In C the
  second costs 2.5 times the first (45 111 against 17 854, FastROM, the `switch` a jump table since lot L2): each
  handler is a far call with its argument on the stack. In assembly they
  are within 8 % of each other.
- `chain`: the weight of calls. The C version makes 28.9 calls for 1 000
  instructions executed (a luna trace of the row, FastROM); `call` alone is
  a loop of nothing but calls. A code-generation gain that lowers the
  instructions between calls raises this figure: read it again then.
- `unpack`: 481 master cycles a byte produced in C, 87 in assembly
  (FastROM): two far pointers walked a byte at a time against a block move
  (1 102 before lot E1, which reads each pointer in its slot and steps it on
  16 bits).
- `blocks` and `probe` read a map of words above `$2000` (`FAR`), as a
  level map is; `blocks` reads the four words of a block there too.

The byte count of an assembly twin is its entry routine only: the handlers
of `fptr` and the callees of `chain` (and of `call`) are not in it.

## The table (2026-11-14, after lot E1, far pointers read in their slot)

Master cycles a call (the lots between L2 and E1 are in it too; E1 alone moved `unpack`, `slots`, `ents` and `col`). `C/asm` is the factor to bring down. At the start
(2026-11-11) the weighted mean was 2.24, every row equal 2.74, the first
twelve rows 2.55; `baseline.json` in the history holds every figure of that day.

| routine | C | asm | C/asm | C fast | asm fast | C/asm fast | C bytes | asm bytes |
|---|--:|--:|--:|--:|--:|--:|--:|--:|
| copy | 22236 | 13842 | 1.61 | 18522 | 12236 | 1.51 | 25 | 15 |
| fill | 32380 | 16177 | 2.00 | 26536 | 13445 | 1.97 | 32 | 56 |
| fillu * | 17678 | 16177 | 1.09 | 14660 | 13442 | 1.09 | 70 | 56 |
| oam | 41486 | 22893 | 1.81 | 34334 | 18937 | 1.81 | 336 | 169 |
| col | 13178 | 6452 | 2.04 | 11206 | 5412 | 2.07 | 89 | 505 |
| coli * | 13674 | 6452 | 2.12 | 11481 | 5414 | 2.12 | 72 | 505 |
| row | 12848 | 6454 | 1.99 | 11008 | 5412 | 2.03 | 87 | 505 |
| rowi * | 14768 | 6452 | 2.29 | 12512 | 5414 | 2.31 | 74 | 505 |
| box | 17992 | 12408 | 1.45 | 15146 | 10298 | 1.47 | 207 | 127 |
| fixed | 93277 | 10474 | 8.91 | 78994 | 8772 | 9.01 | 223 | 83 |
| mul | 26234 | 7772 | 3.38 | 24035 | 6398 | 3.76 | 78 | 52 |
| ents | 25320 | 14740 | 1.72 | 20976 | 12344 | 1.70 | 169 | 85 |
| entsi * | 18095 | 14738 | 1.23 | 15079 | 12342 | 1.22 | 119 | 85 |
| call | 64494 | 32642 | 1.98 | 56115 | 27880 | 2.01 | 74 | 30 |
| soa | 36377 | 24865 | 1.46 | 30624 | 20809 | 1.47 | 286 | 171 |
| glide | 93704 | 67682 | 1.38 | 78638 | 56049 | 1.40 | 242 | 163 |
| slots | 21928 | 7486 | 2.93 | 17990 | 6194 | 2.90 | 75 | 85 |
| state | 19568 | 16161 | 1.21 | 15876 | 13185 | 1.20 | 297 | 209 |
| fptr | 52486 | 16631 | 3.16 | 45111 | 14130 | 3.19 | 98 | 26 |
| blocks | 24409 | 10673 | 2.29 | 20816 | 8967 | 2.32 | 200 | 829 |
| probe | 29955 | 21351 | 1.40 | 25358 | 17932 | 1.41 | 135 | 96 |
| chain | 83208 | 37106 | 2.24 | 71775 | 31857 | 2.25 | 65 | 18 |
| sprites | 93263 | 51216 | 1.82 | 77892 | 42647 | 1.83 | 545 | 323 |
| unpack | 281068 | 51736 | 5.43 | 246234 | 44475 | 5.54 | 225 | 67 |
| mean, weighted by real games' profiles |  |  | 1.85 |  |  | 1.86 |  |  |
| geometric mean, every row equal |  |  | 2.18 |  |  | 2.19 |  |  |
| geometric mean of the first twelve rows |  |  | 2.09 |  |  | 2.11 |  |  |

A row marked `*` is the same routine written another way in C (`fillu` unrolled by
hand, `coli` / `rowi` / `entsi` with an index instead of a walked pointer), against
the same assembly. They are not in the mean: they are there because the way
the C is written must not matter this much, and because an unrolled fill must
not be slower than the loop.

`copy` through the library's `memcpy`: 15 266 (SlowROM) since it copies with
the block move; it was 24 276, slower than the C loop, when the bench started.
The factor is the same SlowROM and FastROM: the gap is in the instructions
executed, not in where they are fetched from.

## Three means

- **Weighted by real games' profiles** (the one to follow): each row weighs
  what its kind of work takes of the busy time of two games built on the
  SDK, read in their luna profiles: object logic 40 to 50 % (`soa`, `ents`,
  `state`, `fptr`, `slots`, `chain`), sprites 16 to 22 % (`oam`, `glide`,
  `sprites`), boxes and probes 6 to 10 %, tilemaps a few %, calls 6.6 % whole,
  multiply under 1 %, block copies 0.2 %. The weights are `WEIGHTS` in
  `run.py`. Counted on executed instructions, the first twelve rows spent 4.2 %
  of their time on byte accesses where the games spend 0.7 %, and 11.6 % in
  the runtime multiply where the games spend 0.1 to 1.1 %: equal weights
  measured the wrong program.
- **Every row equal**, and **the first twelve rows** (2.55 when the bench
  started): kept for the record, so that an old figure can still be read.

## A second emulator

The ROMs were replayed on Mesen2 (2026-11-11, both speeds, interrupts off as
the ROM sets them): the 34 checksums are the same and the master cycles
between the two marks differ by 0 on every row, the hardware multiplier read
10 and 13 CPU cycles after its write included. Two emulators agreeing is
protection against a drift of one of them, not a console.

## Reading a row

- A factor is one routine on one data set; the geometric mean is over twenty
  routines chosen by us. Neither is "the cost of C" for a whole game.
- The assembly is ours. A better hand would lower its column and raise the
  factor; nothing here is an upper bound.
- The driver is assembly, so its own cost does not move when the compiler
  changes; the setup and the checksums are C and are outside the window.
