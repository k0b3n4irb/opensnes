# LIB place M0: the margin of `oamPlaceWorld` against a tighter hand version

Day 1 of a library lever. The function takes 10.3 % of the busy time of one
game built on the SDK; it is already hand assembly (`lib/source/sprite_world.asm`).
Question, one: how many of its master cycles could a tighter hand version save?
A measurement: no library or compiler file changes. Branch `lib-place-m0`.

Prior examination: `L7_library_hot_routines.md` (2026-11-12) counted a ceiling
of 10 to 12 % of the routine, on the hypothesis that the group-of-four
unrolling and the camera in the direct page were the only levers left.

## Prediction (written 2026-11-15, before any luna run)

### Counted on the source, the current routine

Cycle classes: P = a program byte fetched (8 mclk SlowROM, 6 FastROM),
M = a WRAM data byte (8), I = an internal cycle (6). A, X, Y 16 bits, D = 0.

| Path, plain loop (no `order`, no `visible`) | P | M | I | CPU cycles |
|---|--:|--:|--:|--:|
| placed sprite, x >= 0 | 87.5 (one is the `oam_ext_tab` ROM read) | 53 | 23 | 163.5 |
| hidden at the y test | 56.5 | 29 | 16 | 101.5 |

Where the placed sprite's 163.5 go: the five `[dp],y` reads (31), the two
camera reads from the stack and four `sec` (14), two `beq` + two `bcs` not
taken (8), the high-table byte (`lda.l oam_ext_tab` + `and (r5)` + `sta (r5)`
+ `bra`: 22), the group-of-four test (`txa / and / bne`, ~8), the two
pointer steps `inc.b r0 / inc.b r1` (14), `inx` x4 (8), the loop test (`iny /
cpy / bcs / jmp`: 11), `sep / rep` (6), the `visible` flag test (5).

Shape deltas, a sprite: `visible[]` out +4P +4M (placed) and +4P +4M -I
(hidden); `order` +16P +13M (placed: `ldy slot`, `lda [order],y`, `and`,
`cmp given`, `bcs`, `sta sprite`, `asl`, `tay`, `ldy sprite`, minus the two
pointer steps) and +14P +13M (hidden). Plus ~3 % for the DRAM refresh.

**Predicted, master cycles a sprite** (in a batch of 32, fixed cost excluded):

| Shape | placed slow | placed fast | hidden slow | hidden fast |
|---|--:|--:|--:|--:|
| plain | 1 300 | 1 120 | 805 (y test) / ~1 000 (x test) | 690 / ~860 |
| order | 1 540 | 1 330 | 1 030 | 880 |
| visible[] out | 1 365 | 1 180 | 865 | 740 |

Fixed cost a call (entry, five pointers, clamps, limits, cameras, exit):
about 280 CPU cycles, ~2 200 mclk SlowROM, ~1 900 FastROM; `order` +~40
cycles. FastROM saves only on P: a placed sprite is ~60 % P.

### The tighter version, counted before it is written

1. **One index for the words, halved only for a placed sprite.** Y = 2i:
   the two `inc.b r0 / r1` go (14 cycles every sprite); a placed sprite pays
   `tya / lsr / tay` before the byte reads and `tya / inc / asl / tay` after
   (14 cycles), a hidden one `iny / iny` (4).
2. **x before y, both stored as words**: the x store spills into the y byte,
   the y store into the tile byte, each rewritten next; tile and attr as one
   word (attr read as a word one byte early, tile loaded under it in 8 bits).
   One `sep / rep` pair stays.
3. **The camera in the direct page, pre-offset by one**: `world + (size - 1
   - cam)` with C clear gives one unsigned compare a side (the `beq` goes),
   and `adc` of `-(size - 1)` after a `bcs` not taken (C clear) replaces
   `sec / sbc`: about 6 cycles a side.
4. **Groups of four unrolled** for the aligned full groups: the OAM offsets
   fixed (`oamMemory+4k,x`, X += 16 a group), the ninth bits gathered with
   `tsb` of an immediate mask in a direct-page byte and the high-table byte
   written once a group (size bits kept by `and #$AA`); partial groups by a
   per-sprite path like today's.

Counted: placed ~108 CPU cycles (~890 mclk slow), hidden at x ~55 (~440).

**Predicted margin** (32 sprites, half placed, fixed cost included): plain
**25 to 35 %**, `visible[]` 25 to 35 %, `order` **20 to 30 %** (the order
indirection itself stays). That is above the brief's 10 to 30 % and well
above L7's 10 to 12 %: L7 kept the pointer steps and the byte-wide x store,
which this count removes. Hypothesis until measured; the stop rule is under
5 % in the main shape after two attempts.

## Measured (2026-11-15, luna, `devtools/placebench`)

The bench: 32 sprites (16 x 16 and 8 x 8), about half placed, some across
the left and top edges, half the hidden ones hidden by x, half by y; one bad
order byte in the mixed rows; 16 calls a row, interrupts off, minus a kernel
with the same pushes and a call to an `rtl`. Checksum: the whole OAM shadow,
`visible[]`, `oam_max_id`, `oam_update_flag`; every row's checksums equal.
One exclusion, found by the first run: a hidden entry's tile and attribute
bytes (the library's y word store spills into the tile byte of a sprite it
then hides by x; the stress fixture's model already calls those bytes
"don't care"). `place_hand` also passes the stress fixture (10 000 random
batches: arrays in ROM, `FAR` and plain RAM, sizes 8 to 64, partial groups,
bad order bytes) and fails it on each of three deliberate errors (size bits
lost by the group write of attempt 1, of attempt 2, the carry not set after
a left sprite).

### Prediction against measure, the library (master cycles a sprite)

| | predicted slow / fast | measured slow / fast | held |
|---|---|---|---|
| plain, placed | 1 300 / 1 120 | 1 301 / 1 121 | yes |
| plain, hidden (half by x, half by y) | ~900 / ~775 | 936 / 803 | yes (4 %) |
| order, placed | 1 540 / 1 330 | 1 524 / 1 310 | yes |
| order, hidden | 1 030 / 880 | 1 126 / 963 | no (+9 %) |
| visible[], placed | 1 365 / 1 180 | 1 367 / 1 179 | yes |
| visible[], hidden | ~960 / ~830 | 996 / 854 | yes (4 %) |
| fixed a call | ~2 200 / ~1 900 | ~3 070 / ~2 660 | no (+40 %) |

(Per sprite = (32 sprites - 1 sprite) / 31; "fixed" = the one-sprite row
less one placed sprite.)

### The margin, `place_hand` against the library (32 sprites, mixed)

| Shape | attempt 1 slow / fast | attempt 2 slow / fast | predicted |
|---|---|---|---|
| plain (main) | 25.7 % / 25.2 % | **30.3 % / 29.8 %** | 25-35 %: held |
| order | 19.7 % / 18.7 % | **24.1 % / 23.0 %** | 20-30 %: held |
| visible[] out | 20.3 % / 19.8 % | **24.7 % / 24.2 %** | 25-35 %: not held |
| order + visible[] | 17.8 % / 16.9 % | **21.9 % / 20.9 %** | |
| batch of one sprite | -20 % to -7 % | **-6.5 % to +4 %** | |

Attempt 2, a sprite, slow / fast: placed 870 / 757 (library 1 301 / 1 121),
hidden 535 / 465 (936 / 803); with order placed 1 097 (1 524). Attempt 1
was 932 / 571; attempt 2 added the groups of eight (the high table written
as a word), the carry tracked rather than set, `bmi` not taken on the common
path, and a short path for batches under eight (attempt 1 lost 20 % on one
sprite).

### Where the 431 master cycles of a placed sprite go (plain, SlowROM)

Counted by instruction (the sum, 428, matches the measure, 431):

| Source | Library | Hand | Saved |
|---|---|---|--:|
| loop and indexes: `inc r0`, `inc r1`, `inx` x4, the group test, `iny / cpy / bcs / jmp` | 322 | halve / restore Y (98), `bra` (22), group share (18) | 184 |
| ninth bit: `lda.l oam_ext_tab` + `and (r5)` + `sta (r5)` + `bra` | 158 | `bmi` (16) + group-of-eight word write share (36) | 106 |
| `visible` flag test (`lda OW_FLAGS / beq`) every sprite | 46 | 0 (one loop a shape) | 46 |
| camera on the stack + `sec` (two sides) | 104 | `adc` / `sbc` from the direct page, carry tracked (64) | 40 |
| `beq` on screen position + size = 0 (two sides) | 32 | 0 (offset by one: one compare) | 32 |
| `sec / sbc size` back to the screen position (two sides) | 92 | `adc 1 - size`, carry known (64) | 28 |
| x stored as a word (no byte store) | 38 | 46 | -8 |

A hidden sprite saves 401 (the loop and the flag test, 308, are most of it).
The `order` indirection (~225 a sprite, `ldy slot / lda [order],y / and /
cmp / bcs / sta / asl / tay / ldy sprite / inc slot`) is the same in both:
that is why the order shape's margin is lower. Not taken: the one-byte-early
word read of attr (tile and attr in one store, -22 a placed sprite) reads a
byte outside the caller's array.

### What a change of the library function would be worth

- 32 sprites, one call: 12 000 master cycles saved SlowROM (10 150
  FastROM) in the plain shape, 10 650 with order and `visible[]`: 3.4 % and
  3.0 % of a 60 fps frame (357 368 master cycles).
- The game: 10.3 % of its busy time is in this function; that share,
  scaled by the measured margin, is **2.3 to 3.1 points** of busy time
  (22 % order + visible[] to 30 % plain). Weighted by where it spends it
  (L7: order loop 48 %, plain loop 22 %, entries 26 %, four of its nine
  batches one sprite, where the margin is about 0): **about 1.8 points**.
- Cost: one library file, unrolled loops (the hand section is 4 338 bytes
  against the library's 530, measured in the bench's `.sym`), a Class B change on the existing fixture.

## Replay

```
git checkout lib-place-m0
python3 devtools/placebench/run.py --json /tmp/placebench.json   # both ROM speeds, both versions
python3 devtools/placebench/run.py --fixture                     # place_hand against the contract's model
git show 4e96c9b0:devtools/placebench/place_hand.asm             # attempt 1
```

## Verdict of the maintainer session (2026-11-15)

Replayed by me on the branch `lib-place-m0` (`devtools/placebench/run.py --fixture` PASS `world_place`; the rows read as the pair's: plain 1 301 / 936 slow and 1 121 / 803 fast a placed / hidden sprite for the library, 870 / 535 and 757 / 465 for the tighter hand version; 32-sprite batches 20 to 28 % faster; **a one-sprite batch is -6 % slower** (the fixed cost of the call rises from about 3 070 to 3 780 slow)). The prediction was committed alone before any run (`9c6c15ac`).

What it settles:
1. **The function is not at its floor**: about 30 % in the main shape (24 % with the order table, 22 % order plus `visible[]`), from loop and index work (184 master cycles a placed sprite), the high-table read-modify-write (106), the per-sprite `visible` flag test (46), the camera read from the stack (40) and two compare shapes (60). A tighter version is worth **12 000 master cycles for 32 sprites slow (3.4 % of a 60 fps frame)** and, scaled by the real mix of calls (one call in four is a one-sprite batch, where the margin is nil), about **1.8 points of game 1's busy time** (2.3 to 3.1 with the best mix).
2. **The price**: the section grows from 530 to 4 338 bytes (unrolled by 8) and the new version differs from the contract's side effects in three documented ways (it does not rewrite the camera arguments on the stack, leaves the tile and attribute bytes of a hidden sprite untouched where the library may write a stray tile byte, uses 30 bytes of `tcc__lf` instead of 14 of the 40 allowed). It is a Class B change (library), with the `world_place` fixture as its net; to land it needs a one-sprite fast path (or the loss on single sprites made zero), the ABI note updated for the camera arguments, and the games' census of call shapes confirmed by their sessions.
3. **This is a library lot, cheaper per point than any pass of the generator**: about 1.8 points of one game for 2 to 3 days (a production version with the one-sprite path, the fixtures, the gates), against about 1 point per game for 3 to 6 days a pass in the compiler. It serves one game (the other places its sprites in its own C), and a second game would hit it too (the function is the SDK's world-sprite batch).
