# SuperFX Bitmap

> GSU framebuffer rendering -- the GSU fills SRAM with tile data, then the SNES CPU DMAs it to VRAM for display.

![Screenshot](screenshot.png)

## Emulator Compatibility

> **Important**: SuperFX (GSU) examples require a SuperFX-capable emulator.

| Emulator | Status |
|----------|--------|
| **bsnes** | Recommended -- cycle-accurate SuperFX emulation |
| **Mesen2** | Works for boot/registers, but has a bug with GSU backward branches |
| **snes9x** | Does not detect SuperFX in our ROM header |

The screenshot above was captured with opensnes-emu (snes9x-based), which shows
a black screen because snes9x does not emulate SuperFX. On a capable emulator,
the top half of the screen should display colored content rendered by the GSU.

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/graphics/effects/superfx_bitmap
```

Then open `superfx_bitmap.sfc` in bsnes or Mesen2.

## What You'll Learn

- The full GSU-to-SRAM-to-DMA-to-VRAM rendering pipeline
- How the GSU uses PLOT to write pixels in SNES bitplane format
- Setting up an identity tilemap for framebuffer-style display
- Configuring SCMR for PLOT mode (color depth, screen height, bus ownership)
- DMA from SRAM bank $70 to VRAM

## How It Works

1. The SNES CPU sets up Mode 1 with a rainbow palette and an identity tilemap at VRAM $4000 (tile N maps to tile index N)
2. `gsuInit()` detects the SuperFX coprocessor
3. `launchGSU()` runs the GSU program via the WRAM stub pattern
4. The GSU program (`gsu_render.sfx`) clears 16KB of SRAM to black, then uses PLOT to draw a row of red pixels at Y=0
5. After the GSU finishes, the SNES CPU DMAs 16KB from SRAM $70:0000 to VRAM $0000
6. The identity tilemap displays the framebuffer on screen

## Pipeline Architecture

```
GSU PLOT/STW --> SRAM $70:0000 --> DMA ch0 --> VRAM $0000 --> Identity tilemap --> Screen
                                    |                            |
                            Source bank $70               VRAM $4000 (32x16 tiles)
                            Mode $01 (2-reg)              tile[i] = i
                            Dest $18 (VMDATAL)
```

## Work in Progress

The PLOT rendering is partially working. The STW-based pipeline (clearing SRAM,
bulk writes) is validated on bsnes. PLOT pixel rendering has known issues:

- Pixel position may not match expectations (pink line appears at Y~127 instead of Y=0)
- Color index mapping between PLOT and the palette needs investigation
- This is an active area of development -- contributions and debugging help welcome

## Project Structure

| File | Purpose |
|------|---------|
| `main.c` | Mode/palette setup, GSU launch, DMA to VRAM |
| `gsu_loader.asm` | WRAM stub, GSU launch, identity tilemap setup, SRAM-to-VRAM DMA |
| `gsu_render.sfx` | GSU program: clear SRAM + PLOT pixel row |
| `Makefile` | `USE_SUPERFX := 1`, `GSUSRC := gsu_render.sfx` |

## Modules Used

| Module | Purpose |
|--------|---------|
| console | System initialization |
| sprite | OAM management |
| dma | DMA transfers |
| background | BG configuration |
| text | Text display (fallback if GSU not detected) |
| superfx | GSU coprocessor driver (`gsuInit()`) |
