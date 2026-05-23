# shmup_1942

Vertical "1942-style" shoot 'em up built iteratively on Kenney's CC0
*Pixel Shmup* pack. Lands in stages so each commit is a working ROM.

![Screenshot](screenshot.png)

## Final stage — S6: HUD on BG3

The S5 gameplay (player + scrolling terrain + enemy pool + bullets +
collision) is now topped with a HUD on BG3 showing live score (`S
NNN`) and lives (`L N`). Each enemy hit increments the score by 10
and sets a dirty flag; the next frame's main loop repaints the BG3
tilemap via `textPrintAt` + `textPrintU16`. Lives default to 3 and
don't decrement yet (no enemy bullets in this roadmap).

## Architecture

```
VRAM (BYTE / WORD addresses)
  $0000 / $0000  BG1 tilemap          (SC_32x64, 4 KB)
  $1000 / $0800  BG3 tilemap          (32×32, 2 KB)
  $2000 / $1000  BG3 font tiles       (96 chars × 16 B = 1.5 KB, 2bpp)
  $4000 / $2000  BG1 tiles            (49 unique 8×8)
  $C000 / $6000  Sprite tiles         (player + enemy + bullet)

CGRAM
   0-15  BG1 palette 0 (terrain)
  16-19  BG3 palette 4 (HUD: transparent + white)
 128-143 sprite palette 0 (player)
 144-159 sprite palette 1 (enemy)
 160-175 sprite palette 2 (bullet)

OAM
   0       player
   1..8    enemy pool
   9..16   bullet pool

setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
setMainScreen(TM_BG1 | TM_BG3 | TM_OBJ);
```

## Footguns encountered (mémorisés in-code)

The text-module + BG3 setup tripped four silent failure modes in a
row — all documented in the code so the next person doesn't redo
the chase:

1. **API address-unit inconsistency.** `textInit()` takes a BYTE
   address; `bgSetMapPtr()`, `bgSetGfxPtr()`, `textLoadFont()` all
   take WORD addresses. Mismatching them puts BG3 reading from one
   spot while the text module writes to another — symptom is a
   blank HUD with no error.

2. **`bgSetGfxPtr()` only addresses `$1000`-WORD boundaries.** The
   character base register stores `vramAddr / $1000`, so word
   addresses like `$0C00` silently truncate to base 0 and collide
   with BG1's tilemap at byte `$0000`. Tiles must land at word
   `$0000`, `$1000`, `$2000`, etc.

3. **`BG3_MODE1_PRIORITY_HIGH` is mandatory for HUD overlay.**
   Without it, BG3 draws BEHIND BG1 — score text gets hidden under
   the terrain.

4. **Bank `$00` ROM budget.** Adding `text` + a couple of string
   labels pushed bank 0 to 12 bytes free, below the 16-byte
   fail threshold. Long labels (`"SCORE\0"` + `"LIVES\0"` = 12 B)
   spilled to bank 1 = silently rendered as garbage. Shortened to
   `"S"` and `"L"` — 4 bytes total, sits comfortably in bank 0.

## Build

```bash
cd examples/games/shmup_1942 && make
```

Produces `shmup_1942.sfc` (LoROM, 256 KB).

## Re-generating assets

```bash
python3 res/extract_sprites.py   # re-extract player + enemy + draw bullet
python3 res/compose_scene.py     # redraw the terrain (bump SEED)
make
```

## Modules Used

`console`, `dma`, `background`, `asset`, `sprite`, `input`, `text`.

## Roadmap recap

| Stage | What it added |
|-------|---------------|
| S1 | gfx4snes pipeline + procedural land-with-water-channels |
| S2 | 256×512 scene + SC_32x64 + vertical auto-scroll |
| S3 | Player ship (sprite) + D-pad movement |
| S4 | Enemy spawn pool (≤8 simultaneous) |
| S5 | Bullets + AABB collision |
| **S6** *(this stage)* | HUD on BG3 (score / lives via `text` module) |

## Licence

Source artwork: CC0 from [Kenney](https://kenney.nl/assets/pixel-shmup).
See [ATTRIBUTION.md](../../../ATTRIBUTION.md).
