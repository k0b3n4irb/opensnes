# shmup_1942

Vertical "1942-style" shoot 'em up built iteratively on Kenney's CC0
*Pixel Shmup* pack. Lands in stages so each commit is a working ROM.

![Screenshot](screenshot.png)

## Current stage — S5: bullets and collision

Press A to fire a yellow pellet upward from the player's nose
(8-frame cooldown between shots). Bullets travel at 4 px/frame.
Bullet/enemy AABB collision deactivates both on hit. Up to eight
bullets and eight enemies live in the OAM simultaneously.

## Architecture

```
VRAM (word addresses, fed to VMADDR)
  $0000 ─ BG1 tilemap         (SC_32x64, 2 KB)
  $2000 ─ BG1 tiles           (49 unique 8×8 tiles)
  $6000 ─ Player tiles        (32×32, OBJ_SIZE32_L64)
  $6400 ─ Enemy  tiles
  $6800 ─ Bullet tiles        (32×32 canvas with a small visible
                               pellet at top-centre; rest is index 0
                               = transparent)

CGRAM
  128-143 ─ player palette  (sprite palette 0)
  144-159 ─ enemy  palette  (sprite palette 1)
  160-175 ─ bullet palette  (sprite palette 2)

OAM slots
  0      ─ player
  1..8   ─ enemy pool   (tile 64,  palette 1, V-flipped)
  9..16  ─ bullet pool  (tile 128, palette 2)

main loop (per frame)
  - WaitForVBlank()
  - scroll_y -= 1 (wrapping 0 → 511): terrain flows DOWN across the
    screen — player flies UP into the world, 1942 convention
  - D-pad → player position (clamped to visible area)
  - KEY_A → bullet_fire() if fire_cd == 0
  - every 32 frames (mask, not modulo): enemy spawn
  - enemies_update(): drift down at 2 px/frame, despawn at y ≥ 224
  - bullets_update(): drift up at 4 px/frame, despawn at y < -32
  - collisions_resolve(): O(MAX_BULLETS × MAX_ENEMIES) AABB scan with
    early-out on inactive slots
  - render: oamSetFast for player + 8 enemies + 8 bullets (17 sprites,
    well under the OAM limit of 128 and within per-frame budget)
```

## Performance gotchas

- **Spawn period must be a power of 2.** First cut used `% 30` for the
  spawn check, which compiles to a software 16×16 division on cc65816
  (no hardware div). At 17 sprite updates per frame plus 64-iteration
  collision scan worst-case, the extra few hundred cycles tipped the
  example over the lag-frame threshold (6/300 = 2 % > 5/300 = 1.7 %
  ceiling). Switching to `& (PERIOD - 1)` with `PERIOD = 32` brought
  it back under budget without changing the visible cadence.
- **`oamSetFast` is mandatory above ~3 sprites per frame.** The
  function form of `oamSet` carries a documented framesize=158 each,
  so doing 17 of those would mean 2700 bytes of stack work alone
  (and visible jitter on real hardware). The macro is zero-overhead.

## Re-generating assets

```bash
python3 res/extract_sprites.py   # re-extract player + enemy, redraw bullet
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
| S4 | Enemy spawns with sprite pool (≤8 simultaneous) |
| **S5** *(this stage)* | Bullets + AABB collision (≤8 bullets) |
| S6 | HUD (score, lives, power-ups) |

## Licence

Source artwork: CC0 from [Kenney](https://kenney.nl/assets/pixel-shmup).
See [ATTRIBUTION.md](../../../ATTRIBUTION.md).
