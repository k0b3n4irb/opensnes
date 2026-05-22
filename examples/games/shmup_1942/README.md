# shmup_1942

Vertical "1942-style" shoot 'em up built iteratively on Kenney's CC0
*Pixel Shmup* pack. Lands in stages so each commit is a working ROM.

![Screenshot](screenshot.png)

## Current stage — S1: procedural land-with-water-channels

A 256×256 grass-biome screen composed by `res/compose_scene.py`, with
the same visual language as the official Kenney preview render: **grass
is the base biome and water carves into it from off-screen**, with a
sand beach band at every grass/water boundary and small organic dirt
patches scattered inland.

## Algorithm

```
1. Pure-grass fill everywhere.
2. Water mask (cellular automata, ~32% fill, 4 iters)
   + forced edge seed on 2 random sides (post-CA re-seeded so visible
     water reaches the screen edge).
   → rendered with the 15-tile sand-in-water autotile from rows 3-5
     cols 7-11 of ground.png, looked up by NESW neighbour pattern.
     The two missing T-junctions (0b0111 and 0b1110) are derived at
     runtime by flipping their symmetric counterparts.
3. Dirt mask (cellular automata, ~20% fill, 3 iters)
   masked to grass-interior cells (dist ≥ 2 from water).
   → rendered with a runtime-RECOLOURED copy of the same 15-tile
     autotile (sand→dirt-brown, sand-edge→dirt-highlight, water→grass)
     producing organic dirt patches with curved boundaries.
4. Tents placed on the largest dirt-patch connected components,
   wherever there's a 3-cell vertical column to stack the roof + body
   + flag.
5. Tree clusters via Poisson-disk min-distance sampling, biased
   toward grass cells 1-3 cells away from water (vegetation grows
   along coasts).
6. Bushes and wooden rings sparse on the remaining open interior.
```

The runtime recolour for the dirt autotile uses an exact RGB→RGB
mapping derived from sampling the existing dirt-in-grass autotile
(rows 3-5 cols 1-3). Same pattern lookup as the water pass — only
the rendered tile images differ.

`SEED = 17` was picked from a 10-seed comparison sweep — it gives
the most natural distribution of water, dirt and tents for a
1942-style combat scene.

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

```bash
python3 res/compose_scene.py   # regenerates scene.png
make                            # rebuilds the ROM
```

Bump `SEED` in the script for a different layout, or tune the CA fill
percentages (water `0.32`, dirt `0.20`) to shift biome balance.

## Modules Used

`console`, `dma`, `background`, `asset`.

## Roadmap

| Stage | What it adds |
|-------|--------------|
| **S1** *(this stage)* | gfx4snes pipeline + procedural land-with-water-channels |
| S2 | Player ship (sprite) with D-pad movement |
| S3 | Vertical auto-scroll of the BG |
| S4 | Basic enemy spawns with sprite pool |
| S5 | Bullets + AABB collision + explosions on the tents |
| S6 | HUD (score, lives, power-ups) |

## Licence

Source artwork: CC0 from [Kenney](https://kenney.nl/assets/pixel-shmup).
See [ATTRIBUTION.md](../../../ATTRIBUTION.md).
