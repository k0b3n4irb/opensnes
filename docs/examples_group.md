# Example Source Code {#example_sources}

@defgroup examples Examples
@brief All 41 OpenSNES example programs with annotated source code.

Each example demonstrates specific SNES hardware concepts. The source files
include `@par SNES Concepts` (what hardware features are used),
`@par What to Observe` (what to look for in the emulator), and
`@par Modules Used` (which library modules to link).

Browse by category: @ref examples_by_category

## By Topic

### Getting Started
- @ref hello_world/main.c "Hello World" — Hand-coded font, Mode 0, direct VRAM writes
- @ref text_test/main.c "Text Module" — textInit/textPrintAt/textFlush workflow

### Backgrounds
- @ref mode0/main.c "Mode 0" — Four 2bpp layers with parallax
- @ref mode1/main.c "Mode 1" — Standard 16-color 4bpp background
- @ref mode3/main.c "Mode 3" — 256-color 8bpp with assembly DMA loader
- @ref mode5/main.c "Mode 5" — Hi-res 512px with main+sub screen
- @ref mode7/main.c "Mode 7" — Rotation and scaling
- @ref mode1_lz77/main.c "LZ77 Compression" — Compressed tile data
- @ref mode1_bg3_priority/main.c "BG3 Priority" — HUD overlay technique
- @ref continuous_scroll/main.c "Continuous Scroll" — Streaming tile columns
- @ref mixed_scroll/main.c "Mixed Scroll" — Dual-layer compositing

### Sprites
- @ref simple_sprite/main.c "Simple Sprite" — OAM basics
- @ref animated_sprite/main.c "Animated Sprite" — Frame cycling and direction flip
- @ref dynamic_sprite/main.c "Dynamic Sprite" — VRAM streaming engine
- @ref metasprite/main.c "Metasprite" — Multi-tile composite sprites
- @ref object_size/main.c "Object Size" — OBJSEL size modes

### Effects
- @ref fading/main.c "Fading" — INIDISP brightness transitions
- @ref mosaic/main.c "Mosaic" — Pixelation effect
- @ref transparency/main.c "Transparency" — Color math (addition/subtraction)
- @ref window/main.c "Window" — Clipping regions
- @ref transparent_window/main.c "Transparent Window" — Window + color math
- @ref gradient_colors/main.c "Gradient Colors" — HDMA per-scanline palette
- @ref hdma_wave/main.c "HDMA Wave" — Sine distortion
- @ref hdma_helpers/main.c "HDMA Helpers" — Library helper effects
- @ref parallax_scrolling/main.c "Parallax" — HDMA scroll offsets

### Input
- @ref mouse/main.c "Mouse" — SNES Mouse peripheral
- @ref superscope/main.c "Super Scope" — Light gun detection and calibration
- @ref two_players/main.c "Two Players" — Simultaneous controller input

### Audio
- @ref snesmod_music/main.c "Music" — Tracker playback via SPC700
- @ref snesmod_sfx/main.c "Sound Effects" — Standalone SFX with pitch control

### Maps
- @ref dynamic_map/main.c "Dynamic Map" — Extended WRAM tilemaps
- @ref slopemario/main.c "Slope Mario" — Slope collision with map engine

### Memory
- @ref hirom_demo/main.c "HiROM" — 64KB bank memory mapping
- @ref save_game/main.c "Save Game" — SRAM battery-backed save slots

### Games
- @ref breakout/main.c "Breakout" — Complete game with collision and palette cycling
- @ref likemario/main.c "LikeMario" — Platformer with scroll streaming and physics
- @ref mapandobjects/main.c "Map & Objects" — Map engine with enemy AI
- @ref tetris/main.c "Tetris" — State machine, DAS input, HDMA effects

### Basics
- @ref collision_demo/main.c "Collision Demo" — AABB and tile collision
