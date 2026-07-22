# API index — by what you are trying to do

Doxygen lists the SDK by module, which only helps once you know the
module's name. This page goes the other way: **what you want** → **what
to use** → **an example that does it**.

It exists because that gap has a cost. While writing the RPG template
the author re-implemented `collideTile()` by hand, wrote a sprite-culling
helper without noticing `oamHide()`, hand-rolled scene switching without
noticing `<snes/scene.h>`, and missed `examples/maps/tiled` — which
already demonstrates the Tiled pipeline. All four were in the tree. If
you are about to write a helper, scan this page first.

Modules are opt-in: add the name to `LIB_MODULES` in your Makefile and
nothing you do not list is linked.

## Getting something on screen

| I want to… | Use | Module | Example |
|---|---|---|---|
| print text | `textInit`, `textPrintAt`, `textFlush` | `text` | [text/hello_world](../examples/text/hello_world/) |
| show a background from a PNG | `bgInitTileSet`, `bgSetGfxPtr`, `bgSetMapPtr` | `background` | [graphics/backgrounds/mode1](../examples/graphics/backgrounds/mode1/) |
| bundle a tileset + its palette as one thing | `BgAsset`, `DECLARE_BG_ASSET`, `bgLoad` | `asset` | [graphics/backgrounds/mode1](../examples/graphics/backgrounds/mode1/) |
| draw a sprite | `oamInit`, `oamSet`, `oamUpdate` | `sprite` | [graphics/sprites/simple_sprite](../examples/graphics/sprites/simple_sprite/) |
| hide a sprite that left the screen | `oamHide` | `sprite` | [games/rpg](../examples/games/rpg/) |
| animate a sprite | `AnimClip`, `animPlay`, `animTickOam` | `anim` | [graphics/sprites/animated_sprite](../examples/graphics/sprites/animated_sprite/) |
| draw one character out of many tiles | `oamDrawMeta`, `MetaspriteItem` | `sprite` | [graphics/sprites/metasprite](../examples/graphics/sprites/metasprite/) |

## Moving around a world

| I want to… | Use | Module | Example |
|---|---|---|---|
| scroll a background | `bgSetScroll` | `background` | [graphics/effects/parallax_scrolling](../examples/graphics/effects/parallax_scrolling/) |
| scroll a map bigger than VRAM | `mapLoad`, `mapUpdate` | `map` | [maps/mapscroll](../examples/maps/mapscroll/) |
| load a map made in Tiled | `tmx2snes` → `.m16`/`.b16` | `map` | [maps/tiled](../examples/maps/tiled/) |
| ask whether a tile is solid | `collideTile`, `collideTileEx` | `collision` | [basics/collision_demo](../examples/basics/collision_demo/) |
| ask whether two boxes overlap | `collideRect`, `collideRectEx` | `collision` | [basics/collision_demo](../examples/basics/collision_demo/) |
| handle slopes | `collideTileEx` + per-tile attributes | `collision` | [maps/slopemario](../examples/maps/slopemario/) |
| place the sprite so collision *feels* right | the straddle convention | — | [collision tutorial](tutorials/collision.md#where-the-sprite-is-vs-where-it-collides) |

## Input

| I want to… | Use | Module | Example |
|---|---|---|---|
| read a joypad | `padHeld`, `padPressed` | `input` | [input/controller](../examples/input/controller/) |
| read two players | `padHeld(0)` / `padHeld(1)` | `input` | [input/two_players](../examples/input/two_players/) |
| read a mouse or Super Scope | `mouse*` / `scope*` | `input` | [input/mouse](../examples/input/mouse/) |

## Structuring a game

| I want to… | Use | Module | Example |
|---|---|---|---|
| a main loop you do not write | `gameLoopRun` | `gameloop` | [basics/timer](../examples/basics/timer/) |
| title → play → pause, without a state enum | `Scene`, `scenePush`, `scenePop` | `scene` | [basics/scene_stack](../examples/basics/scene_stack/) |
| save the player's progress | `sramSave`, `sramLoad` | `sram` | [memory/save_game](../examples/memory/save_game/) |
| a dialog box or a status bar | `panelDraw`, `panelPut`, `panelFlush` | `panel` | [games/rpg](../examples/games/rpg/) |

## Sound

| I want to… | Use | Module | Example |
|---|---|---|---|
| play music from a tracker module | `snesmod*` | `snesmod` | [audio/snesmod_music](../examples/audio/snesmod_music/) |
| play a sound effect | `audioPlaySample` | `audio` | [audio/soundboard](../examples/audio/soundboard/) |

## Effects

| I want to… | Use | Module | Example |
|---|---|---|---|
| fade in or out | `fadeIn`, `fadeOut` | `console` | [graphics/effects/fading](../examples/graphics/effects/fading/) |
| blend two layers | `colormath*` | `colormath` | [graphics/effects/transparency](../examples/graphics/effects/transparency/) |
| mask part of the screen | `windowSet*` | `window` | [graphics/effects/window](../examples/graphics/effects/window/) |
| change a register mid-frame | `hdmaEnable`, `hdmaGradient` | `hdma` | [graphics/effects/window_multi_hdma](../examples/graphics/effects/window_multi_hdma/) |
| rotate or scale a background | `mode7*` | `mode7` | [games/mode7_racing](../examples/games/mode7_racing/) |
| pixelate | `mosaicEnable`, `mosaicFadeIn` | `mosaic` | [graphics/effects/mosaic](../examples/graphics/effects/mosaic/) |

## Moving data around

| I want to… | Use | Module | Notes |
|---|---|---|---|
| upload tiles or a tilemap | `dmaCopyVram` | `dma` | VBlank fits ~4 KB; more needs `setScreenOff()` |
| upload a palette | `dmaCopyCGram` | `dma` | the source may live in any bank |
| set one colour | `setColor` | `console` | sprite palettes start at CGRAM 128 |
| decompress | `LzssDecodeVram` | `lzss` | |

## Going faster

| I want to… | Use | Notes |
|---|---|---|
| more CPU | SA-1 (`USE_SA1=1`) | same 65816 ISA at 10.74 MHz — [tutorial](tutorials/sa1.md) |
| polygons | Super FX (`USE_SUPERFX=1`) | GSU assembly only, no C compiler |
| fixed-point maths | `fixed32.h` | |

## When something silently does nothing

The SNES fails quietly. `KNOWN_LIMITATIONS.md` is the catalogue; the
ones that cost the most time:

| Symptom | Likely cause |
|---|---|
| a VRAM upload partly lands | outside VBlank/forced blank, or over the ~4 KB budget |
| the screen stays black after a DMA | `setBrightness(0)` is not forced blank — use `setScreenOff()` |
| a sprite renders as background garbage | `oamInit`'s second argument is a page number 0-7, not an address — use `OBJ_NAME_BASE(addr)` |
| a sprite appears where no entity is | off-camera OAM coordinates wrap — `oamHide()` it |
| a `const` array reads as garbage | it spilled past bank $00; the build's bank-blind check should catch it |
| colours shift when you add a tile | gfx4snes re-quantises the whole palette — author a fixed palette |

## Keeping this page honest

Example paths here are checked by `devtools/check_doc_drift.py`, so a
renamed or deleted example fails `make lint-docs`. Function names are
not checked — if you rename a public function, grep this file.
