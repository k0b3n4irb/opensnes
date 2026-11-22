# L7: the library's hot routines against their yardstick

Lot of `../c_asm_gap_plan.md` (item 7). Written before the lot starts, as
asked by the plan: contest, predicted gain on both gauges, cost and risk.
The measured column is filled when each step ends. Examined before L3
(conductor's order of 2026-11-12).

## The axis, re-measured on 2026-11-12

Game 1 spends 18.7 % of its busy time in library assembly: world sprite
placement 9.7 to 10.0 %, VRAM queue flush 3.3 %, NMI handler 2.9 %. Game 2:
NMI handler 2.5 %. Of the plan's other L7 items, two are already done and
one is only a documentation matter:

| Item of the plan | State on `develop` `825a39ab` |
|---|---|
| `oamClear` 143 000 mclk a call | done before this lot: `oamHideRange(0, 128)` is 10 158 (`hide128`), 32 sprites 4 614 (`1b90b2bf`) |
| a full map refresh 797 000 | the column / row paths are 380 / 385 an entry since `cf417385`; the full refresh is not re-measured (step 4) |
| the two "fast" macros slower than the call | the header says so with the figures (`sprite.h`, `oamSetFast` 2 590 against 2 030, `oamSetXYFast` 1 920 against 1 330); removing them is an API decision, left out of this lot |
| world sprite placement | **step 1** below |
| VRAM queue flush | step 2, examined after step 1 |
| NMI handler | step 3, examined after step 2 (`templates/crt0.asm`: Class A) |

## Step 1: `oamPlaceWorld` (`lib/source/sprite_world.asm`)

`libbench` rows: `world` 31 747 mclk for 19 sprites (1 671 a sprite, two
calls), `worldo` 36 309 (with `order`), `worldc` 53 409 (the C loop). Game 1
calls it nine times a frame, with `order` and `visible` on its main batch,
four of the batches of one sprite.

### Where the cycles go (counted on the source, CPU cycles a sprite)

| Path | Cycles | Of which |
|---|--:|---|
| placed, plain | ~171 | five long-indirect reads `[dp],y` (35), the high-table byte read-modified-written (17), two pointer steps (14), group test of the high table (8), the `visible` flag test (5), two camera reads from the stack (10) |
| hidden, plain | ~109 | the same high-table and step costs |
| per call | ~200 | ten long reads to copy the five pointers, clamps, limits |

### Contested before starting

- **The yardstick is not the same work.** The shipped games emit an OAM
  entry in 426 to 643 mclk from screen coordinates they already hold, with
  no culling, from bytes in the direct page or a fixed bank. Ours culls on
  both axes, reads five arrays through 24-bit pointers (they may be in ROM,
  plain RAM or `FAR` RAM) and keeps the ninth bit. The yardstick is a
  floor, not a target: what can be removed is the overhead around the same
  work, not the work.
- **What can be removed, item by item** (cycles a placed sprite):
  1. the high table written once per group of four sprites, the x bits
     gathered in a direct-page byte (`tsb`), the size bits kept by a mask:
     17 + 8 -> about 9;
  2. the camera kept in the direct page, not on the stack: -2 a read;
  3. the loop specialised once per call on `visible` wanted or not: -5
     where it is not wanted, 0 for game 1;
  4. the five reads `[dp],y` become `(dp),y` when every pointer's bank
     reaches plain RAM through the data bank (tested once per call, the
     long path kept for the rest): -2 a read, -10 a sprite. The data bank
     is $00 at run time and $7E in the object engine's callbacks
     (`sprite_oamset.asm`); both map `$0000-$1FFF`, so the test is: bank
     byte $00 or $7E and the array below $2000. Hypothesis to check: the
     game's arrays are plain RAM; the bench's are.
- **Predicted**: placed 171 -> ~135, hidden 109 -> ~90; `world` 1 671 ->
  about 1 330 a sprite (-20 %), `worldo` about the same ratio. Game 1:
  the routine is ~10 % of its busy time, so **-1.5 to -2 % of busy time a
  step**; game 2 does not call it (0 %). Not the plan's "a third": the
  long reads that hold half of the cost stay for the pointers that are not
  in plain RAM, and the culling is work the yardstick does not do.
- **Why it could not pay**: if the game's arrays are not reachable through
  the data bank, item 4 gives nothing: -1 % instead of -2 %. Measured
  first (the bank bytes of its nine batches, one luna read) before item 4
  is written.
- **What it could break**: a wrong ninth bit or a lost size bit is a
  sprite drawn at the wrong place for one frame, silent. Partial groups
  (a batch starting or ending inside a byte of the high table) are where a
  group-wise write goes wrong.
- **How we would notice a mistake**: a new stress fixture runs
  `oamPlaceWorld` on random batches (first id, count, size, cameras on and
  around the limits, `order` with bad bytes, `visible` or not, arrays in
  plain RAM, `FAR` RAM and ROM) and compares the whole OAM shadow and
  `visible[]` with a C model of the documented contract, 10 000 batches;
  it must fail on the current routine with one deliberate error put in
  each path (ninth bit, size bit, partial group). Then every example frame
  for frame, the `world`/`worldo` rows with their checksums, `make tests`.

### Cost and risk

One file of the library (Class B), about 150 lines changed, the fixture
about 200. No API change, no other module touched. Validation: the fixture,
`make tests`, the 65 examples frame-identical, game 1's frozen copy step by
step (state identical, busy time per step before -> after), as for L2.

### Measured (2026-11-12, the examination itself)

The cycle counts above were checked against the 65816 table and against
game 1's profile (gauge build `l2`, 680 frames, per label), and two of the
four items do not hold:

- **Item 4 gives nothing**: `(dp),y` costs 5 cycles plus 1 when the index
  registers are 16 bits wide (they are, all through the routine), `[dp],y`
  6: the same. Moving to near reads only pays with 8-bit index registers,
  which a 512-byte table indexed by X rules out. (The game's eleven arrays
  are indeed plain RAM, `$00:06A6` to `$00:1B53`: the hypothesis held, the
  gain was not there.)
- **Item 1 gives about 2 cycles, not 16**: the byte of the high table must
  still be read, masked and written; gathering the bits in a direct-page
  byte (`tsb`/`trb`, 10 cycles) and writing it once per four sprites (about
  5 a sprite) against 17 today.
- **Where game 1 pays** (mclk a frame, 18 974 in all, 742 instructions):
  the entry of each call 4 970 (26 %, about three calls a frame: copying the
  five pointers is 140 cycles a call and has no cheaper form, a block move
  costing the same 7 cycles a byte); the `order` loop 9 138 (48 %), mostly
  slots hidden by the culling; the plain loop 4 170 (22 %).
- **What is left**: the group of four unrolled (the group test, ~8 cycles a
  sprite), the camera in the direct page (1 a read), a few cycles of entry.
  **Counted ceiling: 10 to 12 % of the routine, about -1 % of game 1's busy
  time; game 2 does not call it.** Hypothesis until measured.

The rest of the plan's 18.7 % is mostly not code:

| Game 1, mclk a frame | Total | Of which DMA stall | Code |
|---|--:|--:|--:|
| `vramQueueFlush` | 6 451 | `@next` 4 463 for 19 instructions | about 2 000 |
| `NmiHandler` | 6 024 | `@oam_done` 3 313 for 3 instructions (544 bytes) | about 2 700 |
| `vramQueuePushSprite` | 3 515 | none | 3 515 |
| `oamPlaceWorld` | 18 974 | none | 18 974 |

Game 2: `NmiHandler` 5 451 (2 942 of it the OAM transfer), the flush 1 557.
The OAM transfer is always 544 bytes; sending only up to `oam_max_id` would
save nothing for game 1, whose `oam_max_id` is 127 in the match (luna
`--peek oam_max_id` at the 300th step), and would change the contract of
every direct writer of `oamMemory[]` (the macros do not raise it).

**Conclusion of the examination**: L7 is worth about -1.5 to -2 % of game 1's
busy time (`oamPlaceWorld` ~1 %, the NMI and flush code ~0.7 % at a third
of their code) and -0.4 % of game 2's. The plan's target (18.7 % -> under
14 %) cannot be reached by code: about 4 of the 18.7 points are bytes moved
by DMA at 8 mclk each. L7 is not opened as a lot now; the `oamPlaceWorld`
unrolling waits as a short lot for a quiet day, on the fixture written for
it (`testing/fixtures/stress/world_place`, 10 000 random batches against a
C model, shown to fail on three deliberate errors). Next in the plan's
order: the private measurement through another 65816 C compiler, then L3.
