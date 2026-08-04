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
| print text | `textInit`, `textPrintAt`, `textFlush` | `text` | [text/print_string](../examples/text/print_string/) |
| show a background from a PNG | `bgInitTileSet`, `bgSetGfxPtr`, `bgSetMapPtr` | `background` | [backgrounds/mode1](../examples/backgrounds/mode1/) |
| bundle a tileset + its palette as one thing | `BgAsset`, `DECLARE_BG_ASSET`, `bgLoad` | `asset` | [backgrounds/mode1](../examples/backgrounds/mode1/) |
| draw a sprite | `oamInit`, `oamSet`, `oamUpdate` | `sprite` | [sprites/simple_sprite](../examples/sprites/simple_sprite/) |
| hide a sprite that left the screen | `oamHide` | `sprite` | [games/rpg](../examples/games/rpg/) |
| animate a sprite | `AnimClip`, `animPlay`, `animTickOam` | `anim` | [sprites/animated_sprite](../examples/sprites/animated_sprite/) |
| draw one character out of many tiles | `oamDrawMeta`, `MetaspriteItem` | `sprite` | [sprites/metasprite](../examples/sprites/metasprite/) |

## Moving around a world

| I want to… | Use | Module | Example |
|---|---|---|---|
| scroll a background | `bgSetScroll` | `background` | [scrolling/parallax_scroll](../examples/scrolling/parallax_scroll/) |
| scroll a map bigger than VRAM | `mapLoad`, `mapUpdate` | `map` | [maps/map_scroll](../examples/maps/map_scroll/) |
| load a map made in Tiled | `tmx2snes` → `.m16`/`.b16` | `map` | [maps/tiled](../examples/maps/tiled/) |
| ask whether a tile is solid | `collideTile`, `collideTileEx` | `collision` | [basics/collision_demo](../examples/basics/collision_demo/) |
| ask whether two boxes overlap | `collideRect`, `collideRectEx` | `collision` | [basics/collision_demo](../examples/basics/collision_demo/) |
| handle slopes | `collideTileEx` + per-tile attributes | `collision` | [maps/slope_collision](../examples/maps/slope_collision/) |
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
| fade in or out | `fadeIn`, `fadeOut` | `console` | [transitions/fading](../examples/transitions/fading/) |
| blend two layers | `colormath*` | `colormath` | [color/transparency](../examples/color/transparency/) |
| mask part of the screen | `windowSet*` | `window` | [windows/window](../examples/windows/window/) |
| change a register mid-frame | `hdmaEnable`, `hdmaGradient` | `hdma` | [windows/window_multi_hdma](../examples/windows/window_multi_hdma/) |
| rotate or scale a background | `mode7*` | `mode7` | [games/mode7_racing](../examples/games/mode7_racing/) |
| pixelate | `mosaicEnable`, `mosaicFadeIn` | `mosaic` | [transitions/mosaic](../examples/transitions/mosaic/) |

## Moving data around

| I want to… | Use | Module | Notes |
|---|---|---|---|
| upload tiles or a tilemap | `dmaCopyVram` | `dma` | VBlank fits ~4 KB; more needs `setScreenOff()` |
| upload a palette | `dmaCopyCGram` | `dma` | the source may live in any bank |
| set one colour | `setColor` | `console` | sprite palettes start at CGRAM 128 |
| cycle a palette (waterfall/fire/lights) | `setColor`, `dmaCopyCGram` | `dma` | [color/palette_cycle](../examples/color/palette_cycle/) — animate with zero VRAM traffic |
| darken or colour-cast a whole scene (night/underwater/sunset) | `colorMathShadow`, `colorMathTint` | `colormath` | [color/shadow_tint](../examples/color/shadow_tint/) |
| draw a HUD / dialog box (9-slice) | `panelInit`, `panelDraw`, `panelPut`, `panelFlush` | `panel` | [basics/panel_hud](../examples/basics/panel_hud/) |
| structure a whole game (title/play/over) | frame loop + `switch(state)` | — | [basics/game_skeleton](../examples/basics/game_skeleton/) |
| scroll each column independently (flag ripple, heat-haze) | offset-per-tile, `setMode(BG_MODE2)` + BG3 offset table | `background` | [backgrounds/mode2](../examples/backgrounds/mode2/) |
| move many sprites at once | `oamMemory`, `oam_update_flag`, `oamSetFast` | `sprite` | [sprites/sprite_swarm](../examples/sprites/sprite_swarm/) — and its 60fps ceiling |
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
