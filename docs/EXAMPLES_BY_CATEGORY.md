# Browse Examples by Category {#examples_by_category}

All 60 examples organized by topic. For a progressive learning path, see
@ref learning_path.

---

## Text

Print text, then move it.

| Example | Description |
|---------|-------------|
| @subpage examples_text_print_string | Print a string with the built-in font (`textModeInit` + `textPrintAt`) |
| @subpage examples_text_pixel_text | Proportional text composed at the pixel: tileBlitGlyph2bpp() into a 2bpp layer |

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
| @subpage examples_backgrounds_mode0 | Mode 0: four 2bpp background layers (the city in parallax) |
| @subpage examples_backgrounds_mode3 | Mode 3: 256-colour (8bpp) single layer |
| @subpage examples_backgrounds_mode2 | Offset-per-tile in Mode 2 (two rows) and Mode 4 (one row, 256 colours): per-column scroll from BG3 |
| @subpage examples_backgrounds_mode5_hires | Mode 5 + interlace hi-res text (512×448) (krom port) |

---

## Sprites

Sprite display, animation, and OAM management.

| Example | Description |
|---------|-------------|
| @subpage examples_sprites_simple_sprite | Basic OAM setup, sprite display |
| @subpage examples_sprites_sprite_sizes | OBJSEL size configurations (8x8 to 64x64) |
| @subpage examples_sprites_animated_sprite | Frame animation, sprite sheets, H-flip |
| @subpage examples_sprites_dynamic_sprite | VRAM streaming, dynamic tile uploads; SELECT: metasprites under three OBJSEL size pairs |
| @subpage examples_sprites_sprite_swarm | A bouncing swarm + the per-sprite OAM throughput ceiling |
| @subpage examples_sprites_world_sprites | World-space sprites: a camera, culling and OAM in one call per batch |
| @subpage examples_sprites_aseprite_pipeline | The asset pipeline: an Aseprite export through `opensnes-sprite` into an animated metasprite |

---

## Scrolling

Move a world past the camera.

| Example | Description |
|---------|-------------|
| @subpage examples_scrolling_mixed_scroll | Multiple BG layers at different scroll rates |
| @subpage examples_scrolling_continuous_scroll | A threshold camera over two layers in parallax |
| @subpage examples_scrolling_parallax_scroll | Per-scanline HDMA parallax scrolling |

---

## Mode 7

The rotate/scale plane — the SNES's signature trick.

| Example | Description |
|---------|-------------|
| @subpage examples_mode7_rotate_scale | Mode 7 rotation and scaling |
| @subpage examples_mode7_perspective | Pseudo-3D perspective (F-Zero style) |
| @subpage examples_mode7_perspective_rotate | Full Mode 7 matrix rotation per scanline (krom port) |
| @subpage examples_mode7_dsp1_ground | Super Mario Kart floor: DSP-1 Raster streams the per-scanline matrices |
| @subpage examples_mode7_extbg | EXTBG: one plane split in two layers by bit 7, a sprite between them |
| @subpage examples_mode7_mode7_racing | F-Zero camera: a hover-bike race, rivals placed on the turning plane |

---

## HDMA & raster

Change a register every scanline.

| Example | Description |
|---------|-------------|
| @subpage examples_hdma_gradient_colors | HDMA + CGRAM colour gradients |
| @subpage examples_hdma_hdma_wave | HDMA wave: a hand-built table (krom port), then the hdma module's helper |
| @subpage examples_hdma_hdma_helpers | High-level HDMA effect library (wave, ripple, iris) |

---

## Colour

Colour math, palette bypass, and beating the 256-colour limit.

| Example | Description |
|---------|-------------|
| @subpage examples_color_palette_cycle | Palette cycling: animate by rotating CGRAM, no pixels moved |
| @subpage examples_color_transparency | Colour math (add/subtract blending) |
| @subpage examples_color_shadow_tint | Shadow & tint: darken or colour-cast a whole scene via fixed-colour math |
| @subpage examples_color_direct_color | Direct colour mode: the 8bpp pixel byte IS the RGB colour |
| @subpage examples_color_hicolor_1792 | 1792 colours from a 4bpp background via per-tile-row HDMA (krom port) |

---

## Windows

Mask part of the screen — static or HDMA-shaped.

| Example | Description |
|---------|-------------|
| @subpage examples_windows_window | Hardware window masking |
| @subpage examples_windows_window_multi_hdma | Both hardware windows animated per scanline (krom port) |

---

## Transitions

How a scene leaves.

| Example | Description |
|---------|-------------|
| @subpage examples_transitions_fading | Brightness fades at three speeds and the mosaic dissolve |

---

## Input & peripherals

Controller input: joypads, mouse, and Super Scope.

| Example | Description |
|---------|-------------|
| @subpage examples_input_controller | Standard joypad: button state, edge detection |
| @subpage examples_input_mouse | Mouse detection, cursor, sensitivity |
| @subpage examples_input_superscope | Light gun detection, PPU H/V counters |

---

## Audio

Tracker music (SNESMOD), the raw-APU engine from C, and DSP tricks.

| Example | Description |
|---------|-------------|
| @subpage examples_audio_snesmod_music | SPC700 music playback, transport controls, sound effects over the music |
| @subpage examples_audio_soundboard | Audio v2 engine driven entirely from C (resident SPC700 driver) |
| @subpage examples_audio_sfx_from_wav | One-shot SFX from WAV: a settings file beside each .wav, converted by opensnes-sample |
| @subpage examples_audio_apu_switch | Hot-swapping APU programs at runtime |
| @subpage examples_audio_speech_synth | BRR speech playback: the SNES says "OPEN SNES" (krom port) |

---

## Maps

Tile-based maps, streaming, Tiled pipeline, and collision.

| Example | Description |
|---------|-------------|
| @subpage examples_maps_map_scroll | Smooth tile-aligned scrolling with viewport tracking |

---

## Game math & mechanics

The reusable logic toolbox.

| Example | Description |
|---------|-------------|
| @subpage examples_basics_collision_demo | Bounding-box sprite collision detection |
| @subpage examples_basics_aim_target | Aim, distance and angle; a sprite orbiting in 16.16 fixed point |
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
| @subpage examples_chips_sa1_starfield | 128-dot Lissajous murmuration driven by SA-1 math |

### Super FX (GSU)

The Super FX is a custom RISC processor (GSU) with built-in pixel PLOT,
hardware multiply, and direct framebuffer access for 3D and bitmap effects.

| Example | Description |
|---------|-------------|
| @subpage examples_chips_superfx_3d | Rotating wireframe cube (Star Fox style 3D) |
| @subpage examples_chips_superfx_game_skeleton | A 60 fps game loop while the GSU renders; double-buffered presentation |

### DSP-1 Coprocessor

The DSP-1 (NEC µPD77C25) is a fixed-function fixed-point math coprocessor —
matrix, vector and projection ops for pseudo-3D (Pilotwings, Super Mario Kart).

| Example | Description |
|---------|-------------|

---

## Demos

Playable demonstrations: several modules in one scene, without a game's
rules (no score, no end).

| Example | Description |
|---------|-------------|
| @subpage examples_demos_mapandobjects | Map engine with interactive objects |
| @subpage examples_demos_shmup_1942 | Vertical shoot 'em up (Kenney Pixel Shmup assets) |
| @subpage examples_demos_rpg | RPG template: Tiled map drives collision and entities, dialog boxes |

---

## Games

Complete games, from the title screen to the game over.

| Example | Description |
|---------|-------------|
| @subpage examples_games_breakout | Breakout clone: sprites, input, game logic, scoring |
| @subpage examples_games_blocks | Falling-blocks puzzle with Korobeiniki music, multi-line clear |
| @subpage examples_games_likemario | Kit's Rooftop Run: a platformer with shards, stompable machines, lives, checkpoints and a goal |
