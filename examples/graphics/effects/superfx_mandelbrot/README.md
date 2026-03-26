# SuperFX Mandelbrot

> Mandelbrot set fractal computed by the GSU using FMULT and 4.12 fixed-point math

![Screenshot](screenshot.png)

## Emulator Compatibility

| Emulator | Status |
|----------|--------|
| **bsnes** | Recommended -- cycle-accurate SuperFX |
| **Mesen2** | Works (minor DMA timing differences) |
| **snes9x** | Detected with cart type $13 |

> **Note:** opensnes-emu (snes9x-based) shows a blank screen because snes9x WASM does not emulate the SuperFX coprocessor.

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/graphics/effects/superfx_mandelbrot
```

Then open `superfx_mandelbrot.sfc` in bsnes (recommended).

## What You'll Learn

- SuperFX FMULT for 16x16 signed fixed-point multiplication (4.12 format)
- PLOT hardware for per-pixel rendering into a bitplane framebuffer
- LOOP instruction for long-range loops (body > 128 bytes)
- CACHE instruction for ~6x GSU instruction fetch speedup
- Static fractal rendering (~1-2 seconds at 21.47 MHz)

## Modules Used

| Module | Purpose |
|--------|---------|
| `console` | System initialization |
| `sprite` | OAM setup (required by consoleInit) |
| `dma` | SRAM-to-VRAM bitmap transfer |
| `background` | BG mode and tilemap configuration |
| `input` | Joypad reading |
| `superfx` | GSU detection and initialization |
