# gapbench — what C costs against assembly written by hand

Twelve routines (and four of them written a second way in C) of the kind a game runs every frame, each written twice on the
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

`soa` and `glide` have the shape of the heaviest routines in the luna profiles
of two games built on the SDK (their sources are not here: these are
stand-alone equivalents). The C is what a careful programmer writes (locals
for what a store could alias, a table rather than a variable shift), not C
bent to the compiler.

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
| geometric mean |  |  | 2.55 |  |  | 2.58 |  |  |

A row marked `*` is the same routine written another way in C (`fillu` unrolled by
hand, `coli` / `rowi` / `entsi` with an index instead of a walked pointer), against
the same assembly. They are not in the mean: they are there because the way
the C is written must not matter this much, and because an unrolled fill must
not be slower than the loop.

`copy` through the library's `memcpy`: 24 276 (SlowROM), slower than the C
loop. The factor is the same SlowROM and FastROM: the gap is in the
instructions executed, not in where they are fetched from.

## Reading a row

- A factor is one routine on one data set; the geometric mean is over twelve
  routines chosen by us. Neither is "the cost of C" for a whole game.
- The assembly is ours. A better hand would lower its column and raise the
  factor; nothing here is an upper bound.
- The driver is assembly, so its own cost does not move when the compiler
  changes; the setup and the checksums are C and are outside the window.
