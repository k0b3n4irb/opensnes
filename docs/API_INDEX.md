# API index — by what you are trying to do {#api_index}

Doxygen lists the SDK by module, which only helps once you know the
module's name. This page goes the other way: **what you want** → **what
to use** → **an example that does it**.

It exists because that gap has a cost. While writing the RPG template
the author re-implemented `collideTile()` by hand, wrote a sprite-culling
helper without noticing `oamHide()`, hand-rolled scene switching without
noticing `<snes/scene.h>`, and missed the example that already
demonstrated the Tiled pipeline (now `examples/maps/map_scroll`). All four were in the tree. If
you are about to write a helper, scan this page first.

Modules are opt-in: add the name to `LIB_MODULES` in your Makefile and
nothing you do not list is linked (SRAM and SNESMOD are switched on with
`USE_SRAM := 1` and `USE_SNESMOD := 1` instead).

## Getting something on screen

| I want to… | Use | Module | Example |
|---|---|---|---|
| print text | `textModeInit`, `textPrintAt` | `text` | [text/print_string](../examples/text/print_string/) |
| show a background from a PNG | `bgInitTileSet`, `bgSetGfxPtr`, `bgSetMapPtr` | `background` | [backgrounds/mode1](../examples/backgrounds/mode1/) |
| bundle a tileset + its palette as one thing | `BgAsset`, `DECLARE_BG_ASSET`, `bgLoad` | `asset` | [backgrounds/mode1](../examples/backgrounds/mode1/) |
| draw a sprite | `oamInit`, `oamSet`, `oamUpdate` | `sprite` | [sprites/simple_sprite](../examples/sprites/simple_sprite/) |
| hide a sprite that left the screen | `oamHide` | `sprite` | [games/rpg](../examples/games/rpg/) |
| animate a sprite | `AnimClip`, `animPlay`, `animTickOam` | `anim` | [sprites/animated_sprite](../examples/sprites/animated_sprite/) |
| draw one character out of many tiles | `oamDrawMetasprite`, `MetaspriteItem`, `MetaspriteStyle` | `sprite` | [sprites/aseprite_pipeline](../examples/sprites/aseprite_pipeline/) |
| make a tile while the game runs (procedural art) | `tileEncode2bpp`, `tileEncode4bpp`, `tileEncode8bpp` | `tile` | [sprites/sprite_swarm](../examples/sprites/sprite_swarm/) |
| compose text at the pixel in a 2bpp layer (a proportional font, a label at any x and y) | `tileBlitGlyph2bpp` | `tile` | [text/pixel_text](../examples/text/pixel_text/) |

## Moving around a world

| I want to… | Use | Module | Example |
|---|---|---|---|
| scroll a background | `bgSetScroll` | `background` | [scrolling/parallax_scroll](../examples/scrolling/parallax_scroll/) |
| scroll a map bigger than VRAM | `mapLoad`, `mapUpdate` | `map` | [maps/map_scroll](../examples/maps/map_scroll/) |
| load a map made in Tiled | `opensnes-level` → `.m16`/`.b16` | `map` | [maps/map_scroll](../examples/maps/map_scroll/) |
| ask whether a tile is solid | `collideTile` (`collideTileEx` for 16- or 32-pixel tiles) | `collision` | [basics/collision_demo](../examples/basics/collision_demo/) |
| ask whether two boxes overlap | `collideRect` | `collision` | [basics/collision_demo](../examples/basics/collision_demo/) |
| handle slopes | `objCollidMapWithSlopes` + per-tile attributes | `object` | [maps/slope_collision](../examples/maps/slope_collision/) |
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
| save the player's progress | `sramSave`, `sramLoad` | `USE_SRAM := 1` | [memory/save_game](../examples/memory/save_game/) |
| a dialog box or a status bar | `panelDraw`, `panelPut`, `panelFlush` | `panel` | [games/rpg](../examples/games/rpg/) |

## Sound

| I want to… | Use | Module | Example |
|---|---|---|---|
| play music from a tracker module | `snesmod*` | `USE_SNESMOD := 1` | [audio/snesmod_music](../examples/audio/snesmod_music/) |
| play a sound effect | `audioPlaySample` | `audio` | [audio/soundboard](../examples/audio/soundboard/) |

## Effects

| I want to… | Use | Module | Example |
|---|---|---|---|
| fade in or out | `fadeIn`, `fadeOut` | `console` | [transitions/fading](../examples/transitions/fading/) |
| blend two layers | `colormath*` | `colormath` | [color/transparency](../examples/color/transparency/) |
| mask part of the screen | `windowSet*` | `window` | [windows/window](../examples/windows/window/) |
| change a register mid-frame | `hdmaSetup`, `hdmaEnableMask` | `hdma` | [windows/window_multi_hdma](../examples/windows/window_multi_hdma/) |
| rotate or scale a background | `mode7*` | `mode7` | [games/mode7_racing](../examples/games/mode7_racing/) |
| pixelate | `mosaicSetLayers`, `mosaicFadeIn` | `mosaic` | [transitions/mosaic](../examples/transitions/mosaic/) |

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
| send several small VRAM uploads in one VBlank (streamed sprite frames, map rows and columns) | `vramQueuePush`, `vramQueuePushSprite`, `vramQueueFlush`, `vramQueueCost` (what the queue will cost the VBlank) | `vramqueue` | [DMA: several small uploads](tutorials/dma.md) |
| place the sprites of a scrolling game (world coordinates, camera, culling) | `oamPlaceWorld`, `OamWorldBatch` | `sprite` | [sprites: a scrolling game's sprites](tutorials/sprites.md) |
| move many sprites at once | `oamMemory`, `oam_update_flag`, `oamSetFast` | `sprite` | [sprites/sprite_swarm](../examples/sprites/sprite_swarm/) — and its 60fps ceiling |
| decompress | `lzssDecodeVram` | `lzss` | |
| copy, fill or compare memory and strings | `memcpy`, `memmove`, `memset`, `strlen`, `strcmp`, `strcpy`, `strncpy` | `string` | [FAR RAM: copying](tutorials/far_ram.md) |

## 3D math on the DSP-1 coprocessor

| I want to… | Use | Module | Notes |
|---|---|---|---|
| rotate + project points in 3D | `dsp1Attitude`, `dsp1Objective`, `dsp1Project` (setup: `dsp1SetCamera`) | `dsp1` | [chips/dsp1_cube](../examples/chips/dsp1_cube/) — needs `USE_DSP1 := 1` |
| true 3D distance / sphere test | `dsp1Distance`, `dsp1Range` | `dsp1` | hardware sqrt — collision, LOD, homing |
| sin/cos scaled by a radius | `dsp1Triangle` | `dsp1` | 16-bit angles (full turn = 2^16) |
| check the chip is there | `dsp1IsPresent` | `dsp1` | known-answer probe, never hangs |

## Going faster

| I want to… | Use | Notes |
|---|---|---|
| more CPU | SA-1 (`USE_SA1=1`) | same 65816 ISA at 10.74 MHz — [tutorial](tutorials/sa1.md) |
| polygons | Super FX (`USE_SUPERFX=1`) | GSU assembly only, no C compiler |
| fixed-point maths | `fixed32.h` | |

## The rest, by header

Functions the tables above do not name, each with the page that shows it
in use. Anything not listed here or above is in the Doxygen reference of
its header.

| Header | Functions | Where it is explained |
|---|---|---|
| `anim.h` | `animRestart` | [animation](tutorials/animation.md) |
| `audio.h` | `audioPlaySampleOn`, `audioStopVoice`, `audioStopAll` | [audio: choosing the voice](tutorials/audio_samples.md) |
| `audio.h` | `audioSetVolume`, `audioGetVolume`, `audioSetVoiceVolume`, `audioSetVoicePitch`, `audioSetVoiceVolumePitch`, `audioSetVoicePan` | [audio: volume and pitch](tutorials/audio_samples.md) |
| `audio.h` | `audioSetADSR`, `audioSetGain` | [audio: ADSR or GAIN](tutorials/audio_samples.md) |
| `audio.h` | `audioSetEcho`, `audioSetEchoFilter`, `audioEnableEcho`, `audioDisableEcho`, `audioReleaseEcho` | [audio: echo](tutorials/audio_samples.md) |
| `audio.h` | `audioIsReady`, `audioGetFreeMemory`, `audioGetVoiceState` | [audio: asking the engine](tutorials/audio_samples.md) |
| `background.h` | `bgInitTileSetData`, `bgInit` | [graphics](tutorials/graphics.md) |
| `collision.h` | `rectSetPos`, `rectGetCenter` | [collision](tutorials/collision.md) |
| `colormath.h` | `colorMathSetChannel`, `colorMathSetBrightness`, `colorMathTransparency50`, `colorMathSetDirectColor` | [colour math](tutorials/colormath.md) |
| `console.h` | `rngNext`, `rngSeed` | [math: random numbers](tutorials/math.md) |
| `console.h` | `resetFrameCount` | [animation](tutorials/animation.md) |
| `console.h` | `isInVBlank` | [DMA](tutorials/dma.md) |
| `debug.h` | `consoleMesenBreakpoint`, `consoleNocashMessage` (behind `SNES_BREAK`, `SNES_NOCASH`) | [debugging](tutorials/debugging.md) |
| `hdma.h` | `hdmaColorGradient`, `hdmaColorGradientStop`, `hdmaBrightnessGradientStop`, `hdmaIrisWipe`, `hdmaIrisWipeStop`, `hdmaWaterRipple`, `hdmaWaveInit`, `hdmaWaveStop`, `hdmaWindowShape`, `hdmaGetEnabled`, `hdmaDisableAll` | [HDMA: ready-made effects](tutorials/hdma.md) |
| `input.h` | `padIsConnected` | [input: is a pad plugged in?](tutorials/input.md) |
| `input.h` | `mouseInit`, `mouseIsConnected`, `mouseGetX`, `mouseGetY`, `mouseButtonsHeld`, `mouseButtonsPressed`, `mouseSetSensitivity`, `mouseGetSensitivity` | [input: the mouse](tutorials/input.md) |
| `input.h` | `scopeInit`, `scopeIsConnected`, `scopeGetX`, `scopeGetY`, `scopeGetRawX`, `scopeGetRawY`, `scopeButtonsHeld`, `scopeButtonsPressed`, `scopeButtonsRepeat`, `scopeSetHoldDelay`, `scopeSetRepeatDelay`, `scopeSinceShot` | [input: the Super Scope](tutorials/input.md) |
| `interrupt.h` | `nmiSet`, `nmiClear`, `irqSet`, `irqClear`, `irqSetHTimer`, `irqSetVTimer`, `irqEnable`, `irqDisable` | [interrupts](tutorials/interrupts.md) |
| `math.h` | `easeInQuad`, `easeOutQuad` (module `math_ease`) | [math: easing curves](tutorials/math.md) |
| `profile.h` | `profileGetScanline` | [profiling](tutorials/profiling.md) |
| `snesmod.h` | `snesmodGetPosition`, `snesmodFlush` | [audio: following the music](tutorials/audio.md) |
| `sprite.h` | `oamSetX` | [sprites](tutorials/sprites.md) |
| `superfx.h` | `gsuIsPresent`, `gsuSetupBitmapTilemap`, `gsuDmaFullFrame`, `gsuSetupHdmaBlanking`, `gsuCacheLoad`, `gsuStartCached`, `gsuBusy`, `gsuWait` | [Super FX](tutorials/superfx.md), [the code cache](tutorials/superfx_advanced.md) |

## When something silently does nothing

The SNES fails quietly. @ref troubleshooting goes from the symptom to the
cause, and @ref known_limitations is the catalogue of the traps behind them.

## Header → tutorial map

Which public header each tutorial covers, and where the documentation is thin.
Written by hand; `make lint-docs` checks that every header of
`lib/include/snes/` has a row and that every header and tutorial a row names
exists. "Partial" means the tutorial teaches the
topic without naming most of the API.

| Header | Covered by | State |
|---|---|---|
| `anim.h` | [Animation](tutorials/animation.md) | ✅ |
| `apu.h` | [Samples and the raw APU](tutorials/audio_samples.md) | ✅ |
| `audio.h` | [Samples and the raw APU](tutorials/audio_samples.md) | ✅ |
| `background.h` | [Scrolling](tutorials/scrolling.md), [Graphics](tutorials/graphics.md) | ✅ |
| `collision.h` | [Collision](tutorials/collision.md) | ✅ |
| `colormath.h` | [Colour math](tutorials/colormath.md) | ✅ |
| `console.h` | scattered across tutorials | 🟡 partial — no page owns init, brightness, fades, region |
| `dma.h` | [DMA](tutorials/dma.md) | ✅ |
| `dsp1.h` | [DSP-1](tutorials/dsp1.md) | ✅ |
| `hdma.h` | [HDMA](tutorials/hdma.md) | ✅ |
| `input.h` | [Input](tutorials/input.md) | ✅ pads, mouse, Super Scope (the multitap cannot be switched on: Known limitations) |
| `interrupt.h` | [Interrupts](tutorials/interrupts.md) | ✅ |
| `tile.h` | [Graphics](tutorials/graphics.md) | ✅ |
| `map.h` | [Maps](tutorials/map.md) | ✅ |
| `math.h` | [Fixed-Point Math](tutorials/math.md) | ✅ |
| `mode7.h` | [Mode 7](tutorials/mode7.md) | ✅ |
| `mosaic.h` | [Mosaic](tutorials/mosaic.md) | ✅ |
| `panel.h` | [Panels](tutorials/panel.md) | ✅ |
| `profile.h` | [Profiling](tutorials/profiling.md) | ✅ |
| `sa1.h` | [SA-1](tutorials/sa1.md) | ✅ |
| `scene.h` | [Game States](tutorials/game_states.md) | ✅ |
| `snesmod.h` | [Music with SNESMOD](tutorials/audio.md) | ✅ |
| `sprite.h` | [Sprites](tutorials/sprites.md), [Animation](tutorials/animation.md) | ✅ |
| `sram.h` | [SRAM](tutorials/sram.md) | ✅ |
| `superfx.h` | [Super FX: a first job](tutorials/superfx.md), [running a game around the GSU](tutorials/superfx_advanced.md) | ✅ |
| `video.h` | [Graphics](tutorials/graphics.md) | ✅ |
| `window.h` | [Window](tutorials/window.md) | ✅ |
| `asset.h` | [The Opt-In Framework](tutorials/framework.md) | ✅ |
| `debug.h` | [Debugging](tutorials/debugging.md) — recipe 7 | ✅ |
| `fixed32.h` | [Math](tutorials/math.md) — the 16.16 section | ✅ |
| `gameloop.h` | [The Opt-In Framework](tutorials/framework.md) | ✅ |
| `lzss.h` | [Graphics](tutorials/graphics.md) — compressed tiles | ✅ |
| `string.h` | [FAR RAM](tutorials/far_ram.md) — copying between ROM, RAM and FAR RAM | ✅ |
| `vramqueue.h` | [DMA](tutorials/dma.md) — several small uploads in one VBlank | ✅ |
| `object.h` | [The Object Engine](tutorials/object.md), [how it works](tutorials/object_engine.md) | ✅ |
| `text.h` | [Text & Fonts](tutorials/text.md) | ✅ |
| `registers.h`, `system.h`, `types.h` | — | reference headers: macros and types, no prose needed |

Every public header is covered: 16.16 maths in the [Math](tutorials/math.md)
tutorial, LZ77 decompression in [Graphics](tutorials/graphics.md), and the
in-ROM debug channel in [Debugging](tutorials/debugging.md) (recipe 7).

## Keeping this page honest

Example paths here, and the header map above, are checked by
`devtools/check_doc_drift.py`, so a renamed or deleted example, a header
without a row, a dead tutorial link or a function no header declares fails
`make lint-docs`.
