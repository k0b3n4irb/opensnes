# Mode 3 — 256-Color Background

Demonstrates BG Mode 3 with a single 256-color (8bpp) background layer.

![Screenshot](mode3.png)

## Description

Mode 3 provides one 256-color background, allowing photographic-quality images on the SNES. The tileset (39 KB) is larger than a ROM bank, so the converter writes it in two parts and the example copies each with `dmaCopyVram()`, whose pointer carries the bank.

## Architecture

- **Mode 3**: BG1 at 8bpp (256 colors), BG2 at 4bpp (16 colors, unused here)
- **BG1**: Full 256-color image
- Tileset in two parts (`bg_tiles`, `bg_tiles_1`), two DMA transfers
- Picture converted by the build from `res/bg.bmp` (`res/bg.bmp.toml`)

## Ported from

PVSnesLib "Mode3" example.

## Modules

`console dma background`
