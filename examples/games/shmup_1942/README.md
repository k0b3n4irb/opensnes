# shmup_1942

Vertical "1942-style" shoot 'em up using Kenney's CC0 *Pixel Shmup* assets.

## Status

**Work in progress — asset import stage only.** No `Makefile` or `main.c`
yet; this directory currently holds the staged source art and the
reproducible split script that prepares it for `gfx4snes`.

## Assets (`res/`)

| File | Dim | Unique colors | Source | Role |
|------|-----|---------------|--------|------|
| `tiles_packed.png` | 192×160 | 23 | Kenney *Pixel Shmup* | Original packed sheet (committed for traceability) |
| `sprites.png`      | 192×48  | 14 | Split top 3 rows | gfx4snes input — sprite/HUD tiles |
| `ground.png`       | 192×112 | 18 | Split bottom 7 rows | gfx4snes input — BG terrain tiles |
| `ships_packed.png` | 128×192 | 15 | Kenney *Pixel Shmup* | 24 ships @ 32×32 — player + enemies |
| `import.sh`        | —       | —  | — | Reproduces `sprites.png` + `ground.png` from `tiles_packed.png` |

The split exists because the original packed sheet bundles sprite/HUD
tiles with terrain tiles on a single image, but the SNES needs them on
distinct palettes (sprites in CGRAM 128+, BG in CGRAM 0+). `import.sh`
slices the sheet at y=48 and is deterministic at the pixel level.

## Biome

**Grass** for the MVP — the warmest palette in the pack and the most
forgiving under quantisation. `ground.png` ships with all three biomes
present (grass, desert, snow) because the VRAM cost is negligible (84
tiles × 32 bytes = 2.7 KB), and keeping them avoids re-splitting if a
later level wants to swap.

## Licence

All assets are CC0 from [Kenney](https://kenney.nl/assets/pixel-shmup).
See [ATTRIBUTION.md](../../../ATTRIBUTION.md) for the entry.
