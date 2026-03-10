# Browse Examples by Category {#examples_by_category}

All 37 examples organized by topic. For a progressive learning path, see
@ref learning_path.

---

## Text

Display text on screen using background tiles and fonts.

| Example | Description |
|---------|-------------|
| @subpage examples_text_hello_world | Your first ROM: PPU setup, tiles, palette, tilemap |
| @subpage examples_text_text_test | Text positioning, formatting with consoleDrawText |

---

## Graphics: Backgrounds

Background layers, scrolling, and video modes.

| Example | Description |
|---------|-------------|
| @subpage examples_graphics_backgrounds_mode1 | Mode 1 multi-layer backgrounds (4bpp + 2bpp) |
| @subpage examples_graphics_backgrounds_mode1_bg3_priority | BG3 priority bit for overlay effects |
| @subpage examples_graphics_backgrounds_mode1_lz77 | LZ77-compressed background data |
| @subpage examples_graphics_backgrounds_continuous_scroll | Streaming scroll with dynamic tile loading |
| @subpage examples_graphics_backgrounds_mixed_scroll | Multiple BG layers at different scroll rates |
| @subpage examples_graphics_backgrounds_mode7 | Mode 7 rotation and scaling |
| @subpage examples_graphics_backgrounds_mode7_perspective | Pseudo-3D perspective (F-Zero style) |

---

## Graphics: Sprites

Sprite display, animation, and OAM management.

| Example | Description |
|---------|-------------|
| @subpage examples_graphics_sprites_simple_sprite | Basic OAM setup, sprite display |
| @subpage examples_graphics_sprites_animated_sprite | Frame animation, sprite sheets, H-flip |
| @subpage examples_graphics_sprites_dynamic_sprite | VRAM streaming, dynamic tile uploads |
| @subpage examples_graphics_sprites_object_size | OBJSEL size configurations (8x8 to 64x64) |
| @subpage examples_graphics_sprites_metasprite | Multi-tile composite sprites |

---

## Graphics: Effects

Visual effects using HDMA, color math, and hardware windows.

| Example | Description |
|---------|-------------|
| @subpage examples_graphics_effects_fading | Brightness control, screen transitions |
| @subpage examples_graphics_effects_mosaic | Mosaic pixelation effect |
| @subpage examples_graphics_effects_hdma_wave | HDMA scanline wave distortion |
| @subpage examples_graphics_effects_hdma_gradient | HDMA color gradient per scanline |
| @subpage examples_graphics_effects_hdma_helpers | High-level HDMA effect library (wave, ripple, iris) |
| @subpage examples_graphics_effects_gradient_colors | HDMA + CGRAM color gradients |
| @subpage examples_graphics_effects_parallax_scrolling | HDMA parallax scrolling |
| @subpage examples_graphics_effects_transparency | Color math (add/subtract blending) |
| @subpage examples_graphics_effects_window | Hardware window masking |
| @subpage examples_graphics_effects_transparent_window | Color math + HDMA windowed transparency |

---

## Input

Controller input: joypads, mouse, and Super Scope.

| Example | Description |
|---------|-------------|
| @subpage examples_input_two_players | Joypad reading, two-player movement |
| @subpage examples_input_mouse | Mouse detection, cursor, sensitivity |
| @subpage examples_input_superscope | Light gun detection, PPU H/V counters |

---

## Audio

Music and sound effects via SNESMOD (Impulse Tracker format).

| Example | Description |
|---------|-------------|
| @subpage examples_audio_snesmod_music | SPC700 music playback, transport controls |
| @subpage examples_audio_snesmod_sfx | Sound effects alongside music |

---

## Maps

Tile-based maps, streaming, and collision.

| Example | Description |
|---------|-------------|
| @subpage examples_maps_dynamic_map | Dynamic tile map streaming |
| @subpage examples_maps_slopemario | Slopes and tile-based collision |

---

## Memory

Memory mapping and persistence.

| Example | Description |
|---------|-------------|
| @subpage examples_memory_hirom_demo | HiROM vs LoROM memory mapping |
| @subpage examples_memory_save_game | SRAM battery-backed saves |

---

## Basics

Fundamental game mechanics.

| Example | Description |
|---------|-------------|
| @subpage examples_basics_collision_demo | Bounding-box sprite collision detection |

---

## Games

Complete game projects combining multiple subsystems.

| Example | Description |
|---------|-------------|
| @subpage examples_games_breakout | Breakout clone: sprites, input, game logic, scoring |
| @subpage examples_games_likemario | Platformer with scrolling, animation, physics |
| @subpage examples_games_mapandobjects | Map engine with interactive objects |
