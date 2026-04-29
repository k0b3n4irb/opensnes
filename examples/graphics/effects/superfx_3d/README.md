# SuperFX 3D Cube

> Auto-rotating wireframe cube at 60 FPS — Star Fox style 3D on SNES

![Screenshot](screenshot.png)

## Emulator Compatibility

| Emulator | Status |
|----------|--------|
| **Mesen2** | ✅ Recommended — runs at 60 FPS with HDMA blanking |
| **bsnes** | ✅ Cycle-accurate; useful as a second reference |
| **snes9x** | ❌ Does not detect GSU despite correct header — example shows "GSU: NOT DETECTED" |

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/graphics/effects/superfx_3d
```

Then open `superfx_3d.sfc` in Mesen2 (recommended) or bsnes.

## What You'll Learn

- **3D rotation**: Y+X axis rotation using 256-entry sine table (C-side)
- **Bresenham line drawing**: GSU draws 12 edges via PLOT (all 8 octants)
- **HDMA screen blanking**: 40 scanlines forced blank top + bottom (like Star Fox)
- **Scanline-polled DMA**: 16KB framebuffer transferred at 60 FPS
- **BG scroll centering**: VOFS=208 centers 128px framebuffer in visible area
- **Vertex clamping**: projected coords clamped to framebuffer bounds (0-126)
- **SuperFX library API**: `gsuLaunch()`, `gsuSetupHdmaBlanking()`, `gsuDmaFullFrame()`

## Architecture

```
SNES CPU (C)                    GSU (SuperFX ASM)
─────────────                   ─────────────────
sin/cos table ──→ rotation      Clear framebuffer (STW loop)
8 vertices    ──→ projection    Read 12 edges from SRAM $4000
12 edges      ──→ SRAM $4000   Bresenham line drawing via PLOT
gsuLaunch()   ──→ GSU starts   ──→ STOP
                                    ↓
HDMA blanking ──→ forced blank  DMA 16KB SRAM → VRAM
gsuDmaFullFrame()               ──→ display on BG1
```

## Per-Example Files

| File | Purpose |
|------|---------|
| `main.c` | Rotation, projection, edge building (100% C) |
| `gsu_3d.sfx` | Bresenham + PLOT rendering (SuperFX assembly) |
| `gsu_loader.asm` | Minimal: `.incbin` + `gsuSetProgram` + `writeEdgesToSRAM` |

## Modules Used

| Module | Purpose |
|--------|---------|
| `console` | System initialization |
| `sprite` | OAM setup (required by consoleInit) |
| `dma` | SRAM-to-VRAM framebuffer DMA |
| `background` | BG mode, tilemap, scroll |
| `input` | Joypad reading (future: manual rotation) |
| `superfx` | GSU library: launch, HDMA, DMA, tilemap |
