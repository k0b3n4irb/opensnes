# Measured frame costs of the library {#perf}
What the library functions cost per frame in real scenes, measured on luna
(cycle-accurate) with OpenSNES 0.49.
`PHILOSOPHY.md` asks every API with a cost to say it; this page is the
measured side of that promise, and `docs/BENCHMARK.md` the compiler side.

**Unit: master cycles (mclk) per frame.** These are the figures every
page of this documentation uses:

| Quantity (NTSC) | Value |
|---|---|
| Master clock | 21.477 MHz, 60.1 frames a second |
| Scanline | 1364 mclk, of which the CPU loses 40 to DRAM refresh |
| Frame | **357,368 mclk** (262 lines; every other frame one line is 4 mclk shorter) |
| VBlank, 224-line display | 37 lines (225-261): **50,468 mclk**, 48,988 of them outside the refresh pause |
| VBlank, 239-line display (overscan) | 22 lines: 30,008 mclk, 29,128 outside the refresh pause |
| 1 % of a frame | about 3,574 mclk |

Sources: anomie's S-CPU timing doc and fullsnes, *Timing H/V Counters*
(1364 mclk a line, 262 lines, the 1360-mclk short line); snesdev wiki,
*Timing* (the 40-mclk refresh pause, VBlank from line 225 or 240 to 261).

## The scenes

| Scene | What runs | Idle (in `WaitForVBlank`) |
|---|---|---|
| `examples/games/likemario` | walking right: map scroll, dynamic sprite, animation, SNESMOD | 88 % of the frame |
| `examples/games/rpg` | walking: map, sprites, HUD | 91 % of the frame |
| `examples/sprites/sprite_swarm` | many sprites moved every frame (direct OAM writes) | 69 % of the frame |
| `examples/games/tetris` | playing: board redraw, HUD, SNESMOD | 81 % of the frame |
| `examples/audio/snesmod_music` | music playing, nothing else | 98 % of the frame |

Idle time is what the game still has.

## Per function

Mean mclk per frame over 300 frames, and in brackets an upper bound of the
worst frame. A function appears only in the scenes that call it, and only
above 300 mclk per frame.

| Function | likemario | rpg | sprite_swarm | tetris | snesmod_music |
|---|---|---|---|---|---|
| `NmiHandler` | 8,162 (8,170) | 8,163 (8,170) | 7,670 (7,716) | 5,676 (5,676) | 5,650 (5,650) |
| `oamSet` |  | 3,426 (4,068) |  |  |  |
| `animTick` | 3,387 (5,306) |  |  |  |  |
| `animPlay` | 2,125 (3,214) |  |  |  |  |
| `dmaCopyVram` |  |  |  | 1,747 (2,248) |  |
| `oamSetSize` |  | 1,736 (1,970) |  |  |  |
| `oamDynamicDraw` | 1,351 (1,626) |  |  |  |  |
| `oamHide` |  | 624 (4,290) |  |  |  |
| `bgSetScroll` | 561 (600) | 589 (640) |  | 560 (560) |  |

## Reading it

- **`NmiHandler`** is the fixed price of a frame: 5,650-8,170 mclk (1.6-2.3 %
  of the frame, 11-16 % of the VBlank). The OAM upload it does every frame
  is 544 bytes of DMA, at 8 mclk per byte (anomie's timing doc: "DMA takes
  8 master cycles per byte transferred") about 4,350 of those; the
  17th-bit pad read is about 380 more.
- **Sprites**: in the RPG, `oamSet` + `oamSetSize` + `oamHide` cost about
  5,800 mclk per frame (1.6 %) for its handful of characters and HUD
  sprites (the setters are assembly). `oamSetFast` / `oamSetXYFast` (macros, `sprite.h`) or writing
  `oamMemory[]` directly (sprite_swarm) are the escape hatches for large
  counts.
- **Animation**: `animTick` + `animPlay` about 5,500 mclk per frame in
  likemario (both are compiled C).
- **`bgSetScroll`**: about 570 mclk per frame in each of the three games.
- **Pads**: `padPressed` / `padHeld` do not appear: they are macros over
  the word the NMI handler filled.

The per-function figures are what luna counts at the function's own
labels. Asm routines a function calls under their own names (the map
engine's `map_prepare_column`, SNESMOD's SPC transfer routines) are not
added to it, and code the compiler inlined into the caller is counted in
the caller.

## What a call costs

A few small functions (`fixSin`, `fixCos`, `textSetPos`, `setScreenOn`,
`setScreenOff`, `getBrightness`, the colour-math and mosaic setters, the
two ease functions…) are ordinary library functions, not `inline` bodies in
their headers: an inline body would need its variables in the public
header, under names a game would collide with (`sine_table`, `cursor_x`).
The one that runs per frame is `fixSin` / `fixCos`, written in assembly so
that a call costs about 96 master cycles more than an inline lookup:
`examples/backgrounds/mode2`, which calls `fixSin` 32 times a frame, spends
18.5 % of its frame busy. The others run at setup or once per frame, where
a call is not measurable.

`oamDrawMetasprite()` takes the base tile, palette and size from a
`MetaspriteStyle`; it is assembly and writes each piece itself. Two
metasprites of eight 16×16 pieces, drawn every frame, cost 27,470 master
cycles a frame.

## Against PVSnesLib, call for call {#perf-libbench}

`devtools/libbench` asks both libraries for the same things — what a game
asks every frame — and times each request on luna. Master cycles for one
frame's worth; `docs/BENCHMARK.md` has the same comparison for the
compilers.

<!-- libbench:begin -->
| Row | What it asks for | PVSnesLib | OpenSNES | |
|---|---|---:|---:|---:|
| `idle` | a frame that only waits (the SDK's vblank handler) | 7,184 | 5,780 | -19.5 % |
| `pad` | held, pressed, released of pad 0, ten times | 11,427 | 9,702 | -15.1 % |
| `scroll` | `bgSetScroll` on three backgrounds, ten times | 39,540 | 27,336 | -30.9 % |
| `oamset` | `oamSet` on 32 sprites | 81,061 | 64,937 | -19.9 % |
| `oamxy` | `oamSetXY` on 32 sprites | 45,048 | 42,446 | -5.8 % |
| `oamsize` | the size of 32 sprites | 48,744 | 28,870 | -40.8 % |
| `dma` | 2 KB to VRAM, one `dmaCopyVram` | 17,897 | 17,858 | -0.2 % |
| `text` | 20 characters printed and shown | 84,678 | 17,438 | -79.4 % |
| `frame` | one frame: pad, three scrolls, 32 sprites, 20 characters | 169,798 | 91,339 | -46.2 % |
| `worldc` | 19 world-space sprites placed by a loop in C (the same source) | 129,332 | 53,877 | -58.3 % |
| `world` | the same 19 sprites: `oamPlaceWorld` here, the C loop there | 129,332 | 31,740 | -75.5 % |
| `vramc` | six 128-byte VRAM transfers, six `dmaCopyVram` calls | 17,395 | 15,888 | -8.7 % |
| `vramq` | the same six, the part paid in VBlank: one `vramQueueFlush` here, the calls there | 17,395 | 10,790 | -38.0 % |
| `worldo` | the 19 sprites sorted by depth (an order array): `oamPlaceWorld` here, the C loop there | 136,701 | 36,297 | -73.4 % |

OpenSNES costs no more than PVSnesLib on **14 of the 14 rows**. PVSnesLib at `fa758c9b 2025-12-28`.
<!-- libbench:end -->

**How a row is timed.** Each row is built twice per SDK, with its library
calls and with the same loops around nothing; both run the same number of
frames, and the cost is what the symbols of the first gained over the
second. Both SDKs build the scene as LoROM, SlowROM (the build's default,
`USE_FASTROM=0`): a FastROM game pays about a quarter less for the
instruction fetches of these routines, on either side. The call, its arguments, the library function and what it calls
are counted, the loop is not. `text` and `frame` include the frame
boundary, where each SDK's handler sends what the calls prepared; `idle`
is that handler alone, in a ROM that only waits.

**Reading it.**

- **`oamxy` and `dma` are level**, within a few %, and move to either side
  of PVSnesLib's figure from one compiler change to the next although the
  library routines do not change: a row is the difference between a loop
  with the calls and the same loop empty, and a compiler improvement that
  helps the empty loop more than the loop around a call moves the row.
  Read these two as ties. PVSnesLib's `oamSetXY` checks nothing and marks
  nothing; ours refuses an id that is not a sprite, marks the table dirty
  and records the highest sprite written, which is what lets the handler
  skip the upload on a frame where no sprite moved and send only the
  sprites in use otherwise — the `idle` row is where that is paid back. A
  2 KB `dmaCopyVram` is the transfer itself (16,384 of its master cycles)
  under either SDK.
- **`frame` is the row to read for a whole game**: everything in one
  frame, uploads included.

## Reproduce

```sh
testing/bin/luna profile examples/games/rpg/rpg.sfc \
    --from-frame 120 --until-frame 420 \
    --input "130:0x0100,250:0,260:0x0400,400:0" --out - --top 0
```

`entries[].per_frame.mean` / `.max` per symbol; sum the `function@label`
entries of one function. The other scenes use the same window (frames
120-420; tetris 300-600, START at frame 100) with the inputs of the table
above: likemario RIGHT held from frame 130, rpg RIGHT then DOWN, tetris
START. Re-measure after a change to the NMI handler, the sprite or
animation modules, and update this page.
