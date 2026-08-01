# Browse Examples by Category {#examples_by_category}

All 80 examples organized by topic. For a progressive learning path, see
@ref learning_path.

---

## Text

Print text, then move it.

| Example | Description |
|---------|-------------|
| @subpage examples_text_print_string | Print a string with the built-in font (`textModeInit` + `textPrintAt`) |
| @subpage examples_text_scroll_message | Move text: scroll the text layer for a marquee (`bgSetScroll`) |

---

## Fundamentals

Under-the-hood: raw tiles and direct VRAM writes, with the module stripped away.

| Example | Description |
|---------|-------------|
| @subpage examples_fundamentals_text_glyphs | How a glyph becomes pixels: a hand-coded 2bpp font written straight to VRAM |

---

## Backgrounds

The PPU's background modes — colour vs layers vs resolution.

| Example | Description |
|---------|-------------|
| @subpage examples_backgrounds_mode1 | Mode 1 multi-layer backgrounds (4bpp + 2bpp) |
| @subpage examples_backgrounds_mode1_bg3_priority | BG3 priority bit for overlay effects |
| @subpage examples_backgrounds_mode1_lz77 | LZ77-compressed background data |
| @subpage examples_backgrounds_mode0 | Mode 0: four 2bpp background layers (Kirby parallax) |
| @subpage examples_backgrounds_mode3 | Mode 3: 256-color (8bpp) single layer |
| @subpage examples_backgrounds_mode5 | Mode 5: hi-res 512×256 (16-color) |
| @subpage examples_backgrounds_mode5_hires | Mode 5 + interlace hi-res text (512×448) (krom port) |

---

## Sprites

Sprite display, animation, and OAM management.

| Example | Description |
|---------|-------------|
| @subpage examples_sprites_simple_sprite | Basic OAM setup, sprite display |
| @subpage examples_sprites_sprite_sizes | OBJSEL size configurations (8x8 to 64x64) |
| @subpage examples_sprites_animated_sprite | Frame animation, sprite sheets, H-flip |
| @subpage examples_sprites_metasprite | Multi-tile composite sprites |
| @subpage examples_sprites_dynamic_sprite | VRAM streaming, dynamic tile uploads |
| @subpage examples_sprites_dynamic_metasprite | Dynamic metasprite engine with batched OAM updates |
| @subpage examples_sprites_sprite_swarm | A bouncing swarm + the per-sprite OAM throughput ceiling |

---

## Scrolling

Move a world past the camera.

| Example | Description |
|---------|-------------|
| @subpage examples_scrolling_mixed_scroll | Multiple BG layers at different scroll rates |
| @subpage examples_scrolling_continuous_scroll | Streaming scroll with dynamic tile loading |
| @subpage examples_scrolling_parallax_scroll | Per-scanline HDMA parallax scrolling |

---

## Mode 7

The rotate/scale plane — the SNES's signature trick.

| Example | Description |
|---------|-------------|
| @subpage examples_mode7_rotate_scale | Mode 7 rotation and scaling |
| @subpage examples_mode7_perspective | Pseudo-3D perspective (F-Zero style) |
| @subpage examples_mode7_perspective_rotate | Full Mode 7 matrix rotation per scanline (krom port) |

---

## HDMA & raster

Change a register every scanline.

| Example | Description |
|---------|-------------|
| @subpage examples_hdma_gradient_colors | HDMA + CGRAM color gradients |
| @subpage examples_hdma_hdma_indirect_gradient | Indirect HDMA pointer-table gradient (krom port) |
| @subpage examples_hdma_hdma_wave | HDMA scanline wave distortion |
| @subpage examples_hdma_hdma_wave_table | Raw HDMA table in C, krom-style repoint animation |
| @subpage examples_hdma_hdma_helpers | High-level HDMA effect library (wave, ripple, iris) |

---

## Colour

Colour math, palette bypass, and beating the 256-colour limit.

| Example | Description |
|---------|-------------|
| @subpage examples_color_palette_cycle | Palette cycling: animate by rotating CGRAM, no pixels moved |
| @subpage examples_color_transparency | Color math (add/subtract blending) |
| @subpage examples_color_shadow_tint | Shadow & tint: darken or colour-cast a whole scene via fixed-colour math |
| @subpage examples_color_direct_color | Direct color mode: the 8bpp pixel byte IS the RGB color |
| @subpage examples_color_gradient_9bit | Brightness-dithered "9-bit" gradient backdrop (krom port) |
| @subpage examples_color_hicolor_1792 | 1792 colors from a 4bpp background via per-tile-row HDMA (krom port) |
| @subpage examples_color_hicolor_blend | 3840 colors via RGB channel-split blend (krom port) |

---

## Windows

Mask part of the screen — static or HDMA-shaped.

| Example | Description |
|---------|-------------|
| @subpage examples_windows_window | Hardware window masking |
| @subpage examples_windows_window_multi_hdma | Both hardware windows animated per scanline (krom port) |
| @subpage examples_windows_transparent_window | Color math + HDMA windowed transparency |

---

## Transitions

How a scene leaves.

| Example | Description |
|---------|-------------|
| @subpage examples_transitions_fading | Brightness control, screen transitions |
| @subpage examples_transitions_mosaic | Mosaic pixelation effect |

---

## Input & peripherals

Controller input: joypads, mouse, and Super Scope.

| Example | Description |
|---------|-------------|
| @subpage examples_input_controller | Standard joypad: button state, edge detection |
| @subpage examples_input_move_sprite | Drive a sprite with the D-pad |
| @subpage examples_input_two_players | Joypad reading, two-player movement |
| @subpage examples_input_mouse | Mouse detection, cursor, sensitivity |
| @subpage examples_input_superscope | Light gun detection, PPU H/V counters |

---

## Audio

Tracker music (SNESMOD), the raw-APU engine from C, and DSP tricks.

| Example | Description |
|---------|-------------|
| @subpage examples_audio_snesmod_music | SPC700 music playback, transport controls (LoROM) |
| @subpage examples_audio_snesmod_music_large | Large soundbank: multi-bank module split |
| @subpage examples_audio_snesmod_sfx | Sound effects alongside music |
| @subpage examples_audio_soundboard | Audio v2 engine driven entirely from C (resident SPC700 driver) |
| @subpage examples_audio_apu_switch | Hot-swapping APU programs at runtime |
| @subpage examples_audio_play_noise | A drum kit from the S-DSP white-noise generator (krom port) |
| @subpage examples_audio_pitch_mod | Hardware vibrato via pitch modulation (PMON) (krom port) |
| @subpage examples_audio_speech_synth | BRR speech playback: the SNES says "OPEN SNES" (krom port) |
| @subpage examples_audio_echo | S-DSP echo / reverb, isolated — START toggles dry/wet |

---

## Maps

Tile-based maps, streaming, Tiled pipeline, and collision.

| Example | Description |
|---------|-------------|
| @subpage examples_maps_map_scroll | Smooth tile-aligned scrolling with viewport tracking |
| @subpage examples_maps_tiled | Tiled Map Editor (.tmx) integration |
| @subpage examples_maps_dynamic_map | Dynamic tile map streaming |
| @subpage examples_maps_slope_collision | Slopes and tile-based collision |

---

## Game math & mechanics

The reusable logic toolbox.

| Example | Description |
|---------|-------------|
| @subpage examples_basics_collision_demo | Bounding-box sprite collision detection |
| @subpage examples_basics_aim_target | Aim a cursor at moving targets — sprites + input |
| @subpage examples_basics_fix32_orbit | 16.16 fixed-point API: a sprite orbits the screen centre |
| @subpage examples_basics_random | LCG pseudo-random number generation |
| @subpage examples_basics_timer | Frame-accurate timers with VBlank counters |
| @subpage examples_basics_scene_stack | Scene stack: title → game → pause workflow |
| @subpage examples_basics_panel_hud | 9-slice HUD + dialog box on one layer (the `panel` module) |
| @subpage examples_basics_game_skeleton | The smallest complete game: title → play → game-over state machine |

---

## Memory & mappers

Persistence and ROM mapping.

| Example | Description |
|---------|-------------|
| @subpage examples_memory_hirom_demo | HiROM vs LoROM memory mapping |
| @subpage examples_memory_save_game | SRAM battery-backed saves |

---

## Enhancement chips

Cartridge coprocessors that extend the SNES beyond its base hardware.

### SA-1 Coprocessor

The SA-1 is a second 65816 CPU running at 10.74 MHz (3x main CPU speed),
sharing I-RAM with the main CPU for inter-processor communication.

| Example | Description |
|---------|-------------|
| @subpage examples_chips_sa1_hello | Boot diagnostic: SA-1 init, I-RAM handshake, register verification |
| @subpage examples_chips_sa1_starfield | 128-dot Lissajous murmuration driven by SA-1 math |

### SuperFX (GSU)

The SuperFX is a custom RISC processor (GSU) with built-in pixel PLOT,
hardware multiply, and direct framebuffer access for 3D and bitmap effects.

| Example | Description |
|---------|-------------|
| @subpage examples_chips_superfx_hello | Boot + SRAM + FMULT hardware tests |
| @subpage examples_chips_superfx_3d | Rotating wireframe cube (Star Fox style 3D) |

---

## Games (capstones)

Complete game projects combining multiple subsystems.

| Example | Description |
|---------|-------------|
| @subpage examples_games_breakout | Breakout clone: sprites, input, game logic, scoring |
| @subpage examples_games_tetris | Tetris with Korobeiniki music, multi-line clear |
| @subpage examples_games_likemario | Platformer with scrolling, animation, physics |
| @subpage examples_games_mapandobjects | Map engine with interactive objects |
| @subpage examples_games_shmup_1942 | Vertical shoot 'em up (Kenney Pixel Shmup assets) |
| @subpage examples_games_mode7_racing | F-Zero-style racer on the rotating Mode 7 plane |
| @subpage examples_games_mode7_flying | Pilotwings-style flight: altitude drives Mode 7 scale |
| @subpage examples_games_rpg | RPG template: Tiled map drives collision and entities, dialog boxes |
