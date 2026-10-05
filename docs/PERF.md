# Measured frame costs of the library {#perf}
What the library functions cost per frame in real scenes, measured on luna
(cycle-accurate) on 2026-09-26 with the SDK of that day (v0.45.0 + develop).
`PHILOSOPHY.md` asks every API with a cost to say it; this page is the
measured side of that promise, and `docs/BENCHMARK.md` the compiler side.

**Unit: master cycles (mclk) per frame.** An NTSC frame is about
**357,370 mclk** (21.477 MHz / 60.1 Hz); the VBlank, where VRAM may be
written, about 49,000 of them (48,988: 37 lines of 1324 available master clocks, snesdev-wiki "Timing"). 1 % of a frame is about 3,570 mclk.

## The scenes

| Scene | What runs | Idle (in `WaitForVBlank`) |
|---|---|---|
| `examples/games/likemario` | walking right: map scroll, dynamic sprite, animation, SNESMOD | 85 % of the frame |
| `examples/games/rpg` | walking: map, sprites, HUD | 83 % of the frame |
| `examples/sprites/sprite_swarm` | many sprites moved every frame (direct OAM writes) | 17 % of the frame |
| `examples/games/tetris` | playing: board redraw, HUD, SNESMOD | 68 % of the frame |
| `examples/audio/snesmod_music` | music playing, nothing else | 97 % of the frame |

Idle time is what the game still has: a scene at 17 % idle (sprite_swarm)
is close to dropping frames, one at 97 % (snesmod_music) does almost nothing.

## Per function

Mean mclk per frame over 300 frames, and in brackets an upper bound of the
worst frame. A function appears only in the scenes that call it, and only
above 300 mclk per frame.

| Function | likemario | rpg | sprite_swarm | tetris | snesmod_music |
|---|---|---|---|---|---|
| `NmiHandler` | 8,164 (8,170) | 8,164 (8,170) | 7,670 (7,716) | 5,676 (5,676) | 5,650 (5,650) |
| `oamSetSize` |  | 8,049 (9,502) |  |  |  |
| `oamSet` |  | 4,612 (5,490) |  |  |  |
| `animTick` | 4,226 (6,198) |  |  |  |  |
| `animPlay` | 3,161 (4,926) |  |  |  |  |
| `dmaCopyVram` |  |  |  | 1,766 (2,292) |  |
| `bgSetScroll` | 1,652 (1,652) | 1,657 (1,692) |  | 1,652 (1,652) |  |
| `oamDynamicDraw` | 1,548 (3,076) |  |  |  |  |
| `padPressed` | 1,247 (1,370) |  |  |  | 1,250 (1,250) |
| `padHeld` | 1,036 (1,036) | 1,036 (1,036) |  |  |  |
| `oamHide` |  | 961 (6,612) |  |  |  |

## Reading it

- **`NmiHandler`** is the fixed price of a frame: 5,650-8,170 mclk (1.6-2.3 %
  of the frame, 11-16 % of the VBlank). The OAM upload it does every frame
  is 544 bytes of DMA, at 8 mclk per byte (anomie's timing doc: "DMA takes
  8 master cycles per byte transferred") about 4,350 of those. It
  includes the 17th-bit pad read added on 2026-09-26 (+378 mclk). The
  per-example gate is `testing/nmi_budget.py`.
- **Sprites**: in the RPG, `oamSet` + `oamSetSize` + `oamHide` cost about
  13,600 mclk per frame (3.8 %) for its handful of characters and HUD
  sprites. `oamSetFast` / `oamSetXYFast` (macros, `sprite.h`) or writing
  `oamMemory[]` directly (sprite_swarm) are the escape hatches for large
  counts.
- **Animation**: `animTick` + `animPlay` about 7,400 mclk per frame in
  likemario.
- **`bgSetScroll`**: about 1,650 mclk per frame in each of the three games.
- **Pads**: `padPressed` / `padHeld` about 1,000-1,250 mclk per frame each
  in the scenes that read them.

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
