# shmup_1942

Vertical "1942-style" shoot 'em up built iteratively on Kenney's CC0
*Pixel Shmup* pack. Lands in stages so each commit is a working ROM.

![Screenshot](screenshot.png)

## Current stage — S2: vertical auto-scroll

Extends the S1 procedural scene to **256×512 pixels (two screens
tall)** and scrolls it vertically at 1 px per frame. The 32×64
tilemap is loaded once into VRAM and the SNES BG scroll register
cycles through the full map height, wrapping back to the top after
about 8.5 s at 60 fps. Water channels, dirt patches with tents, and
tree clusters come into view and pass below the visible window as the
"camera" moves upward.

## Algorithm

```
1. Pure-grass fill on the 16×32 macro grid (256×512 px).
2. Water mask via cellular automata + post-CA edge re-seed.
   → 15-tile sand-in-water autotile resolves the coast (NESW
     pattern lookup, missing T-junctions derived via H/V flip).
3. Dirt mask via cellular automata on grass-interior cells.
   → Same autotile, runtime RECOLOURED (sand→dirt-brown,
     water→grass-green) gives organic dirt patches with curved
     boundaries.
4. Tents stacked on the largest dirt-patch components (≥3 cells,
   centre column with a 3-cell vertical run).
5. Trees clustered near grass-to-water boundary (Poisson-disk
   sampling), bushes and rings sparse.
6. `gfx4snes` converts the 256×512 scene to .pic + .pal + .map.
7. `main.c` loads the bundle into BG1 with SC_32x64 (two-screen
   tilemap), then in the main loop:
        scroll_y = (scroll_y + 1) % 512;
        bgSetScroll(0, 0, scroll_y);
        WaitForVBlank();
   The hardware scrolls smoothly because `bgSetScroll` defers the
   register write to the next NMI via a dirty flag.
```

`SEED = 17` is the same seed as S1; the algorithm naturally extended
to the taller grid produces a varied vertical scrolling landscape.

## SNES Concepts shown

- Mode 1 (4bpp BG1, up to 16 colours per palette slot)
- SC_32x64 tilemap (2 KB VRAM, 32 wide × 64 tall)
- `bgSetScroll()` deferred register write via NMI dirty flag
- Wrap-around scroll using modulo on a `u16` counter
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
percentages (water `0.32`, dirt `0.28`) to shift biome balance.

## Modules Used

`console`, `dma`, `background`, `asset`.

## Roadmap

| Stage | What it adds |
|-------|--------------|
| S1 | gfx4snes pipeline + procedural land-with-water-channels |
| **S2** *(this stage)* | 256×512 scene + SC_32x64 + vertical auto-scroll |
| S3 | Player ship (sprite) with D-pad movement |
| S4 | Basic enemy spawns with sprite pool |
| S5 | Bullets + AABB collision + explosions on the tents |
| S6 | HUD (score, lives, power-ups) |

## Licence

Source artwork: CC0 from [Kenney](https://kenney.nl/assets/pixel-shmup).
See [ATTRIBUTION.md](../../../ATTRIBUTION.md).
