# Measured frame costs of the library {#perf}
What the library functions cost per frame in real scenes, measured on luna
(cycle-accurate) on 2026-10-09 with the SDK of that day (1.0.0 in
preparation; first measured on 2026-09-26, and the differences are noted).
`PHILOSOPHY.md` asks every API with a cost to say it; this page is the
measured side of that promise, and `docs/BENCHMARK.md` the compiler side.

**Unit: master cycles (mclk) per frame.** An NTSC frame is about
**357,370 mclk** (21.477 MHz / 60.1 Hz); the VBlank, where VRAM may be
written, about 49,000 of them (48,988: 37 lines of 1324 available master clocks, snesdev-wiki "Timing"). 1 % of a frame is about 3,570 mclk.

## The scenes

| Scene | What runs | Idle (in `WaitForVBlank`) |
|---|---|---|
| `examples/games/likemario` | walking right: map scroll, dynamic sprite, animation, SNESMOD | 88 % of the frame |
| `examples/games/rpg` | walking: map, sprites, HUD | 91 % of the frame |
| `examples/sprites/sprite_swarm` | many sprites moved every frame (direct OAM writes) | 69 % of the frame |
| `examples/games/tetris` | playing: board redraw, HUD, SNESMOD | 81 % of the frame |
| `examples/audio/snesmod_music` | music playing, nothing else | 98 % of the frame |

Idle time is what the game still has. On 2026-09-26 sprite_swarm had 17 %
of its frame left and was close to dropping frames; on 2026-10-09 it has
69 %.

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
  8 master cycles per byte transferred") about 4,350 of those. It
  includes the 17th-bit pad read added on 2026-09-26 (+378 mclk). The
  per-example gate is `testing/nmi_budget.py`.
- **Sprites**: in the RPG, `oamSet` + `oamSetSize` + `oamHide` cost about
  5,800 mclk per frame (1.6 %) for its handful of characters and HUD
  sprites; 13,600 on 2026-09-26, before the setters were written in
  assembly. `oamSetFast` / `oamSetXYFast` (macros, `sprite.h`) or writing
  `oamMemory[]` directly (sprite_swarm) are the escape hatches for large
  counts.
- **Animation**: `animTick` + `animPlay` about 5,500 mclk per frame in
  likemario (7,400 on 2026-09-26; both are compiled C).
- **`bgSetScroll`**: about 570 mclk per frame in each of the three games
  (1,650 on 2026-09-26).
- **Pads**: `padPressed` / `padHeld` no longer appear: they are macros over
  one word since 2026-10-09 (1,000-1,250 mclk per frame each before).

The per-function figures are what luna counts at the function's own
labels. Asm routines a function calls under their own names (the map
engine's `map_prepare_column`, SNESMOD's SPC transfer routines) are not
added to it, and code the compiler inlined into the caller is counted in
the caller.

## What a call costs where there used to be none

Until 2026-10-03 a handful of small functions were `inline` in their headers
(`fixSin`, `fixCos`, `textSetPos`, `setScreenOn`, `setScreenOff`,
`getBrightness`, `colorMathInit`, `colorMathSetLayers`, `colorMathDisable`,
`mosaicInit`, `hdmaWaveSetSpeed`, `scopeCalibrate`, `scopeSetHoldDelay` and
the two ease functions). An inline body needs its variables in the public
header, and those were names a game would collide with (`sine_table`,
`cursor_x`, `force_blanked`). They are ordinary lib functions now.

The one that runs per frame is `fixSin` / `fixCos`. Measured on
`examples/backgrounds/mode2`, which calls `fixSin` 32 times a frame
(`luna profile`, frames 60 to 300):

| | busy master cycles per frame | share of the frame |
|---|---|---|
| inlined (before) | 62,896 | 17.60 % |
| compiled C function | 77,415 | 21.66 % |
| assembly function (what ships) | 65,959 | 18.46 % |

So a call costs about 96 master cycles more than the inlined lookup did, and
the first, compiled version cost about 450: `fixSin` and `fixCos` are written
in assembly for that reason. The other functions run at setup or once per
frame, where a call is not measurable.

## What a style struct costs

`oamDrawMetasprite()` (2026-10-03) takes the base tile, palette and size
from a `MetaspriteStyle` instead of three arguments. Measured on
`examples/sprites/metasprite`, two metasprites a frame (`luna profile`,
frames 60 to 300):

| | master cycles per frame in the draw | share of the frame |
|---|---|---|
| the former public `oamDrawMeta` (before; removed from the API 2026-10-05) | 51,992 | 14.55 % |
| one function, flip tested per piece | 58,190 | 16.28 % |
| one function, two loops | 61,252 | 17.14 % |
| reads the style, then runs the old loop (what ships) | 56,057 | 15.69 % |

About 2,000 master cycles per call, 0.6 % of a frame per metasprite drawn:
three reads through the style pointer and one more call. The two attempts
above it were slower although they add no call: with the mirrored loop in
the same function, the compiler's copies between the two loops fell on
every piece. The mirrored draw is therefore a function of its own, and the
unflipped one still runs the same loop, now the internal (removed from the API) `oamDrawMeta`.

## Against PVSnesLib, call for call {#perf-libbench}

`devtools/libbench` asks both libraries for the same things — what a game
asks every frame — and times each request on luna. Master cycles for one
frame's worth; `docs/BENCHMARK.md` has the same comparison for the
compilers.

<!-- libbench:begin -->
| Row | What it asks for | PVSnesLib | OpenSNES | |
|---|---|---:|---:|---:|
| `idle` | a frame that only waits (the SDK's vblank handler) | 7,184 | 5,780 | -19.5 % |
| `pad` | held, pressed, released of pad 0, ten times | 11,429 | 10,229 | -10.5 % |
| `scroll` | `bgSetScroll` on three backgrounds, ten times | 39,539 | 27,610 | -30.2 % |
| `oamset` | `oamSet` on 32 sprites | 81,070 | 66,546 | -17.9 % |
| `oamxy` | `oamSetXY` on 32 sprites | 45,047 | 44,066 | -2.2 % |
| `oamsize` | the size of 32 sprites | 48,746 | 29,694 | -39.1 % |
| `dma` | 2 KB to VRAM, one `dmaCopyVram` | 17,894 | 17,885 | -0.1 % |
| `text` | 20 characters printed and shown | 84,676 | 67,440 | -20.4 % |
| `frame` | one frame: pad, three scrolls, 32 sprites, 20 characters | 169,798 | 142,257 | -16.2 % |
| `worldc` | 19 world-space sprites placed by a loop in C (the same source) | 129,330 | 62,797 | -51.4 % |
| `world` | the same 19 sprites: `oamPlaceWorld` here, the C loop there | 129,330 | 31,379 | -75.7 % |
| `vramc` | six 128-byte VRAM transfers, six `dmaCopyVram` calls | 17,400 | 16,605 | -4.6 % |
| `vramq` | the same six, the part paid in VBlank: one `vramQueueFlush` here, the calls there | 17,400 | 10,737 | -38.3 % |

OpenSNES costs no more than PVSnesLib on **13 of the 13 rows**. PVSnesLib at `fa758c9b 2025-12-28`.
<!-- libbench:end -->

**How a row is timed.** Each row is built twice per SDK, with its library
calls and with the same loops around nothing; both run the same number of
frames, and the cost is what the symbols of the first gained over the
second. The call, its arguments, the library function and what it calls
are counted, the loop is not. `text` and `frame` include the frame
boundary, where each SDK's handler sends what the calls prepared; `idle`
is that handler alone, in a ROM that only waits.

**Reading it.**

- **The first run of this bench, on 2026-10-09, had OpenSNES behind on
  seven rows of eight**: the pad read at 3.5 times PVSnesLib's cost,
  `oamSetXY` at 3.6 times, text +83 %, sprite size +41 %, scroll +13 %,
  `oamSet` +9 %. The compiler had been measured against PVSnesLib for
  months; the library had only been measured against itself (the tables
  above).
- What changed that day: `padHeld` and `padPressed` are also macros over
  the word the handler filled, and `padReleased` is assembly; the sprite
  setters (`oamSet`, `oamSetX`, `oamSetY`, `oamSetXY`, `oamSetSize`) and
  the three scroll setters are assembly, with a table for the bits a shift
  loop used to build; `textPrint` writes a run of characters with the
  buffer position computed once instead of three calls and a multiply per
  character.
- **`oamxy` and `dma` are level**, within about 1 %. PVSnesLib's
  `oamSetXY` checks nothing and marks nothing; ours refuses an id that is
  not a sprite, marks the table dirty and records the highest sprite
  written, which is what lets the handler skip the upload on a frame where
  no sprite moved and send only the sprites in use otherwise — the `idle`
  row is where that is paid back. A 2 KB `dmaCopyVram` is the transfer
  itself (16,384 of its master cycles) under either SDK.
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
