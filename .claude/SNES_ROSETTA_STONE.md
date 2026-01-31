# SNES Development Rosetta Stone

**OpenSNES vs PVSnesLib Implementation Guide**

This document bridges knowledge between PVSnesLib (reference SDK) and OpenSNES. For hardware specifications, see **SNES_HARDWARE_REFERENCE.md**.

---

## Table of Contents

1. [PVSnesLib Sprite Engine Deep Dive](#1-pvsneslib-sprite-engine-deep-dive)
2. [PVSnesLib gfx4snes Tool Analysis](#2-pvsneslib-gfx4snes-tool-analysis)
3. [Build Systems Comparison](#3-build-systems-comparison)
4. [OpenSNES vs PVSnesLib](#4-opensnes-vs-pvsneslib)
5. [Recommended Improvements for OpenSNES](#5-recommended-improvements-for-opensnes)
6. [HDMA Techniques](#6-hdma-techniques)
7. [Mode 7 Techniques](#7-mode-7-techniques)
8. [VBlank Timing and DMA Budget](#8-vblank-timing-and-dma-budget)
9. [Debugging Techniques](#9-debugging-techniques)
10. [Real-World Lessons](#10-real-world-lessons)
11. [Code Patterns and Anti-Patterns](#11-code-patterns-and-anti-patterns)

---

## 1. PVSnesLib Sprite Engine Deep Dive

### t_sprites Structure (16 bytes)

```c
typedef struct {
    s16 oamx;           // 0-1: X position on screen
    s16 oamy;           // 2-3: Y position on screen
    u16 oamframeid;     // 4-5: Frame index in graphic file
    u8 oamattribute;    // 6: Sprite attribute (vhoopppc)
    u8 oamrefresh;      // 7: =1 if graphics need reloading
    u8 *oamgraphics;    // 8-11: Pointer to graphic file
    u16 dummy1;         // 12-13: Padding
    u16 dummy2;         // 14-15: Padding
} t_sprites;

extern t_sprites oambuffer[128];  // In WRAM bank $7E
```

### Lookup Tables for VRAM Addressing

PVSnesLib uses lookup tables to convert frame IDs to VRAM addresses:

**For 16x16 Sprites:**
```asm
lkup16oamS:  ; VRAM source offsets (where to read graphics from)
    .word $0000,$0040,$0080,$00c0,$0100,$0140,$0180,$01c0
    .word $0400,$0440,$0480,$04c0,$0500,$0540,$0580,$05c0
    ; Each 16x16 sprite = 64 bytes (4 tiles × 16 bytes)

lkup16idT:   ; OAM tile IDs (what tile number to put in OAM)
    .word $0100,$0102,$0104,$0106,$0108,$010A,$010C,$010E
    .word $0120,$0122,$0124,$0126,$0128,$012A,$012C,$012E
    ; Tile IDs skip by 2 (each 16x16 uses tiles N and N+1)

lkup16idB:   ; VRAM destination block addresses
    .word $0000,$0020,$0040,$0060,$0080,$00A0,$00C0,$00E0
    .word $0200,$0220,$0240,$0260,$0280,$02A0,$02C0,$02E0
    ; Where to place sprite graphics in VRAM
```

**For 32x32 Sprites:**
```asm
lkup32oamS:
    .word $0000,$0080,$0100,$0180,$0800,$0880,$0900,$0980
    ; Each 32x32 sprite = 128 bytes (4×4=16 tiles × 8 bytes for 2bpp)

lkup32idT:
    .word $0000,$0004,$0008,$000C,$0040,$0044,$0048,$004C
    ; Tile IDs skip by 4 (each 32x32 uses 4 tiles per row)
```

### Dynamic Sprite Engine Flow

```
1. Initialize:
   oamInitDynamicSprite(largeVRAM, smallVRAM, largeOAM, smallOAM, sizeMode)

2. Each Frame:
   oamInitDynamicSpriteScreen()     // Reset frame counters

   for each sprite:
       oambuffer[i].oamx = x
       oambuffer[i].oamy = y
       oambuffer[i].oamframeid = frame
       oambuffer[i].oamrefresh = 1  // Mark for VRAM update
       oambuffer[i].oamgraphics = &graphics
       oamDynamic16Draw(i)          // Queue sprite

   oamInitDynamicSpriteEndFrame()   // Hide unused sprites
   WaitForVBlank()
   oamVramQueueUpdate()             // DMA graphics during VBlank
```

### Queue Entry Structure

```
oamQueueEntry: 128 entries × 6 bytes each
  Bytes 0-2: Graphics source pointer (24-bit)
  Bytes 3-4: VRAM destination address (16-bit)
  Byte 5: Sprite size identifier
```

### Metasprite Structure

```c
typedef struct {
    s16 dx, dy;      // Delta position from origin
    u16 dtile;       // Tile offset from base
    u8 props;        // Palette, priority, flip flags
    u8 unused;       // Alignment
} t_metasprite;

#define METASPR_ITEM(dx,dy,tile,attr) {(dx),(dy),(tile),(attr)}
#define METASPR_TERM {-128}  // End marker

// Example: 32x32 metasprite from 4×16x16 sprites
const t_metasprite hero32_frame0[] = {
    METASPR_ITEM(0,  0,  0, OBJ_PAL(0) | OBJ_PRIO(2)),  // Top-left
    METASPR_ITEM(16, 0,  1, OBJ_PAL(0) | OBJ_PRIO(2)),  // Top-right
    METASPR_ITEM(0,  16, 6, OBJ_PAL(0) | OBJ_PRIO(2)),  // Bottom-left
    METASPR_ITEM(16, 16, 7, OBJ_PAL(0) | OBJ_PRIO(2)),  // Bottom-right
    METASPR_TERM
};
```

### Drawing Functions

```c
// Static sprites (graphics pre-loaded to VRAM)
void oamSet(u16 id, u16 x, u16 y, u16 tile, u8 attr, u8 size);
void oamSetEx(u16 id, u8 xmsb, u8 size);

// Dynamic sprites (graphics uploaded per-frame)
void oamDynamic8Draw(u16 id);
void oamDynamic16Draw(u16 id);
void oamDynamic32Draw(u16 id);

// Dynamic metasprites
void oamMetaDrawDyn8(u16 id, s16 x, s16 y, u8 *meta, u8 *gfx);
void oamMetaDrawDyn16(u16 id, s16 x, s16 y, u8 *meta, u8 *gfx, u16 size);
void oamMetaDrawDyn32(u16 id, s16 x, s16 y, u8 *meta, u8 *gfx);
```

---

## 2. PVSnesLib gfx4snes Tool Analysis

### Conversion Pipeline

```
Source Image (PNG/BMP)
       ↓
Extract Palette (build color table)
       ↓
Convert to Indexed (palette indices)
       ↓
Reorganize for VRAM (128px wide output)
       ↓
Convert to Bitplane Format (SNES tiles)
       ↓
Output: .pic (tiles), .pal (palette), [.map (tilemap)]
```

### Two-Pass Sprite Conversion

For sprites (no tilemap), gfx4snes uses a two-pass approach:

**Pass 1:** Reorganize sprite blocks to 128px width
```c
// Input: 64x32 image with 8 16x16 sprites
// Arranged as 4x2 grid

// After Pass 1 (newwidth = sprite_width = 16):
// Output: 16px wide, 8 rows of sprites
// Linear sequence: sprite 0, 1, 2, 3, 4, 5, 6, 7
```

**Pass 2:** Convert sprite blocks to 8x8 tiles
```c
// Input: Pass 1 output (sprites in linear order)
// Each 16x16 sprite becomes 4 sequential 8x8 tiles

// After Pass 2 (newwidth = 8):
// Output: 8px wide, tiles in VRAM order
// Sequence: tile 0, 1, 2, 3 (sprite 0), 4, 5, 6, 7 (sprite 1), ...
```

### Key Insight: N/N+1/N+16/N+17 Pattern

The pattern emerges naturally from VRAM width (128px = 16 tiles):

```
After conversion, tiles are placed linearly in 128px-wide buffer:

Row 0: [T0][T1][T2][T3][T4][T5]...
Row 1: [T16][T17][T18][T19]...
Row 2: [T32][T33][T34][T35]...

For a 16x16 sprite at position 0:
- Top-left tile 0 at row 0, col 0 → VRAM tile 0
- Top-right tile 1 at row 0, col 1 → VRAM tile 1
- Bottom-left at row 1, col 0 → VRAM tile 16
- Bottom-right at row 1, col 1 → VRAM tile 17

This matches OAM's expectation of tiles N, N+1, N+16, N+17!
```

### tiles_convertsnes() Algorithm

```c
unsigned char *tiles_convertsnes(
    unsigned char *imgbuf,     // Source pixels
    int imgwidth, int imgheight,
    int blksizex, int blksizey, // Block size (8, 16, or 32)
    int *sizex, int *sizey,     // Input/output block counts
    int newwidth,               // Output width in pixels
    bool isquiet)
{
    // Calculate output dimensions
    int blocks_per_row = newwidth / blksizex;
    int rows = (total_blocks + blocks_per_row - 1) / blocks_per_row;

    // Allocate output buffer
    buffer = malloc(rows * blksizey * newwidth);

    // Position in output (pixel coordinates)
    int x = 0, y = 0;

    // Copy each block from input to output
    for (j = 0; j < input_blocks_y; j++) {
        for (i = 0; i < input_blocks_x; i++) {
            // Copy block line by line
            for (line = 0; line < blksizey; line++) {
                memcpy(&buffer[(y + line) * newwidth + x],
                       &imgbuf[(j*blksizey + line) * imgwidth + i*blksizex],
                       blksizex);
            }

            // Move to next output position
            x += blksizex;
            if (x >= newwidth) {
                x = 0;
                y += blksizey;
            }
        }
    }

    return buffer;
}
```

---

## 3. Build Systems Comparison

### PVSnesLib Toolchain

```
C (.c) → 816-tcc → .ps → 816-opt → .asp → constify → .asm
                                                       ↓
                                                wla-65816 → .obj
                                                       ↓
                                                wlalink → .sfc

Additional tools:
- gfx4snes: Image → SNES tiles
- smconv: IT/XM → SNESMOD soundbank
- bin2txt: Binary → ASM include
```

### OpenSNES Toolchain

```
C (.c) → cproc → QBE IR → w65816 backend → .asm
                                            ↓
                                      wla-65816 → .obj
                                            ↓
                                      wlalink → .sfc

Additional tools:
- gfx4snes: Image → SNES tiles/sprites/metasprites (from PVSnesLib)
- font2snes: Font → SNES tiles
```

### Memory Segment Configuration

**PVSnesLib (linkfile):**
```
[objects]
crt0_snes.obj
main.obj
libpvsneslib.obj

[mapping]
LOROM

[banks]
0 8 16 32 ...
```

**OpenSNES (linkfile):**
```
[objects]
crt0.obj
main.obj
opensnes.lib

[mapping]
LOROM

[banks]
# 32KB per bank for LoROM
SLOT 0: ROM $8000-$FFFF
SLOT 1: RAM $0000-$1FFF
```

---

## 4. OpenSNES vs PVSnesLib

### Feature Comparison

| Feature | OpenSNES | PVSnesLib |
|---------|----------|-----------|
| Compiler | cproc/QBE | 816-tcc |
| Assembler | wla-dx | wla-dx |
| Static sprites | Yes | Yes |
| Dynamic sprites | Yes | Yes |
| Metasprites | Yes | Yes |
| Lookup tables | Yes | Yes |
| SNESMOD | Yes | Yes |
| Mode 7 | Yes | Yes |
| HDMA | Manual | Helper functions |

### API Comparison

**Sprite Initialization:**
```c
// OpenSNES
oamInit();
oamInitGfxSet(tiles, tiles_size, palette, pal_size, 0, 0x0000, OBJ_SIZE16_L32);

// PVSnesLib
oamInit();
oamInitGfxSet(tiles, tiles_size, palette, pal_size, 0, 0x0000, OBJ_SIZE16_L32);
// Nearly identical for static sprites
```

**Setting a Sprite:**
```c
// OpenSNES
oamSet(0, x, y, tile, OAM_ATTR(pal, prio, 0, 0), OAM_LARGE);
oamUpdate();

// PVSnesLib (static)
oamSet(0, x, y, tile, OAM_ATTR(pal, prio, 0, 0), OAM_LARGE);
oamUpdate();
// Nearly identical for static sprites

// PVSnesLib (dynamic)
oambuffer[0].oamx = x;
oambuffer[0].oamy = y;
oambuffer[0].oamframeid = frame;
oambuffer[0].oamgraphics = &sprite_gfx;
oambuffer[0].oamrefresh = 1;
oamDynamic16Draw(0);
// Then later: oamVramQueueUpdate();
```

### Tile Addressing - gfx4snes

> **Note (2026-01-25):** OpenSNES now uses gfx4snes from PVSnesLib (develop branch).
> The original gfx2snes tool has been removed.

**gfx4snes features:**
- Two-pass conversion ensures correct layout
- Output maintains 128px width for VRAM
- Tiles automatically at correct positions
- 16x16 sprites work correctly with hardware
- Metasprite generation with `-T -X -Y` flags

**Usage:**
```bash
# Basic sprite conversion
gfx4snes -s 16 -p -i sprites.png

# Metasprite with definition file
gfx4snes -s 16 -p -T -X 32 -Y 48 -i character.png
```

---

## 5. Recommended Improvements for OpenSNES

### 1. Add Lookup Tables to Sprite Library

**Add to lib/source/sprite.asm:**
```asm
; Lookup tables for 16x16 sprites
.SECTION ".lut_sprite16" SUPERFREE
lkup16oamS:
    .word $0000,$0040,$0080,$00c0,$0100,$0140,$0180,$01c0
    .word $0400,$0440,$0480,$04c0,$0500,$0540,$0580,$05c0
    ; ... (64 entries for 64 sprites)

lkup16idT:
    .word $0000,$0002,$0004,$0006,$0008,$000A,$000C,$000E
    .word $0020,$0022,$0024,$0026,$0028,$002A,$002C,$012E
    ; ...
.ENDS
```

**Add to lib/include/snes/sprite.h:**
```c
extern u16 lkup16oamS[64];  // VRAM source offsets
extern u16 lkup16idT[64];   // OAM tile IDs
```

### 2. Verification Steps

After implementing changes:

```bash
# 1. Rebuild tools
make tools

# 2. Test gfx4snes output
./bin/gfx4snes -s 16 -p -i test_sprite.png
xxd test_sprite.pic | head -20  # Verify tile layout

# 3. Rebuild library and examples
make lib
make examples

# 4. Run black screen tests
./tests/run_black_screen_tests.sh /path/to/Mesen

# 5. Test specific sprite example in Mesen
# Check that 16x16 sprites display correctly
```

---

## 6. HDMA Techniques

HDMA (Horizontal-blank DMA) transfers data during each scanline's horizontal blanking period.

### HDMA Channel Setup

```c
// HDMA Channel 0: Color gradient effect
REG_DMAP0 = 0x00;        // Mode 0: 1 byte per transfer
REG_BBAD0 = 0x21;        // Target: CGADD ($2121)
REG_A1T0L = (u8)table;   // Table address low
REG_A1T0H = (u8)(table >> 8);
REG_A1B0 = (u8)(table >> 16);
REG_HDMAEN = 0x01;       // Enable HDMA channel 0
```

### HDMA Table Format

**Direct Mode** (bit 6 of DMAPx = 0):
```
For each group of scanlines:
  Byte 0: Line count (1-127) or 0 = end
          Bit 7: 1 = repeat same data for N lines
  Bytes 1-N: Data to transfer (depends on transfer mode)

Example (Mode 0, 1 byte per line):
table:
  .db $08, $00    ; 8 lines, color index 0
  .db $08, $10    ; 8 lines, color index 16
  .db $80 | 100   ; Repeat for 100 lines
  .db $20         ; Same color index 32
  .db $00         ; End table
```

### Common HDMA Effects

**1. Color Gradient (Sky/Water):**
```c
// Table: change palette color each scanline
u8 gradient_table[] = {
    1, 0x00,    // 1 line, CGRAM address 0
    1, 0x1F, 0x00,  // Mode 2: write color (blue)
    1, 0x00,
    1, 0x3F, 0x00,  // Slightly lighter blue
    // ... continue for each scanline
    0           // End
};
```

**2. Parallax Scrolling:**
```c
// Change BG scroll position per scanline
// Target: $210D (BG1HOFS) - needs mode 2 (2 bytes to same register)
u8 parallax_table[] = {
    32, 0x00, 0x00,  // 32 lines at scroll 0
    32, 0x10, 0x00,  // 32 lines at scroll 16
    32, 0x20, 0x00,  // 32 lines at scroll 32
    0
};
```

**3. Wavy Water Effect:**
```c
// Sine wave distortion via scroll offset
// Update table each frame with offset sin values
for (int y = 0; y < 224; y++) {
    s16 offset = sin_table[(y + frame) & 0xFF];
    water_table[y*3] = 1;  // 1 line
    water_table[y*3+1] = offset & 0xFF;
    water_table[y*3+2] = offset >> 8;
}
```

---

## 7. Mode 7 Techniques

Mode 7 enables rotation, scaling, and pseudo-3D effects.

### Transformation Matrix

```
[ X' ]   [ A  B ] [ X - X0 ]   [ X0 + H ]
[ Y' ] = [ C  D ] [ Y - Y0 ] + [ Y0 + V ]

Where:
- (X, Y) = Screen coordinates
- (X', Y') = Tilemap coordinates
- (X0, Y0) = Center of rotation
- (H, V) = Scroll offset
- A, B, C, D = Matrix elements (1.7.8 fixed-point)
```

### Matrix Elements for Common Effects

**Rotation by angle θ:**
```c
s16 A = (s16)(cos(theta) * 256);   // cos * 256
s16 B = (s16)(sin(theta) * 256);   // sin * 256
s16 C = (s16)(-sin(theta) * 256);  // -sin * 256
s16 D = (s16)(cos(theta) * 256);   // cos * 256
```

**Scaling by factor S:**
```c
s16 A = (s16)(256 / S);  // 1/S * 256
s16 B = 0;
s16 C = 0;
s16 D = (s16)(256 / S);
```

**Combined Rotation + Scale:**
```c
s16 A = (s16)(cos(theta) * 256 / scale);
s16 B = (s16)(sin(theta) * 256 / scale);
s16 C = (s16)(-sin(theta) * 256 / scale);
s16 D = (s16)(cos(theta) * 256 / scale);
```

### Pseudo-3D Floor/Ceiling (Mode 7 + HDMA)

```c
// For each scanline y from horizon to bottom:
for (int y = horizon; y < 224; y++) {
    // Distance increases as y increases
    int distance = (camera_height * 256) / (y - horizon + 1);

    // Scale inversely with distance
    s16 scale = 256 * 256 / distance;

    // Set matrix for this scanline
    hdma_table[y].A = (cos(angle) * scale) >> 8;
    hdma_table[y].B = (sin(angle) * scale) >> 8;
    hdma_table[y].C = (-sin(angle) * scale) >> 8;
    hdma_table[y].D = (cos(angle) * scale) >> 8;
}
```

---

## 8. VBlank Timing and DMA Budget

### VBlank Duration

| Region | VBlank Lines | Total Cycles |
|--------|--------------|--------------|
| NTSC | 37 (225-261) | ~50,468 |
| PAL | 87 (225-311) | ~118,668 |

**Usable time:**
- NTSC: ~14,100 cycles (after NMI overhead)
- PAL: ~33,200 cycles

### DMA Budget

DMA transfers at ~2.68 MB/second:
- **~5.9 KB per VBlank (NTSC)** theoretical maximum
- **~4-4.5 KB practical** due to NMI handling, CPU setup

```c
// Bytes transferable in VBlank (conservative)
#define VBLANK_DMA_BUDGET  4500  // Safe for NTSC
```

### Real-World DMA Budgets

| Transfer | Size | Status |
|----------|------|--------|
| OAM (full) | 544 bytes | Fast |
| CGRAM (full) | 512 bytes | Fast |
| One tilemap | 2 KB | Safe |
| Two tilemaps | 4 KB | ~3ms |
| Tilemap + palette + OAM | ~3.5 KB | Safe |
| Two tilemaps + palette | ~4.5 KB | Borderline |

### Safe DMA Pattern

```c
void update_vram(void) {
    WaitForVBlank();

    // Order matters: smallest first, then largest
    // 1. OAM first (544 bytes) - sprites must update
    dmaCopyOam(oam_buffer, 544);

    // 2. Palette changes (variable)
    if (palette_dirty) {
        dmaCopyCGram(palette, 0, 512);
        palette_dirty = 0;
    }

    // 3. Tilemap updates last (can be largest)
    if (tilemap_dirty) {
        dmaCopyVram(tilemap, vram_addr, 2048);
        tilemap_dirty = 0;
    }
}
```

---

## 9. Debugging Techniques

### Mesen2 Debugging

**Memory Watch:**
```
View → Memory Tools
- Select "WRAM" for variables
- Select "VRAM" for graphics
- Select "CGRAM" for palette
- Select "OAM" for sprites
```

**Breakpoints:**
```
Debug → Debugger
- Break on write to $7E:0100 (watch variable)
- Break on read from $4218 (joypad)
- Break on NMI (execution at NMI vector)
```

### Symbol File Analysis

```bash
# Check symbol placement
grep "variable_name" game.sym

# Find RAM symbols (should be $0000-$1FFF or $7E:xxxx)
grep "^00:0" game.sym | head -20

# Find ROM symbols (should be $8000-$FFFF)
grep "^00:[89A-Fa-f]" game.sym | head -20

# Check for WRAM mirror collision
python3 tools/symmap/symmap.py --check-overlap game.sym
```

### Common Debug Patterns

**Black Screen:**
1. Check if code reaches main() - add infinite loop, verify in debugger
2. Verify NMI is enabled ($4200 = $80 or $81)
3. Check screen is on ($2100 bit 7 = 0, brightness > 0)
4. Verify TM register enables layers ($212C)

**Corrupt Graphics:**
1. Check VRAM address in tilemap pointers
2. Verify palette loaded to correct CGRAM address
3. Check DMA source address and bank
4. Look for VRAM region overlaps

**Sprites Not Showing:**
1. Verify OAM uploaded during VBlank
2. Check Y position (240 = hidden)
3. Verify X position MSB in high table
4. Check OBJSEL for correct base address

---

## 10. Real-World Lessons

### Case Study: Breakout "Pink Border" Bug

**Symptom:** During level transitions, a pink flash appeared for one frame.

**Root Cause:** VRAM tilemaps overlapped, and DMA transfers were split across VBlanks.

**VRAM Layout:**
```
BG1 tilemap: 0x0000-0x07FF (2KB)
BG3 tilemap: 0x0400-0x0BFF (2KB)
Overlap:     0x0400-0x07FF (shared 1KB!)
```

**The Bug:**
```c
// BAD - Split DMA causes corruption
WaitForVBlank();
dmaCopyVram(blockmap, 0x0000, 0x800);  // Writes 0x0000-0x07FF
WaitForVBlank();                        // Frame renders HERE with corrupt BG3!
dmaCopyVram(backmap, 0x0400, 0x800);   // Writes 0x0400-0x0BFF
```

**The Fix:**
```c
// GOOD - Atomic update in single VBlank
WaitForVBlank();
dmaCopyVram(blockmap, 0x0000, 0x800);
dmaCopyVram(backmap, 0x0400, 0x800);  // Same VBlank!
```

**Lesson:** When VRAM regions overlap, ALL related DMA transfers must complete in the SAME VBlank.

### Case Study: Sprite 0 Corruption

**Symptom:** Sprite 0 displayed garbage while sprites 1-127 worked fine.

**Root Cause:** OAM buffer in WRAM overlapped with another variable due to WRAM mirroring ($00:0000-$1FFF = $7E:0000-$1FFF).

**Fix:** Use `ORGA $0300 FORCE` for buffers in Bank $7E to avoid the mirrored region.

### Case Study: Static Variables in ROM

**Symptom:** Variables initialized in C code never changed.

**Root Cause:** QBE compiler places initialized static variables in ROM.

```c
// BAD - lives is in ROM, can't be modified!
static u8 lives = 3;

// GOOD - uninitialized static is in RAM
static u8 lives;  // C guarantees zero-init

void init(void) {
    lives = 3;  // Set initial value at runtime
}
```

---

## 11. Code Patterns and Anti-Patterns

### Patterns That Work

**1. Frame Update Loop:**
```c
void main(void) {
    init_graphics();
    init_game();

    while (1) {
        padUpdate();
        update_game();

        WaitForVBlank();
        oamUpdate();

        if (bg_dirty) {
            dmaCopyVram(tilemap, 0x0000, 0x800);
            bg_dirty = 0;
        }
    }
}
```

**2. Safe Palette Animation:**
```c
palette[color_index] = new_color;
palette_dirty = 1;

// In VBlank handler
if (palette_dirty) {
    dmaCopyCGram(&palette[color_index], color_index * 2, 2);
    palette_dirty = 0;
}
```

### Anti-Patterns to Avoid

**1. DON'T: DMA outside VBlank:**
```c
// BAD
dmaCopyVram(data, 0x0000, 1024);
WaitForVBlank();  // Too late!

// GOOD
WaitForVBlank();
dmaCopyVram(data, 0x0000, 1024);
```

**2. DON'T: Assume static init works:**
```c
// BAD - Initialized static is in ROM
static u16 score = 0;

// GOOD - Uninitialized static, set in init
static u16 score;
void init(void) { score = 0; }
```

**3. DON'T: Ignore WRAM mirroring:**
```c
// BAD - Both might map to same physical RAM!
// Bank 0: variable at $0100
// Bank $7E: buffer at $0100

// GOOD - Use ORGA to avoid mirror zone
.RAMSECTION ".buf" BANK $7E SLOT 1 ORGA $0300 FORCE
```

**4. DON'T: Split overlapping VRAM updates:**
```c
// BAD
WaitForVBlank();
dmaCopyVram(bg1, 0x0000, 0x800);
WaitForVBlank();
dmaCopyVram(bg3, 0x0400, 0x800);

// GOOD
WaitForVBlank();
dmaCopyVram(bg1, 0x0000, 0x800);
dmaCopyVram(bg3, 0x0400, 0x800);
```

---

## Appendix A: Joypad Button Mapping

```
16-bit combined value (JOY1H << 8 | JOY1L):

Bit: 15  14  13  12  11  10   9   8   7   6   5   4   3-0
     B   Y  Sel Sta Up  Dn  Lt  Rt   A   X   L   R  (ID)

Button constants:
#define KEY_B      0x8000  // Bit 15
#define KEY_Y      0x4000  // Bit 14
#define KEY_SELECT 0x2000  // Bit 13
#define KEY_START  0x1000  // Bit 12
#define KEY_UP     0x0800  // Bit 11
#define KEY_DOWN   0x0400  // Bit 10
#define KEY_LEFT   0x0200  // Bit 9
#define KEY_RIGHT  0x0100  // Bit 8
#define KEY_A      0x0080  // Bit 7
#define KEY_X      0x0040  // Bit 6
#define KEY_L      0x0020  // Bit 5
#define KEY_R      0x0010  // Bit 4
```

---

## Appendix B: Useful Constants and Macros

### Tile and Palette Macros

```c
// Create tilemap entry
#define TILE(num, pal, prio, hflip, vflip) \
    ((num) | ((pal) << 10) | ((prio) << 13) | ((hflip) << 14) | ((vflip) << 15))

// Create OAM attribute byte
#define OAM_ATTR(pal, prio, hflip, vflip) \
    (((vflip) << 7) | ((hflip) << 6) | ((prio) << 4) | ((pal) << 1))

// Convert RGB to BGR555
#define RGB(r, g, b) (((b) << 10) | ((g) << 5) | (r))
#define RGB8(r, g, b) ((((b) >> 3) << 10) | (((g) >> 3) << 5) | ((r) >> 3))
```

### Screen Size Constants

```c
#define SCREEN_WIDTH   256
#define SCREEN_HEIGHT  224  // NTSC (239 for PAL)

#define TILEMAP_32x32  0x800   // 2KB
#define TILEMAP_64x32  0x1000  // 4KB
#define TILEMAP_64x64  0x2000  // 8KB
```

### DMA Constants

```c
#define DMA_MODE_BYTE      0x00  // 1 byte per transfer
#define DMA_MODE_WORD      0x01  // 2 bytes (VRAM)
#define DMA_TO_PPU         0x00  // A-bus to B-bus
#define DMA_FROM_PPU       0x80  // B-bus to A-bus
```

### Sprite Constants

```c
#define OAM_SIZE_SMALL     0
#define OAM_SIZE_LARGE     1
#define OAM_HIDE_Y         240   // Y position to hide sprite

// OBJSEL size modes
#define OBJ_SIZE8_16       0x00  // 8x8 small, 16x16 large
#define OBJ_SIZE8_32       0x20  // 8x8 small, 32x32 large
#define OBJ_SIZE16_32      0x60  // 16x16 small, 32x32 large
#define OBJ_SIZE16_64      0x80  // 16x16 small, 64x64 large
```

---

## Appendix C: VRAM Layout Examples

### Simple Game Layout (Mode 1)

```
$0000-$07FF: BG1 Tilemap (2KB, 32x32)
$0800-$0FFF: BG2 Tilemap (2KB, 32x32)
$1000-$17FF: BG3 Tilemap (2KB, 32x32)
$2000-$3FFF: BG1/BG2 Tiles (8KB, 4bpp)
$4000-$47FF: BG3 Tiles (2KB, 2bpp)
$6000-$7FFF: Sprite Tiles (8KB, 4bpp)
```

### Overlapping Layout (Memory Efficient)

```
$0000-$07FF: BG1 Tilemap (uses rows 0-15 only)
$0400-$0BFF: BG3 Tilemap (uses rows 8-23, overlaps!)
$1000-$1FFF: BG Tiles
$2000-$3FFF: Sprite Tiles

WARNING: Overlapping layouts require atomic DMA updates!
```

### Mode 7 Layout

```
$0000-$3FFF: Mode 7 Tilemap (16KB, 128x128)
$4000-$7FFF: Mode 7 Tiles (16KB, 256 tiles x 64 bytes)
$8000-$BFFF: Sprite Tiles (for non-Mode-7 layers)
```

---

*Document Version: 3.0 - January 2026*
*OpenSNES vs PVSnesLib Implementation Guide*
*For hardware specifications, see SNES_HARDWARE_REFERENCE.md*
