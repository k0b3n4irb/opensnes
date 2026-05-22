# shmup_1942

Vertical "1942-style" shoot 'em up built iteratively on Kenney's CC0
*Pixel Shmup* pack. Lands in stages so each commit is a working ROM.

![Screenshot](screenshot.png)

## Current stage — S3: player ship + D-pad

Adds a controllable 32×32 player ship on top of the S2 scrolling
terrain. The ship — `ship_0000` from Kenney's `ships_packed.png`, a
blue chasseur — is extracted by `res/extract_player.py` with its
dark-blue background forced to palette index 0 (transparent in SNES
sprite rendering). The terrain continues its 1 px/frame auto-scroll
while the player moves freely with the D-pad, clamped to a
192×160-px playing field so the sprite stays fully on screen.

## Algorithm / architecture

```
Build
  - res/extract_player.py     ─→ res/player.png      (32×32 indexed)
  - gfx4snes -s 32  on player.png   ─→ player.pic + player.pal
  - gfx4snes -s 8   on scene.png    ─→ scene.pic + scene.pal + scene.map

VRAM map
  $0000 ─ BG1 tilemap        (SC_32x64 = 2 KB)
  $4000 ─ BG1 tiles          (49 unique 8×8 tiles after dedup)
  $6000 ─ Sprite tiles       (16 8×8 tiles for the 32×32 ship, padded
                              to 64 slots for OAM tile-row stride)

main loop (per frame)
  - WaitForVBlank()
  - scroll_y = (scroll_y + 1) % 512;
  - pad = padHeld(0);
    LEFT/RIGHT/UP/DOWN → player_x/_y ±= 2 px (with clamping)
  - oamSet(0, player_x, player_y, …);
  - bgSetScroll(0, 0, scroll_y);
```

`bgSetScroll` and `oamSet` defer to the NMI handler via dirty flags,
so the per-frame work is just a few bytes of buffer writes; the
hardware-register updates happen atomically at VBlank.

## SNES Concepts shown

- Mode 1 (4bpp BG1, up to 16 colours per palette slot)
- SC_32x64 tilemap with continuous vertical auto-scroll
- OAM: `oamInitGfxSet` loads sprite tiles + palette + configures OBJSEL
- `OBJ_SIZE32_L64`: small sprites are 32×32 (our ship), large are 64
- Sprite palette in CGRAM 128+ (we use sprite palette 0 → CGRAM 128-143)
- VRAM region planning to avoid BG/sprite tile clashes
- `padHeld(0)` for read-while-pressed input

## Build

```bash
cd examples/games/shmup_1942 && make
```

Produces `shmup_1942.sfc` (LoROM, 256 KB).

## Re-generating assets

```bash
python3 res/extract_player.py   # if you want to swap the player ship
python3 res/compose_scene.py    # to redraw the terrain (bump SEED)
make
```

## Modules Used

`console`, `dma`, `background`, `asset`, `sprite`, `input`.

## Roadmap

| Stage | What it adds |
|-------|--------------|
| S1 | gfx4snes pipeline + procedural land-with-water-channels |
| S2 | 256×512 scene + SC_32x64 + vertical auto-scroll |
| **S3** *(this stage)* | Player ship (sprite) + D-pad movement |
| S4 | Basic enemy spawns with sprite pool |
| S5 | Bullets + AABB collision + explosions on the tents |
| S6 | HUD (score, lives, power-ups) |

## Licence

Source artwork: CC0 from [Kenney](https://kenney.nl/assets/pixel-shmup).
See [ATTRIBUTION.md](../../../ATTRIBUTION.md).
