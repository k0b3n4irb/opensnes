# Super Scope

![Screenshot](superscope.png)

Light gun detection, calibration via PPU H/V counters, and fire tracking with an impact marker sprite — at the calibration target of the city's security range.

## Controls

| Input | Action |
|-------|--------|
| Fire (trigger) | Calibrate aim / mark hit position |
| Pause | Return to calibration screen |
| Cursor | Advance to ready state |

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/input/superscope
```

Then open `superscope.sfc` in an emulator with a Super Scope on port 2
(luna drives one from a test manifest).

## What You'll Learn

- How the SNES Super Scope communicates hit position via PPU H/V counters
- The detect/calibrate/ready state machine pattern for light guns
- Using `OBJSEL()` to configure sprite sizes and VRAM base in one readable macro
- Direct OAM buffer writes for fast single-sprite updates
- 2bpp background graphics on Mode 0 with 4-color palettes

## Walkthrough

### 1. Initialization

```c
consoleInit();
setMode(BG_MODE0, 0);
```

Mode 0 gives four 2bpp BG layers. We use BG1 for text and BG2 for the calibration target image.

### 2. Text System (BG1)

```c
textInit(TEXT_DEFAULT_TILEMAP_ADDR, TEXT_DEFAULT_FONT_TILE, TEXT_DEFAULT_PALETTE);
textLoadFont(0x0000);
bgSetGfxPtr(0, 0x0000);
bgSetMapPtr(0, 0x3800, BG_MAP_32x32);
```

The built-in 2bpp font is loaded to VRAM $0000. BG1's tilemap sits at $3800 to avoid conflicts with BG2 graphics.

### 3. Calibration Background (BG2)

```c
dmaCopyVram(aim_target_tiles, 0x1000, ...);
dmaCopyVram(aim_target_map, 0x2000, ...);
bgSetGfxPtr(1, 0x1000);
bgSetMapPtr(1, 0x2000, BG_MAP_32x32);
```

The crosshair target image is a 256x224 background. Tiles at $1000, tilemap at $2000.

### 4. Sprites

```c
dmaCopyVram(sprites_tiles, 0x4000, ...);
REG_OBJSEL = OBJSEL(OBJ_SIZE16_L32, 0x4000);
```

The `OBJSEL()` macro replaces the cryptic `0x62` with a readable size + base address. `OBJ_SIZE16_L32` means small=16x16, large=32x32. The sprite sheet is the impact marker alone, a 16x16 sprite: tile 0.

### 5. Direct OAM Setup

```c
oamMemory[0] = 0;
oamMemory[1] = 0xE0;     /* Off-screen */
oamMemory[2] = 0x00;     /* Tile 0 (the impact marker) */
oamMemory[3] = 0x30;     /* Priority 3, palette 0 */
oamMemory[512] = 0x00;   /* Small (16x16), X high = 0 */
oam_update_flag = 1;
```

Writing directly to `oamMemory[]` avoids the call and argument pushes of `oamSet()`. For a single sprite updated occasionally, this is clean and fast.

### 6. State Machine

The example uses three states:

- **DETECT**: Calls `scopeInit()` each frame until `scopeIsConnected()` returns true
- **CALIBRATE**: Waits for a fire event, then calls `scopeCalibrate()` to set the aim offset
- **READY**: Reads `scopeGetX()`/`scopeGetY()` on fire events and moves the impact marker

The `fire_armed` flag prevents the calibration fire from immediately re-triggering in the next state (debouncing).

## Tips & Tricks

- **2bpp pictures**: draw them indexed on their four colours (here `devtools/art_assets/build.py` writes the target pixel by pixel): an RGB picture quantized down to 4 colours can lose a thin line that covers few pixels.

- **Sprite tile numbering**: in a 128px-wide sprite sheet of 16x16 sprites, tile numbers follow `row * 32 + col * 2`. `opensnes-sprite inspect` reports the block and tile counts.

- **OBJSEL macro**: Instead of memorizing that `0x62` means "16/32 sizes at VRAM $4000", use `OBJSEL(OBJ_SIZE16_L32, 0x4000)`.

- **Trying it without a gun**: luna drives the Super Scope from a test manifest (`superscope = "frame:x,y,buttons"`, see `testing/manifests/superscope.toml`).

## Under the Hood: The Build

```makefile
TARGET      := superscope.sfc
USE_LIB     := 1
LIB_MODULES := console input sprite dma text background
```

| Module | Why it's here |
|--------|--------------|
| `console` | VBlank/NMI handling |
| `input` | Super Scope detection and reading |
| `sprite` | `oamInit()`, `oamClear()` |
| `dma` | DMA transfers for tiles, maps, palettes |
| `text` | Font loading and text rendering |
| `background` | `bgSetGfxPtr()`, `bgSetMapPtr()`, `bgSetScroll()` |

## Modules Used

`console`, `input`, `sprite`, `dma`, `text`, `background` (`LIB_MODULES` in the Makefile).

## Technical Reference

| Register | Address | Role in this example |
|----------|---------|---------------------|
| OBJSEL | $2101 | Sprite size + tile base address |
| CGADD | $2121 | Palette write address |
| CGDATA | $2122 | Palette color data (RGB555 LE) |
| TM | $212C | Main screen layer enable |
| OPHCT | $213C | PPU horizontal counter (Super Scope X) |
| OPVCT | $213D | PPU vertical counter (Super Scope Y) |

## Files

| File | What's in it |
|------|-------------|
| `main.c` | State machine, sprite control, text display |
| `res/*.png.toml` | the import settings of the target picture (`opensnes-tileset`, 2bpp) and the sprite sheet (`opensnes-sprite`, 32-pixel blocks, 48 colours); the build converts and links them |
| `res/` | Pre-converted graphics (.pic, .pal, .map) and source PNGs |
| `Makefile` | `LIB_MODULES := console input sprite dma text background` |
