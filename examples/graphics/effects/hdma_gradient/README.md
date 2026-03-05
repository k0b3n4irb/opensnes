# HDMA Gradient

This example demonstrates how to use HDMA (Horizontal Blank DMA) to write different brightness values to the INIDISP register ($2100) on each group of scanlines, creating a vertical brightness gradient across the screen. A full-color 256-color background image is displayed in Mode 3, and HDMA progressively dims the image from top to bottom. Press A to cycle through 15 gradient intensity levels.

![Screenshot](screenshot.png)

## What You'll Learn

- How HDMA transfers work: automatic per-scanline register writes with no CPU involvement
- How the INIDISP register ($2100) controls master brightness (values 0-15)
- How to build and update HDMA tables at runtime
- How to handle large ROM assets that exceed bank $00 (32KB) using assembly DMA loaders

## SNES Concepts

### HDMA and the INIDISP Register

HDMA is a special DMA mode that transfers a small amount of data to a PPU register at the start of every horizontal blanking period (between scanlines). Register $2100 (INIDISP) controls the master screen brightness: bits 0-3 set brightness from 0 (black) to 15 (full), and bit 7 enables forced blanking. By writing different brightness values per scanline group, you create a gradient fade effect entirely in hardware -- the CPU only builds the table once.

### HDMA Table Format (Mode 0)

HDMA mode 0 writes 1 byte per entry to a single register. Each table entry is 2 bytes: a scanline count followed by the data byte. A count of 0 terminates the table. This example creates 16 entries of 14 scanlines each (16 x 14 = 224, covering the full visible screen). The brightness value decreases as you go down the screen:

```
[14] [brightness_0]   — top 14 scanlines at maximum brightness
[14] [brightness_1]   — next 14 scanlines, slightly dimmer
...
[14] [brightness_15]  — bottom 14 scanlines, darkest
[0]                   — terminator
```

### HDMA Channel Setup

The library function `hdmaSetup(channel, mode, dest_register, table_ptr)` configures an HDMA channel. In this example: channel 3, mode 0 (1-register/no-repeat), destination register offset $00 (which maps to $2100 = INIDISP), and a pointer to the gradient table in RAM. The channel is activated with `hdmaEnable(3)` and runs automatically every frame until disabled.

### Large Asset DMA with Bank Bytes

The 8bpp background image requires about 39KB of tile data -- more than the 32KB bank $00 limit. The data is split across two SUPERFREE ROM sections that the linker may place in bank $01+. Since the C library's `dmaCopyVram()` hardcodes bank $00, a custom assembly `loadGraphics` function uses WLA-DX's `:label` syntax to resolve the correct source bank byte at link time, performing 4 separate DMA transfers (two tile chunks, palette, and tilemap).

## Controls

| Button | Action |
|--------|--------|
| A      | Cycle gradient intensity (15 down to 2, then back to 15) |

## How It Works

**1. Graphics loading** -- An assembly function handles all VRAM/CGRAM transfers during force blank, splitting the large tileset across two DMA operations with correct bank bytes:

```c
REG_INIDISP = 0x80;   /* Force blank */
loadGraphics();        /* ASM: tiles, palette, tilemap to VRAM/CGRAM */
```

**2. BG1 configuration** -- Mode 3 is selected for 256-color BG1. The tilemap is at VRAM $0000 and tile data starts at VRAM $1000:

```c
REG_BG1SC = (0x0000 >> 8) | 0x00;  /* Tilemap at $0000, SC_32x32 */
REG_BG12NBA = 0x01;                /* BG1 tile base at $1000 */
setMode(BG_MODE3, 0);
```

**3. Building the gradient table** -- The `buildGradientTable()` function computes 16 brightness steps from the current level down to 0, writing them into a static RAM buffer:

```c
static void buildGradientTable(u8 level) {
    u16 i, idx = 0;
    for (i = 0; i < 16; i++) {
        u16 divisor = 32 / (level + 1);
        if (divisor == 0) divisor = 1;
        u16 step = i / divisor;
        if (step > level) step = level;
        u8 brightness = level - (u8)step;
        hdma_gradient_table[idx++] = 14;          /* scanline count */
        hdma_gradient_table[idx++] = brightness;   /* INIDISP value */
    }
    hdma_gradient_table[idx] = 0;  /* terminator */
}
```

**4. HDMA activation** -- Channel 3 is configured for mode 0 writes to register $00 (INIDISP = $2100), and enabled:

```c
hdmaSetup(3, 0, 0x00, hdma_gradient_table);
hdmaEnable(3);
```

**5. Interactive cycling** -- Pressing A decreases the gradient level (fewer brightness steps = more uniform brightness), wrapping from 2 back to 15. The table is rebuilt in RAM and HDMA picks up the new values on the next frame automatically.

## Project Structure

```
hdma_gradient/
├── main.c          — Gradient table builder, HDMA setup, input handling
├── data.asm        — ROM data: 8bpp tiles (split 32KB+7KB), tilemap, palette, ASM DMA loader
├── Makefile        — Build configuration
└── res/
    └── pvsneslib.png  — 256-color background image (converted to 8bpp tiles at build time)
```

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/graphics/effects/hdma_gradient
```

Then open `hdma_gradient.sfc` in your emulator (Mesen2 recommended).
