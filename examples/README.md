# OpenSNES Examples

Learn SNES development step by step. 80 examples organized by topic, building
from basic concepts to complete games.

## Categories

| Category | Examples | What It Covers |
|----------|----------|----------------|
| [text/](text/) | 2 | Text display, fonts, tilemaps |
| [fundamentals/](fundamentals/) | 1 | Under-the-hood: raw tiles, direct VRAM writes |
| [basics/](basics/) | 8 | Collision, timing, scene stack, randomness, fixed-point, aiming, HUD panels, game skeleton |
| [backgrounds/](backgrounds/) | 7 | BG modes 0/1/3/5, priority, LZ77, hi-res |
| [sprites/](sprites/) | 7 | Sprite display, animation, OAM, metasprites, VRAM streaming, swarm |
| [hdma/](hdma/) | 5 | Per-scanline HDMA effects: gradients, waves, raster |
| [color/](color/) | 7 | Palette cycling, colour math, shadow/tint, direct colour, hi-colour tricks |
| [windows/](windows/) | 3 | Hardware window masking, shaped per scanline |
| [transitions/](transitions/) | 2 | Screen transitions: fade, mosaic pixelate |
| [scrolling/](scrolling/) | 3 | Layer scrolling: parallax, streaming, per-scanline HDMA |
| [mode7/](mode7/) | 3 | Mode 7: rotation, scaling, per-scanline perspective |
| [input/](input/) | 5 | Joypads, drive a sprite, mouse, Super Scope, multi-player |
| [audio/](audio/) | 9 | Music and sound effects: SNESMOD and raw APU/DSP |
| [maps/](maps/) | 4 | Tile maps, dynamic streaming, slopes |
| [memory/](memory/) | 2 | HiROM mode, battery-backed saves |
| [chips/](chips/) | 4 | Enhancement chips: SA-1 and SuperFX/GSU coprocessors |
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
| 3 | [sprites/simple_sprite](sprites/simple_sprite/) | OAM, sprite display, CGRAM split |
| 4 | [input/two_players](input/two_players/) | Joypad reading, multiplayer input |

### Level 2 -- Graphics Fundamentals

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 5 | [backgrounds/mode1](backgrounds/mode1/) | Mode 1 multi-layer backgrounds |
| 6 | [backgrounds/mode1_bg3_priority](backgrounds/mode1_bg3_priority/) | BG3 priority bit in Mode 1 |
| 7 | [backgrounds/mode1_lz77](backgrounds/mode1_lz77/) | LZ77-compressed background data |
| 8 | [sprites/animated_sprite](sprites/animated_sprite/) | Frame animation, sprite sheets, H-flip |
| 9 | [sprites/dynamic_sprite](sprites/dynamic_sprite/) | VRAM streaming, dynamic tile uploads |
| 10 | [sprites/sprite_sizes](sprites/sprite_sizes/) | OBJSEL sprite size configurations |
| 11 | [transitions/fading](transitions/fading/) | Brightness control, screen transitions |
| 12 | [transitions/mosaic](transitions/mosaic/) | Mosaic pixelation effect |

### Level 3 -- Scrolling and Effects

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 13 | [scrolling/continuous_scroll](scrolling/continuous_scroll/) | Streaming background scroll with dynamic tile loading |
| 14 | [scrolling/mixed_scroll](scrolling/mixed_scroll/) | Multiple BG layers scrolling at different rates |
| 15 | [hdma/hdma_wave](hdma/hdma_wave/) | HDMA scanline wave distortion |
| 15b | [hdma/hdma_wave_table](hdma/hdma_wave_table/) | Raw HDMA table built in C, krom-style repoint animation |
| 15c | [hdma/hdma_indirect_gradient](hdma/hdma_indirect_gradient/) | Indirect HDMA: pointer table drives a backdrop gradient (krom port) |
| 15d | [color/hicolor_1792](color/hicolor_1792/) | H-IRQ CGRAM streaming: 1792 colors from a 4bpp BG (krom port) |
| 15e | [mode7/perspective_rotate](mode7/perspective_rotate/) | Full Mode 7 matrix per scanline: rotating perspective (krom port) |
| 15f | [backgrounds/mode5_hires](backgrounds/mode5_hires/) | BG Mode 5 + interlace: 512x448 hi-res text (krom port) |
| 15g | [windows/window_multi_hdma](windows/window_multi_hdma/) | Both windows shaped per scanline: HDMA porthole grid (krom port) |
| 15h | [color/gradient_9bit](color/gradient_9bit/) | Brightness-dithered backdrop: the 9-bit color trick (krom port) |
| 15j | [color/hicolor_blend](color/hicolor_blend/) | RGB channel-split color-math blend: 3840 colors (krom port) |
| 15k | [color/direct_color](color/direct_color/) | Direct color: 8bpp pixel bytes read as BBGGGRRR, CGRAM bypassed |
| 16 | [hdma/gradient_colors](hdma/gradient_colors/) | HDMA + CGRAM color gradients |
| 17 | [scrolling/parallax_scroll](scrolling/parallax_scroll/) | HDMA parallax scrolling |
| 18 | [color/transparency](color/transparency/) | Color math (add/subtract blending) |
| 19 | [windows/window](windows/window/) | Hardware window masking |
| 20 | [windows/transparent_window](windows/transparent_window/) | Color math + HDMA windowed transparency |

### Level 4 -- Advanced Topics

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 21 | [mode7/rotate_scale](mode7/rotate_scale/) | Mode 7 rotation and scaling |
| 22 | [mode7/perspective](mode7/perspective/) | Pseudo-3D perspective (F-Zero style) |
| 23 | [sprites/metasprite](sprites/metasprite/) | Multi-tile composite sprites |
| 24 | [input/mouse](input/mouse/) | Mouse detection, cursor, sensitivity |
| 25 | [input/superscope](input/superscope/) | Light gun detection, PPU H/V counters |
| 26 | [memory/hirom_demo](memory/hirom_demo/) | HiROM vs LoROM memory mapping |
| 27 | [memory/save_game](memory/save_game/) | SRAM persistence (battery saves) |
| 28 | [audio/snesmod_music](audio/snesmod_music/) | SPC700 music playback via SNESMOD |
| 29 | [audio/snesmod_sfx](audio/snesmod_sfx/) | Sound effects via SNESMOD |
| 42c | [audio/speech_synth](audio/speech_synth/) | Phoneme-bank speech synthesis: the SNES says "OPEN SNES" (krom port) |
| 42d | [audio/play_noise](audio/play_noise/) | Drum kit from the DSP noise generator — zero samples (krom port) |
| 42e | [audio/pitch_mod](audio/pitch_mod/) | Hardware vibrato: PMON pitch modulation + LFO voice (krom port) |
| 42f | [audio/apu_switch](audio/apu_switch/) | Hot-swap APU programs at runtime: apuReset() + IPL re-entry |
| 42g | [audio/soundboard](audio/soundboard/) | The audio v2 engine from pure C: dynamic samples, pan/pitch, echo |

### Level 5 -- Maps and Complete Projects

| # | Example | What You Will Learn |
|---|---------|---------------------|
| 30 | [maps/dynamic_map](maps/dynamic_map/) | Dynamic tile map streaming |
| 31 | [maps/slope_collision](maps/slope_collision/) | Slopes and tile-based collision |
| 32 | [basics/collision_demo](basics/collision_demo/) | Bounding-box sprite collision |
| 33 | [games/breakout](games/breakout/) | Complete game: sprites, input, game logic |
| 34 | [games/likemario](games/likemario/) | Platformer with scrolling and animation |
| 35 | [games/mapandobjects](games/mapandobjects/) | Maps with interactive objects |
| 36 | [games/mode7_racing](games/mode7_racing/) | F-Zero-style racing: the Mode 7 camera, fixed-point physics, banked data |
| 37 | [games/mode7_flying](games/mode7_flying/) | Pilotwings-style flying: altitude-as-scale, shadow depth cue, landings |
| 38 | [games/rpg](games/rpg/) | RPG template: a Tiled (.tmj) map drives terrain, collision and entities; 9-slice dialog box |

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
