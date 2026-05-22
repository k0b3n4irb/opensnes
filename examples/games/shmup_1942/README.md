# shmup_1942

Vertical "1942-style" shoot 'em up built iteratively on Kenney's CC0
*Pixel Shmup* pack. Lands in stages so each commit is a working ROM.

![Screenshot](screenshot.png)

## Current stage — S1: static scene render

A 256×256 grass-biome screen composed from the 16×16 source tiles in
`res/ground.png` by `res/compose_scene.py`:

- Pure grass fill across the whole screen (the source pack has exactly
  one borderless grass tile — `(6,2)` — so we tile it everywhere)
- Twelve trees scattered off-grid for visual texture
- Two red tent assemblies (roof + body + flag, three tiles vertical) as
  enemy bases — these will be bombable in S5
- Bushes in the four corners + a couple of wooden rings for breaks

The composed `scene.png` (12 unique colours) feeds `gfx4snes`, which
emits `.pic` / `.pal` / `.map`. `main.c` hands the bundle to `bgLoad()`
for a single static frame on BG1 in Mode 1. No movement, no sprites,
no scrolling.

## SNES Concepts shown

- Mode 1 (4bpp BG1, up to 16 colours per palette slot)
- `DECLARE_BG_ASSET` bundling tiles + palette + tilemap
- `bgLoad()` one-shot VRAM/CGRAM setup
- VRAM layout: tilemap at $0000, tile graphics at $4000

## Build

```bash
cd examples/games/shmup_1942 && make
```

Produces `shmup_1942.sfc` (LoROM, 256 KB).

## Re-composing the scene

`res/compose_scene.py` is the source of truth for the scene layout —
edit the tile selections and tent/tree positions there, then re-run:

```bash
python3 res/compose_scene.py
make
```

## Modules Used

`console`, `dma`, `background`, `asset`.

## Roadmap

| Stage | What it adds |
|-------|--------------|
| **S1** *(this stage)* | gfx4snes pipeline + composed static scene |
| S2 | Player ship (sprite) with D-pad movement |
| S3 | Vertical auto-scroll of the BG |
| S4 | Basic enemy spawns with sprite pool |
| S5 | Bullets + AABB collision + explosions on the tents |
| S6 | HUD (score, lives, power-ups) |

## Licence

Source artwork: CC0 from [Kenney](https://kenney.nl/assets/pixel-shmup).
See [ATTRIBUTION.md](../../../ATTRIBUTION.md).
