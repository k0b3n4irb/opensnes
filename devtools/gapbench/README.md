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

## The table (2026-11-12, after the first compiler lot)

Master cycles a call. `C/asm` is the factor to bring down. At the start
(2026-11-11) the weighted mean was 2.24, every row equal 2.74, the first
twelve rows 2.55; `baseline.json` in the history holds every figure of that day.

| routine | C | asm | C/asm | C fast | asm fast | C/asm fast | C bytes | asm bytes |
|---|--:|--:|--:|--:|--:|--:|--:|--:|
| copy | 22276 | 13842 | 1.61 | 18563 | 12236 | 1.52 | 26 | 15 |
| fill | 32422 | 16180 | 2.00 | 26574 | 13442 | 1.98 | 33 | 56 |
| fillu * | 17718 | 16177 | 1.10 | 14698 | 13445 | 1.09 | 71 | 56 |
| oam | 42886 | 22893 | 1.87 | 35544 | 18937 | 1.88 | 340 | 169 |
| col | 16582 | 6452 | 2.57 | 14080 | 5412 | 2.60 | 98 | 505 |
| coli * | 14178 | 6454 | 2.20 | 11918 | 5414 | 2.20 | 74 | 505 |
| row | 12890 | 6452 | 2.00 | 11046 | 5412 | 2.04 | 88 | 505 |
| rowi * | 14771 | 6452 | 2.29 | 12512 | 5414 | 2.31 | 74 | 505 |
| box | 18469 | 12408 | 1.49 | 15560 | 10298 | 1.51 | 206 | 127 |
| fixed | 93277 | 10474 | 8.91 | 78991 | 8772 | 9.00 | 223 | 83 |
| mul | 26234 | 7772 | 3.38 | 24035 | 6398 | 3.76 | 78 | 52 |
| ents | 30667 | 14740 | 2.08 | 25657 | 12342 | 2.08 | 182 | 85 |
| entsi * | 18735 | 14738 | 1.27 | 15644 | 12342 | 1.27 | 127 | 85 |
| call | 66876 | 32642 | 2.05 | 58233 | 27880 | 2.09 | 74 | 30 |
| soa | 37082 | 24865 | 1.49 | 31233 | 20809 | 1.50 | 291 | 171 |
| glide | 98814 | 67680 | 1.46 | 83062 | 56049 | 1.48 | 249 | 163 |
| slots | 32330 | 7486 | 4.32 | 27121 | 6194 | 4.38 | 82 | 85 |
| state | 22753 | 16161 | 1.41 | 18394 | 13185 | 1.40 | 346 | 209 |
| fptr | 52486 | 16631 | 3.16 | 45111 | 14130 | 3.19 | 98 | 26 |
| blocks | 24452 | 10673 | 2.29 | 20854 | 8964 | 2.33 | 201 | 829 |
| probe | 30319 | 21351 | 1.42 | 25676 | 17930 | 1.43 | 134 | 96 |
| chain | 84381 | 37106 | 2.27 | 72835 | 31860 | 2.29 | 65 | 18 |
| sprites | 97178 | 51216 | 1.90 | 81387 | 42647 | 1.91 | 568 | 323 |
| unpack | 633456 | 51736 | 12.24 | 545960 | 44472 | 12.28 | 362 | 67 |
| mean, weighted by real games' profiles |  |  | 2.00 |  |  | 2.02 |  |  |
| geometric mean, every row equal |  |  | 2.41 |  |  | 2.43 |  |  |
| geometric mean of the first twelve rows |  |  | 2.20 |  |  | 2.23 |  |  |

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
