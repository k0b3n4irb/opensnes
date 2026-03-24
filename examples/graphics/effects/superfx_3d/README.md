# SuperFX 3D Cube

> Auto-rotating wireframe cube with Y+X axis rotation and Bresenham line drawing

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
make -C examples/graphics/effects/superfx_3d
```

Then open `superfx_3d.sfc` in bsnes (recommended).

## What You'll Learn

- 3D vertex rotation using a 256-entry sine lookup table
- Two-axis rotation (Y then X) with 8-bit fixed-point math
- CPU-side projection and edge buffer construction
- GSU Bresenham line drawing via PLOT into a bitmap framebuffer
- Chunked VBlank DMA: 16KB framebuffer split into 4x4KB transfers (one per VBlank) to avoid flicker

## Modules Used

| Module | Purpose |
|--------|---------|
| `console` | System initialization |
| `sprite` | OAM setup (required by consoleInit) |
| `dma` | Chunked SRAM-to-VRAM framebuffer transfer |
| `background` | BG mode and tilemap configuration |
| `input` | Joypad reading |
| `superfx` | GSU detection and initialization |
