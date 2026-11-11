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
  second costs 1.9 times the first (46 457 against 24 829, FastROM): each
  handler is a far call with its argument on the stack. In assembly they
  are within 8 % of each other.
- `chain`: the weight of calls. The C version makes 28.9 calls for 1 000
  instructions executed (a luna trace of the row, FastROM); `call` alone is
  a loop of nothing but calls. A code-generation gain that lowers the
  instructions between calls raises this figure: read it again then.
- `unpack`: 1 102 master cycles a byte produced in C, 87 in assembly
  (FastROM): two far pointers walked a byte at a time against a block move.
- `blocks` and `probe` read a map of words above `$2000` (`FAR`), as a
  level map is; `blocks` reads the four words of a block there too.

The byte count of an assembly twin is its entry routine only: the handlers
of `fptr` and the callees of `chain` (and of `call`) are not in it.

## The starting table (2026-11-11, compiler pins of that day)

Master cycles a call. `C/asm` is the factor to bring down.

| routine | C | asm | C/asm | C fast | asm fast | C/asm fast | C bytes | asm bytes |
|---|--:|--:|--:|--:|--:|--:|--:|--:|
| copy | 22276 | 13842 | 1.61 | 18563 | 12236 | 1.52 | 26 | 15 |
| fill | 40928 | 16177 | 2.53 | 34018 | 13442 | 2.53 | 39 | 56 |
| fillu * | 49038 | 16177 | 3.03 | 41334 | 13442 | 3.07 | 142 | 56 |
| oam | 47734 | 22893 | 2.09 | 39608 | 18937 | 2.09 | 368 | 169 |
| col | 28912 | 6452 | 4.48 | 24574 | 5414 | 4.54 | 122 | 505 |
| coli * | 22814 | 6452 | 3.54 | 19170 | 5412 | 3.54 | 95 | 505 |
| row | 19152 | 6452 | 2.97 | 16314 | 5412 | 3.01 | 107 | 505 |
| rowi * | 20046 | 6452 | 3.11 | 16927 | 5414 | 3.13 | 87 | 505 |
| box | 20710 | 12408 | 1.67 | 17538 | 10298 | 1.70 | 210 | 127 |
| fixed | 94370 | 10474 | 9.01 | 79942 | 8769 | 9.12 | 231 | 83 |
| mul | 27123 | 7772 | 3.49 | 24692 | 6398 | 3.86 | 85 | 52 |
| ents | 34021 | 14738 | 2.31 | 28413 | 12344 | 2.30 | 195 | 85 |
| entsi * | 23919 | 14738 | 1.62 | 19893 | 12342 | 1.61 | 147 | 85 |
| call | 69380 | 32642 | 2.13 | 60474 | 27880 | 2.17 | 76 | 30 |
| soa | 40259 | 24865 | 1.62 | 33916 | 20809 | 1.63 | 312 | 171 |
| glide | 106396 | 67682 | 1.57 | 89510 | 56049 | 1.60 | 259 | 163 |
| slots | 35627 | 7486 | 4.76 | 29758 | 6196 | 4.80 | 87 | 85 |
| state | 30293 | 16161 | 1.87 | 24829 | 13187 | 1.88 | 392 | 209 |
| fptr | 54077 | 16628 | 3.25 | 46457 | 14132 | 3.29 | 98 | 26 |
| blocks | 29072 | 10673 | 2.72 | 24850 | 8964 | 2.77 | 223 | 829 |
| probe | 33588 | 21351 | 1.57 | 28549 | 17930 | 1.59 | 145 | 96 |
| chain | 86079 | 37106 | 2.32 | 74337 | 31857 | 2.33 | 71 | 18 |
| sprites | 108212 | 51216 | 2.11 | 90571 | 42647 | 2.12 | 643 | 323 |
| unpack | 657068 | 51736 | 12.70 | 564302 | 44475 | 12.69 | 381 | 67 |
| mean, weighted by real games' profiles |  |  | 2.24 |  |  | 2.26 |  |  |
| geometric mean, every row equal |  |  | 2.74 |  |  | 2.77 |  |  |
| geometric mean of the first twelve rows |  |  | 2.55 |  |  | 2.58 |  |  |

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
