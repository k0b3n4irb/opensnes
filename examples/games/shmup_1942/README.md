# shmup_1942

Vertical "1942-style" shoot 'em up built iteratively on Kenney's CC0
*Pixel Shmup* pack. Lands in stages so each commit is a working ROM.

![Screenshot](screenshot.png)

## Current stage — S4: enemy spawns with a sprite pool

Up to eight red chasseurs spawn from above the screen every 30 frames,
drift downward at 2 px/frame, and despawn when they exit the bottom.
The blue player ship (S3) and the auto-scrolling terrain (S2) keep
working unchanged.

The architecture: player and enemies share the OAM but occupy distinct
VRAM tile regions and distinct sprite palettes. Each on-screen enemy
is one OAM entry referencing the same 16 enemy tiles — the pool is
in CPU state, not a per-enemy VRAM copy.

## Architecture

```
VRAM (word addresses, fed to VMADDR)
  $0000 ─ BG1 tilemap         (SC_32x64, 2 KB)
  $2000 ─ BG1 tiles           (49 unique 8×8 tiles)
  $6000 ─ Player sprite tiles (16 tiles padded to 64 OAM-grid slots)
  $6400 ─ Enemy  sprite tiles (idem)

CGRAM
  128-143 ─ player palette  (sprite palette 0)
  144-159 ─ enemy  palette  (sprite palette 1)

OAM
  slot 0      ─ player (tile 0,  palette 0)
  slot 1..8   ─ enemy pool (tile 64, palette 1)
                inactive slots hidden via off-screen Y (240)

main loop (per frame)
  - WaitForVBlank()
  - scroll_y = (scroll_y + 1) % 512;
  - D-pad → player position (clamped to visible area)
  - every ENEMY_SPAWN_PERIOD frames: find inactive slot, spawn at top
  - enemies_update(): drift down at 2 px/frame, despawn at y ≥ 224
  - enemies_render(): oamSetFast for each of the 8 slots
  - bgSetScroll(0, 0, scroll_y);
```

`oamSetFast()` (zero-overhead macro) replaces `oamSet()` (framesize=158)
because the loop now does 9 sprite updates per frame — beyond the
2-3-per-frame jitter limit documented for the function form.

## Gotcha: dmaCopyVram word vs byte addressing

The first cut put enemies at `vramAddr = 0x6800` because I'd computed
that as `$6000 + 64 × 32` byte offsets. Wrong: `dmaCopyVram()` takes a
**word** VRAM address (it writes the value straight to `REG_VMADDR`,
which the PPU interprets in 16-bit words). The correct word offset is
`64 × 16 = 0x400` past the base, so enemy tiles live at `vramAddr =
0x6400`. The README's note here is louder than a doc comment because
that mismatch silently looked like a transparent sprite; nothing in
the build flagged it.

## SNES Concepts shown

- VRAM region planning for two sprite types in the same OAM tile grid
- `OBJ_SIZE32_L64` for 32×32 sprites
- Distinct sprite palettes loaded via `dmaCopyCGram(pal, 128+N*16, …)`
- Sprite pool pattern: fixed OAM slot allocation per logical entity
- Off-screen Y trick (240) to hide inactive sprites without disturbing
  attributes
- `oamSetFast()` macro for ≥4-sprite-per-frame loops
- 16-bit LCG for spawn jitter (`x = x * 25173 + 13849`)

## Build

```bash
cd examples/games/shmup_1942 && make
```

Produces `shmup_1942.sfc` (LoROM, 256 KB).

## Re-generating assets

```bash
python3 res/extract_sprites.py   # re-extract player + enemy
python3 res/compose_scene.py     # redraw the terrain (bump SEED)
make
```

## Modules Used

`console`, `dma`, `background`, `asset`, `sprite`, `input`.

## Roadmap

| Stage | What it adds |
|-------|--------------|
| S1 | gfx4snes pipeline + procedural land-with-water-channels |
| S2 | 256×512 scene + SC_32x64 + vertical auto-scroll |
| S3 | Player ship (sprite) + D-pad movement |
| **S4** *(this stage)* | Enemy spawns with sprite pool (≤8 simultaneous) |
| S5 | Bullets + AABB collision + explosions on the tents |
| S6 | HUD (score, lives, power-ups) |

## Licence

Source artwork: CC0 from [Kenney](https://kenney.nl/assets/pixel-shmup).
See [ATTRIBUTION.md](../../../ATTRIBUTION.md).
