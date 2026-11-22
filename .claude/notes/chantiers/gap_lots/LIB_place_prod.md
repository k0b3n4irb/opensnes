# LIB place prod: the tighter `oamPlaceWorld` made the library's

Follows `LIB_place_m0.md` (branch `lib-place-m0`: the margin measured, about
30 % plain, 24 % order, 22 % order + `visible[]` on 32 sprites, a batch of one
sprite -6.5 % to +4 %). This lot replaces the body of `oamPlaceWorld` in
`lib/source/sprite_world.asm` (Class B, library). Branch `lib-place`, from
`develop`. Pair: Opus writes and measures; the maintainer session re-runs the
gates. Neutral names: "the consumer" is the one game that calls the function.

## Prediction (written 2026-11-15, before any production code; committed alone)

### What will be built

Starting point: `place_hand.asm` attempt 2 (groups of eight unrolled, Y = 2i,
the camera in the direct page offset by `size - 1`, x stored first as a word,
the eight ninth bits written once a group as a word). Four changes to make it
production:

1. **Entry through the data bank**: the batch's bank put in DBR for the
   pointer copies (`lda abs,x`, 10 cycles a word with its store instead of 14
   with `ldy #n / lda [dp],y`), restored before any write to the OAM shadow.
   About 25 CPU cycles a call, every shape.
2. **A straight path for `count == 1`** (size not 0): no loop, no index, no
   camera constants in the direct page; the screen position tested by its
   sign (`bmi` = the left or top edge, else `cmp #256` / `cmp #224`); with
   `order`, the only valid order byte is 0. The library's ninth-bit
   read-modify-write kept.
3. **Partial groups through a checked copy of the group** instead of the
   per-sprite path: entered at sprite `first_id & 7` through a jump table, one
   `cpy end / bcs` a sprite (6 cycles), the group's high-table word written
   with a mask that keeps the size bits and the x bits of the sprites outside
   the batch. Small batches (2 to 7) and the head and tail of a large one go
   through it. Full groups keep the unchecked copy.
4. **The y subtraction sets its carry** (`sec`, 2 cycles a placed sprite):
   attempt 2's tracked carry ("set after `adc 1 - size` when x >= 0") is false
   for size 0 and 1, which the library handles; with it every size 0..255
   gives the library's result. Size 0 does not take the one-sprite path (its
   sign test would accept x = 0).

Contract differences, the three accepted ones and no other: the camera
arguments are not rewritten; a hidden sprite's tile and attribute bytes are
left untouched; up to 30 bytes of `tcc__lf`. Within a call the ninth x bits of
a group of eight land together after the group's entries (attempt 2's design;
the flags `oam_max_id` / `oam_update_flag` are still set last).

### Predicted, master cycles (SlowROM; FastROM about 0.86 of it)

Per sprite, in a batch of 32 (full groups), new vs current:

| Shape | placed new / current | hidden new / current |
|---|---|---|
| plain | 885 / 1 301 | 535 / 936 |
| order | 1 110 / 1 524 | 790 / 1 126 |
| `visible[]` | 950 / 1 367 | 720 / 996 |
| order + `visible[]` | 1 175 / ~1 590 | ~950 / ~1 190 |

(attempt 2's figures, plus the `sec` on a placed sprite; a sprite of a partial
group pays about 50 more.)

A call of one sprite (the whole call), new vs current:

| Shape | placed new / current | hidden new / current |
|---|---|---|
| plain | ~3 650 / 4 370 | ~3 450 / 4 146 |
| order | ~3 800 / 5 016 | ~3 600 / 4 758 |
| `visible[]` | ~3 750 / 4 456 | ~3 550 / 4 225 |

So no loss on one sprite, about -16 %, and the fixed cost as LIB_place_m0
defines it (the one-sprite call less one placed sprite of a batch of 32) at
or under the current one: plain ~2 770 against 3 069, order ~2 690 against
3 492, `visible[]` ~2 800 against 3 089. The fixed cost of a call of 2 or more
sprites (the general path: camera constants, group limits, the jump into the
group, the masked high-table write) is predicted about 400 above the current
one, paid back from the first two sprites.

### The consumer's pass (neutral bench rows, written before the code)

Per pass: one batch of 18 and one of 20 sprites 32x32 with `order` and
`visible[]` (the consumer measured 4.4 placed of 18 on average: 4 or 5
placed, the rest hidden by x and by y, so about 76 % of these two batches go
through the hidden path); one sprite 16x16 with `visible[]` (placed 88 % of
the passes); 5 sprites 32x32, 1 sprite 32x32, 4 and 7 sprites 16x16, 4 sprites
16x16, all with `visible[]` and no order. Two calls always (18 and 20), zero
to five of the others.

Predicted, SlowROM, current -> new:

| Call | current | new | saved |
|---|--:|--:|--:|
| 18, order + `visible[]`, 4.5 placed | ~26 700 | ~22 000 | ~4 700 |
| 20, order + `visible[]`, 4.5 placed | ~29 000 | ~23 900 | ~5 100 |
| 1 x 16, `visible[]`, placed 88 % | ~4 430 | ~3 730 | ~700 |
| 5 x 32, `visible[]`, half placed | ~9 000 | ~7 900 | ~1 100 |
| 1 x 32, `visible[]`, half placed | ~4 340 | ~3 650 | ~700 |
| 4 x 16, `visible[]`, half placed | ~7 800 | ~7 000 | ~800 |
| 7 x 16, `visible[]`, half placed | ~11 350 | ~9 650 | ~1 700 |

**The pass**: the two calls always there, about **9 800 master cycles saved
SlowROM (~18 %, 2.7 % of a frame), ~8 400 FastROM**; with the five others,
about 14 000 (~15 %, 3.9 % of a frame). Reason: the hidden path dominates the
two large batches (76 % of their sprites), and there the margin is the
loop's (the indexes, the per-sprite flag test, the ninth-bit
read-modify-write gone): ~240 a hidden sprite with order + `visible[]`
against ~415 a placed one. Hence a pass gain under LIB_place_m0's 22 % for
32 half-placed sprites: fewer placed sprites, partial groups (18 and 20 are
not multiples of eight), and the general path's higher fixed cost on the
small batches.

Stop rule (the brief's): the one-sprite path losing on any shape or speed
after two attempts, or a gate failing twice unexplained.

## Measured (2026-11-15, luna v1.44.0, `devtools/placebench`, `make bench-place`)

Second agent on the lot: the first one's draft (dispatch first, the
one-sprite path, groups of eight with a checked copy for partial groups,
a per-sprite loop for 2 to 8 sprites) was taken as unverified, built,
passed `world_place` and measured as is. One loss found by the bench: a
one-sprite batch of **size 0** went to the general path (-22 % plain,
-11 % with order). Fixed by a size-0 entry into the one-sprite path (a
sprite at screen x = 0 or y = 0 hidden first, as the general rule does);
the main one-sprite path pays 6 master cycles for the reordered tests.
Every figure below is that final code, every row's old and new checksums
equal (OAM shadow, `visible[]`, `oam_max_id`, `oam_update_flag`).

### Per sprite in a batch of 32, SlowROM (FastROM in brackets)

| Shape | placed predicted / measured / old | hidden predicted / measured / old | held |
|---|---|---|---|
| plain | 885 / 939 (814) / 1 301 | 535 / 588 (509) / 936 | no: +6 % and +10 % over the prediction |
| order | 1 110 / 1 169 (1 018) / 1 524 | 790 / 844 (734) / 1 126 | no: +5 %, +7 % |
| `visible[]` | 950 / 1 004 (871) / 1 367 | 720 / 776 (669) / 996 | no: +6 %, +8 % |
| order + `visible[]` | 1 175 / 1 235 (1 076) / 1 590 | ~950 / 991 (861) / 1 241 | no: +5 %, +4 % |

About 55 cycles a sprite above attempt 2 + `sec`: the production code
pays the dispatch per shape and the partial groups' checked copy (the
32-sprite rows start at id 0, but the bench's `per sprite` subtracts a
one-sprite call that now takes the straight path, which is cheaper than
the general path's fixed cost). The margin against the old body still
holds: 21.8 to 36.5 % on 32 sprites.

### A call of one sprite, SlowROM (FastROM)

| Shape | placed predicted / measured / old | hidden measured / old | fixed (one placed - a placed sprite) new / old |
|---|---|---|---|
| plain | ~3 650 / 3 181 (2 730) / 4 372 | 2 838 / 4 146 | 2 242 / 3 071 |
| order | ~3 800 / 3 286 (2 823) / 5 016 | 2 946 / 4 758 | 2 117 / 3 492 |
| `visible[]` | ~3 750 / 3 264 (2 800) / 4 454 | 2 899 / 4 222 | 2 260 / 3 087 |
| order + `visible[]` | / 3 368 (2 891) / 5 100 | 3 006 / 4 890 | 2 133 / 3 510 |
| size 0, the four shapes | | 3 432 to 3 622 / 4 398 to 5 128 | |

Held, better than predicted: no shape and no speed loses (-21.5 % to
-38.9 %), the fixed cost is under the old one everywhere (condition 2).
Two sprites: -12.7 to -17.4 %; batches of 4 to 7: -8.4 to -12.3 % (the
smallest margins; the per-sprite loop keeps the old read-modify-write of
the high table).

### The consumer's pass (neutral rows `p*`)

| Call | old | new | saved | predicted saved |
|---|--:|--:|--:|--:|
| 18, order + `visible[]`, a quarter placed | 27 542 | 22 174 | 5 368 | ~4 700 |
| 20, order + `visible[]`, a quarter placed | 30 248 | 25 023 | 5 225 | ~5 100 |
| 1 x 16, `visible[]`, placed 88 % | 4 455 | 3 247 | 1 208 | ~700 |
| 5 x 32, `visible[]`, half placed | 9 262 | 8 430 | 832 | ~1 100 |
| 1 x 32, `visible[]` | 4 482 | 3 290 | 1 192 | ~700 |
| 4 x 16, `visible[]`, half placed (twice) | 7 858 | 7 200 | 658 | ~800 |
| 7 x 16, `visible[]`, half placed | 11 739 | 10 430 | 1 309 | ~1 700 |

**The pass**: the two calls always there save **10 593 master cycles
SlowROM (18.3 %, 2.96 % of a frame), 8 699 FastROM (17.5 %, 2.43 %)**;
with the five others, **16 450 SlowROM (15.9 %, 4.60 %), 14 145 FastROM
(15.9 %, 3.96 %)**. Predicted 9 800 / 8 400 and ~14 000: held, a little
above. The small batches (4 to 7) save less than predicted (the per-sprite
loop), the one-sprite calls more (the straight path).

### Contract, size, gates

- Section `.text.oamPlaceWorld` **7 751 bytes** (old 530; predicted about
  4 KB: not held, four shapes times eight unchecked and eight checked
  sprite copies plus four per-sprite loops). SUPERFREE: bank $00 in the
  examples that have room; with bank $00 filled (`testing/bank_spill.py`'s
  filler, 8 000 bytes evicted) it links at `$81:8000` and
  `examples/sprites/world_sprites` shows the same frames (60, 120, 200,
  300, tolerance 0). LoROM, HiROM (`$C0:8790`), SA-1, SuperFX and FastROM
  builds of world_sprites link and pass `opensnes-rom check`.
- `world_place`: PASS; fails with the group word's size bits dropped
  (r_bad 0x5BA2), `visible[0]` written 0 on the one-sprite path (0x2E),
  the ninth bit cleared instead of set on the per-sprite loop (0x298).
- WRAM oracle: world_sprites alone moved; the reason is in
  `testing/baselines/WRAM_JOURNAL.md` (callee scratch, stack band, one
  hidden sprite's tile byte: difference b). luna diff old ROM / new ROM:
  MATCH on every frame compared, idle and with the D-pad held.
- libbench: `world` 31 747 -> 23 767, `worldo` 36 309 -> 27 803 (the two
  rows re-measured alone; the other rows of `baseline.json` left as they
  were: several have moved since their capture for reasons outside this
  lot, `idle` +6.5 %, `worldc` -25 %, `oamfast` -12 %, all within the
  gate).

Replay: `make bench-place`; `testing/bin/luna test
testing/fixtures/stress/world_place/world_place.toml`.
