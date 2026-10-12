# OpenSNES Examples

Learn SNES development step by step. 88 examples organized by topic, building
from basic concepts to complete games.

## Categories

| Category | Examples | What It Covers |
|----------|----------|----------------|
| [text/](text/) | 3 | Text display, fonts, tilemaps |
| [fundamentals/](fundamentals/) | 1 | Under-the-hood: raw tiles, direct VRAM writes |
| [basics/](basics/) | 7 | Collision, timing, scene stack, randomness, fixed-point, aiming, HUD panels, game skeleton |
| [backgrounds/](backgrounds/) | 9 | BG modes 0/1/2/3/4/5/6, priority, LZ77, hi-res, offset-per-tile |
| [sprites/](sprites/) | 9 | Sprite display, animation, OAM, metasprites, VRAM streaming, swarm, Aseprite pipeline |
| [hdma/](hdma/) | 4 | Per-scanline HDMA effects: gradients, waves, raster |
| [color/](color/) | 6 | Palette cycling, colour math, shadow/tint, direct colour, HiColor, pseudo-hires blend |
| [windows/](windows/) | 3 | Hardware window masking, shaped per scanline |
| [transitions/](transitions/) | 2 | Screen transitions: fade, mosaic pixelate |
| [scrolling/](scrolling/) | 3 | Layer scrolling: parallax, streaming, per-scanline HDMA |
| [mode7/](mode7/) | 5 | Mode 7: rotation, scaling, per-scanline perspective, DSP-1 ground, EXTBG |
| [input/](input/) | 4 | Joypads, mouse, Super Scope, multi-player |
| [audio/](audio/) | 10 | Music and sound effects: SNESMOD and raw APU/DSP |
| [maps/](maps/) | 4 | Tile maps, dynamic streaming, slopes |
| [memory/](memory/) | 2 | HiROM mode, battery-backed saves |
| [chips/](chips/) | 8 | Enhancement chips: SA-1 and SuperFX/GSU coprocessors |
| [games/](games/) | 8 | Complete game projects (Tetris, Breakout, Mario-like, map+objects, 1942-style shmup, Mode 7 racing + flying, Tiled-driven RPG) |

## Learning Path

A curated progression — it doesn't list every example. Use the category
table above for the full set; anything not listed here is a variant or
deep-dive of a step below.

### Level 1 -- First Steps

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 1 | [text/print_string](text/print_string/) | Your first ROM: text on a background, the VBlank rhythm |
| 2 | [text/scroll_message](text/scroll_message/) | Move text -- bgSetScroll on the text layer |
| 3 | [text/pixel_text](text/pixel_text/) | Proportional text composed at the pixel: `tileBlitGlyph2bpp()` into a 2bpp layer |
| 4 | [sprites/simple_sprite](sprites/simple_sprite/) | OAM, sprite display, CGRAM split |
| 5 | [input/two_players](input/two_players/) | Joypad reading, multiplayer input |

### Level 2 -- Graphics Fundamentals

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 6 | [backgrounds/mode1](backgrounds/mode1/) | Mode 1 multi-layer backgrounds |
| 7 | [backgrounds/mode1_bg3_priority](backgrounds/mode1_bg3_priority/) | BG3 priority bit in Mode 1 |
| 8 | [backgrounds/mode1_lz77](backgrounds/mode1_lz77/) | LZ77-compressed background data |
| 9 | [sprites/animated_sprite](sprites/animated_sprite/) | Frame animation, sprite sheets, H-flip |
| 10 | [sprites/dynamic_sprite](sprites/dynamic_sprite/) | VRAM streaming, dynamic tile uploads |
| 11 | [sprites/sprite_sizes](sprites/sprite_sizes/) | OBJSEL sprite size configurations |
| 12 | [transitions/fading](transitions/fading/) | Brightness control, screen transitions |
| 13 | [transitions/mosaic](transitions/mosaic/) | Mosaic pixelation effect |

### Level 3 -- Scrolling and Effects

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 14 | [scrolling/continuous_scroll](scrolling/continuous_scroll/) | Streaming background scroll with dynamic tile loading |
| 15 | [scrolling/mixed_scroll](scrolling/mixed_scroll/) | Multiple BG layers scrolling at different rates |
| 16 | [hdma/hdma_wave](hdma/hdma_wave/) | HDMA wave: build the table by hand (krom port), then use the hdma module |
| 17 | [hdma/hdma_indirect_gradient](hdma/hdma_indirect_gradient/) | Indirect HDMA: pointer table drives a backdrop gradient (krom port) |
| 18 | [color/hicolor_1792](color/hicolor_1792/) | H-IRQ CGRAM streaming: 1792 colors from a 4bpp BG (krom port) |
| 19 | [mode7/perspective_rotate](mode7/perspective_rotate/) | Full Mode 7 matrix per scanline: rotating perspective (krom port) |
| 20 | [backgrounds/mode5_hires](backgrounds/mode5_hires/) | BG Mode 5 + interlace: 512x448 hi-res text (krom port) |
| 21 | [windows/window_multi_hdma](windows/window_multi_hdma/) | Both windows shaped per scanline: HDMA porthole grid (krom port) |
| 22 | [color/direct_color](color/direct_color/) | Direct color: 8bpp pixel bytes read as BBGGGRRR, CGRAM bypassed |
| 23 | [color/pseudo_hires](color/pseudo_hires/) | Pseudo-hires: a 50 % blend of two layers without colour math (SETINI bit 3) |
| 24 | [backgrounds/mode4](backgrounds/mode4/) | Mode 4: offset-per-tile on a 256-colour layer, one row of H-or-V words |
| 25 | [backgrounds/mode6](backgrounds/mode6/) | Mode 6: offset-per-tile on a hi-res layer, 16-half-pixel columns |
| 26 | [hdma/gradient_colors](hdma/gradient_colors/) | HDMA + CGRAM color gradients |
| 27 | [scrolling/parallax_scroll](scrolling/parallax_scroll/) | HDMA parallax scrolling |
| 28 | [color/transparency](color/transparency/) | Color math (add/subtract blending) |
| 29 | [windows/window](windows/window/) | Hardware window masking |
| 30 | [windows/transparent_window](windows/transparent_window/) | Color math + HDMA windowed transparency |

### Level 4 -- Advanced Topics

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 31 | [mode7/rotate_scale](mode7/rotate_scale/) | Mode 7 rotation and scaling |
| 32 | [mode7/perspective](mode7/perspective/) | Pseudo-3D perspective (F-Zero style) |
| 33 | [mode7/dsp1_ground](mode7/dsp1_ground/) | Super Mario Kart floor: DSP-1 Raster streams the per-scanline Mode 7 matrices |
| 34 | [mode7/extbg](mode7/extbg/) | Mode 7 EXTBG: one plane, two layers split by bit 7, a sprite between them |
| 35 | [sprites/metasprite](sprites/metasprite/) | Multi-tile composite sprites |
| 36 | [sprites/aseprite_pipeline](sprites/aseprite_pipeline/) | Full asset pipeline: Aseprite → gfx4snes -P + aseprite2snes → animated metasprite |
| 37 | [sprites/world_sprites](sprites/world_sprites/) | World-space sprites: a camera, culling and OAM in one call per batch (`oamPlaceWorld`) |
| 38 | [input/mouse](input/mouse/) | Mouse detection, cursor, sensitivity |
| 39 | [input/superscope](input/superscope/) | Light gun detection, PPU H/V counters |
| 40 | [memory/hirom_demo](memory/hirom_demo/) | HiROM vs LoROM memory mapping |
| 41 | [memory/save_game](memory/save_game/) | SRAM persistence (battery saves) |
| 42 | [audio/snesmod_music](audio/snesmod_music/) | SPC700 music playback via SNESMOD |
| 43 | [audio/snesmod_sfx](audio/snesmod_sfx/) | Sound effects via SNESMOD |
| 44 | [audio/speech_synth](audio/speech_synth/) | Phoneme-bank speech synthesis: the SNES says "OPEN SNES" (krom port) |
| 45 | [audio/play_noise](audio/play_noise/) | Drum kit from the DSP noise generator — zero samples (krom port) |
| 46 | [audio/pitch_mod](audio/pitch_mod/) | Hardware vibrato: PMON pitch modulation + LFO voice (krom port) |
| 47 | [audio/apu_switch](audio/apu_switch/) | Hot-swap APU programs at runtime: apuReset() + IPL re-entry |
| 48 | [audio/soundboard](audio/soundboard/) | The audio v2 engine from pure C: dynamic samples, pan/pitch, echo |

### Level 5 -- Maps and Complete Projects

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 49 | [maps/dynamic_map](maps/dynamic_map/) | Dynamic tile map streaming |
| 50 | [maps/slope_collision](maps/slope_collision/) | Slopes and tile-based collision |
| 51 | [basics/collision_demo](basics/collision_demo/) | Bounding-box sprite collision |
| 52 | [games/breakout](games/breakout/) | Complete game: sprites, input, game logic |
| 53 | [games/likemario](games/likemario/) | Platformer with scrolling and animation |
| 54 | [games/mapandobjects](games/mapandobjects/) | Maps with interactive objects |
| 55 | [games/mode7_racing](games/mode7_racing/) | F-Zero-style racing: the Mode 7 camera, fixed-point physics, banked data |
| 56 | [games/mode7_flying](games/mode7_flying/) | Pilotwings-style flying: altitude-as-scale, shadow depth cue, landings |
| 57 | [games/rpg](games/rpg/) | RPG template: a Tiled (.tmj) map drives terrain, collision and entities; 9-slice dialog box |

## Building

```bash
# Build all examples
cd opensnes
make

# Build a single example
make -C examples/text/print_string

# Clean and rebuild
make clean && make
```

## Running

We recommend [Mesen2](https://github.com/SourMesen/Mesen2) for accurate SNES emulation:

```bash
mesen examples/text/print_string/print_string.sfc
```

Use Mesen's built-in debugger to inspect VRAM, OAM, palettes, and registers in real time.

## Tips

1. **Follow the order** -- each example builds on concepts from earlier ones
2. **Read the source** -- every `main.c` is commented to explain the "why"
3. **Experiment** -- change values, break things, see what happens
4. **Use the debugger** -- Mesen2's PPU viewer is invaluable for understanding VRAM

---

**Ready?** Start with [text/print_string](text/print_string/) and build your first SNES ROM.
