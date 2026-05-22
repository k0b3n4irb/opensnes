# shmup_1942

Vertical "1942-style" shoot 'em up built iteratively on Kenney's CC0
*Pixel Shmup* pack. Lands in stages so each commit is a working ROM.

![Screenshot](screenshot.png)

## Current stage — S1: static tile-library preview

Proves the asset pipeline end to end: `gfx4snes` converts
`res/ground.png` (the 192×112 grass-biome terrain library, 18 → 16
colours after quantisation) into `.pic` / `.pal` / `.map`, and `main.c`
hands the bundle to `bgLoad()` for a single static frame on BG1 in
Mode 1.

The 24×14 tile-library occupies the top of the 32×32 screen; the rest
fills with tile 0 from the library. **It is not a designed level** —
the actual screen composition, player ship and scrolling come in S2
and S3.

## SNES Concepts shown

- Mode 1 (4bpp BG1, up to 16 colours)
- `DECLARE_BG_ASSET` bundling tiles + palette + tilemap
- `bgLoad()` one-shot VRAM/CGRAM setup
- VRAM layout: tilemap at $0000, tile graphics at $4000

## Build

```bash
cd examples/games/shmup_1942 && make
```

Produces `shmup_1942.sfc` (LoROM, 256 KB).

## Modules Used

`console`, `dma`, `background`, `asset`.

## Roadmap

| Stage | What it adds |
|-------|--------------|
| **S1** *(this stage)* | gfx4snes pipeline + static tilemap render |
| S2 | Player ship (sprites) with D-pad movement |
| S3 | Vertical auto-scroll of the BG |
| S4 | Basic enemy spawns with sprite pool |
| S5 | Bullets + AABB collision + explosions |
| S6 | HUD (score, lives, power-ups) |

## Licence

Source artwork: CC0 from [Kenney](https://kenney.nl/assets/pixel-shmup).
See [ATTRIBUTION.md](../../../ATTRIBUTION.md).
