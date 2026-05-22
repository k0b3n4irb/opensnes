# shmup_1942

Vertical "1942-style" shoot 'em up built iteratively on Kenney's CC0
*Pixel Shmup* pack. Lands in stages so each commit is a working ROM.

![Screenshot](screenshot.png)

## Current stage — S1: procedurally-generated archipelago

A 256×256 grass-biome screen composed by `res/compose_scene.py` from
Kenney's tile pack. The composition uses:

- **Cellular automata** to generate a binary land/water mask
  (~52 % initial fill, 4 smoothing iterations of the classic cave-gen
  "5+ neighbours" rule, a 1-cell water border forced around the
  screen). The result is one or more organic island shapes — not
  rectangles.
- **15-tile autotile resolver** for the sand-in-water transition.
  Each of the 15 tiles in the bank (rows 3-5 cols 7-11 of ground.png)
  is auto-classified by sampling its 4 mid-edge pixels for water vs
  land, building a NESW → tile catalog. For every land cell in the
  mask, the four cardinal neighbours determine the 4-bit pattern,
  which selects the appropriate tile from the catalog. The bank is
  missing the all-land interior and the two horizontal T-junctions,
  so those fall back to a pure sand fill (6,8) and the nearest
  available pattern respectively.
- **Layered biomes inside each island**: connected components ≥ 12
  cells get a rectangular grass area dropped inside, then a 3×3-to-5×4
  dirt clearing inside the grass, then a 3-tile red tent stack in the
  centre of the dirt clearing.
- **Decorations**: tree clusters on the grass interior via
  Poisson-disk min-distance sampling (5 cluster centres, each seeding
  2-4 trees with tight gaussian offsets), plus sparse bushes and
  wooden rings.

`SEED = 17` was picked from a 6-seed comparison sweep because it
produces an L-shaped island with the most natural-looking coastline
curvature. Bump SEED for an entirely different layout.

`main.c` hands the converted bundle to `bgLoad()` for a single static
frame on BG1 in Mode 1. No movement, no sprites, no scrolling — those
come in S2-S6.

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
edit the `SEED`, the cellular-automata parameters or the tile catalog,
then re-run:

```bash
python3 res/compose_scene.py
make
```

## Modules Used

`console`, `dma`, `background`, `asset`.

## Roadmap

| Stage | What it adds |
|-------|--------------|
| **S1** *(this stage)* | gfx4snes pipeline + procedural archipelago |
| S2 | Player ship (sprite) with D-pad movement |
| S3 | Vertical auto-scroll of the BG |
| S4 | Basic enemy spawns with sprite pool |
| S5 | Bullets + AABB collision + explosions on the tents |
| S6 | HUD (score, lives, power-ups) |

## Licence

Source artwork: CC0 from [Kenney](https://kenney.nl/assets/pixel-shmup).
See [ATTRIBUTION.md](../../../ATTRIBUTION.md).
