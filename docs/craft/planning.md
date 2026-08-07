# Planning your SNES game {#craft_planning}

The single most common way a first SNES game dies is not a bug — it is a
scene that cannot fit in the hardware, discovered *after* the art is drawn and
the code is written. This guide is the planning you do first: budget the three
scarce resources, pick a background mode from your genre, and scope something
you can actually finish.

For the register-level detail behind every number here, see
@ref snes_graphics_guide. This guide is about the *decisions*.

## The three budgets

A SNES scene lives inside three fixed pools. Plan them on paper before you
draw, the way you would a level's layout.

### 1. VRAM — 64 KB, shared by everything on screen

VRAM holds your background tiles, your background maps, **and** your sprite
tiles, all at once. It is one pool, not three. The cost of a tile depends only
on its colour depth:

| Colour depth | Colours | Bytes per 8×8 tile |
|--------------|---------|--------------------|
| 2bpp | 4 | 16 |
| 4bpp | 16 | 32 |
| 8bpp | 256 | 64 |

A background *map* is separate from its tiles: a 32×32 screen of map entries
is **2 KB** (each entry is 2 bytes — tile number, flip, palette, priority). A
64×64 scrolling map is 8 KB.

Rule of thumb, and your worksheet:

```
VRAM used  =  BG tiles     (bytes/tile × unique tiles, per layer)
           +  BG maps      (2 KB per 32×32 screen, ×4 for a 64×64 map)
           +  sprite tiles (bytes/tile × unique sprite tiles)
           +  font/UI tiles
           must stay under 64 KB — all of it, simultaneously.
```

The lever: **unique** tiles are what cost you, not screen size — a background
built from a small, reused tileset is nearly free to make bigger. This is why
16×16 metatiles and tile reuse are the backbone of SNES level art.

### 2. CGRAM — 256 colours, split BG vs sprites

CGRAM holds exactly 256 colours (512 bytes, 15-bit BGR). In the common 4bpp
modes it is eight 16-colour sub-palettes, and the near-universal convention is:

- **BG layers use palettes 0–7 (colours 0–127)**
- **sprites use palettes 8–15 (colours 128–255)**

Colour 0 of every sub-palette is transparent, so a 16-colour sprite really
gives you 15 visible colours plus transparency (the "15+1" convention). Budget
your art *per sub-palette*, and decide early which tiles share which palette —
because re-quantising one image can shift the colours of everything that
shares a sub-palette with it. (A project-level palette planner is on our
tooling roadmap for exactly this reason.)

### 3. OAM — 128 sprites, but far fewer per line

You get 128 hardware sprites total, but the real constraint is per-scanline:
the PPU draws at most **32 sprites** and **34 8×8 tile-slivers** on any single
horizontal line. Cross either limit and sprites drop out — and the cull starts
from the *lowest* OAM priority, so your least-important sprites vanish first.
One more trap worth planning around: a sprite parked off-screen to the left
still counts toward the 32-per-line limit (a documented PPU quirk).

The lever: spread your action vertically, keep big sprites few, and reserve
OAM headroom for the moment everything happens at once. @ref
examples_sprites_sprite_swarm measures exactly where this ceiling bites.

> **Go deeper.** For *why* these limits exist — the VRAM fetch timing and the
> sprite line-buffer — read Fabien Sanglard's
> [SNES PPU article](https://fabiensanglard.net/snes_ppus_why/). It turns the
> numbers above from arbitrary into obvious.

> **Check it, don't guess it — twice.** Before the ROM even runs,
> `make asset-budget` weighs your converted graphics — tiles, maps, palettes —
> against the 64 KB / 256-colour limits: this worksheet, computed from the files
> you built. Then once it runs, `make budget` reports the live VRAM/CGRAM/OAM
> footprint of a scene via luna. The first bounds what you *built* (an upper
> bound — a streaming game ships more than fits at once); the second measures
> what a scene actually *loads* (a lower bound — non-zero content at one frame).
> Between them you know where you stand. Add `ARGS="--only <name>"` to focus
> either on one example.

## Choose a background mode from your genre

The SNES has eight background modes. The wikis tell you what each *is*; here is
how to *pick* one, starting from the game you want to make:

| Your game | Start with | Why | Example |
|-----------|-----------|-----|---------|
| Platformer / action | **Mode 1** | 2 rich (16-colour) layers + 1 cheap (4-colour) layer — art, parallax, and a HUD | @ref examples_backgrounds_mode1 |
| RPG / adventure | **Mode 1**, or **Mode 3** for lavish single-layer art | Mode 1 for maps+menus; Mode 3's 256-colour layer for painterly scenes | @ref examples_backgrounds_mode3 |
| Racer / pseudo-3D | **Mode 7** | The one layer that rotates and scales in hardware | @ref examples_mode7_rotate_scale |
| Puzzle / board / menu-heavy | **Mode 0** | Four cheap layers for grids, backdrops, overlays at low colour cost | @ref examples_backgrounds_mode0 |
| Per-column effects (flag, heat-haze) | **Mode 2** | Offset-per-tile: each column scrolls independently | @ref examples_backgrounds_mode2 |

Default to **Mode 1** unless your game has a specific reason not to — it is
what most of the library used, and every other mode trades a layer or colour
depth for its special power. Put your HUD on the low-priority layer (Mode 1's
BG3) so it draws over the action; see @ref craft_backgrounds for how to
compose layers, and @ref examples_backgrounds_mode1_bg3_priority for the HUD
overlay in practice.

### The cost side: tiles by mode

Each mode also trades layers and colour depth against **how much VRAM a tile
costs**, because a tile's size is fixed by its colour depth (@ref
snes_graphics_guide for the register detail):

| Mode | Layers (colours) | Bytes/tile | The budget trade |
|------|------------------|-----------|------------------|
| 0 | BG1–BG4, all 4-colour (2bpp) | 16 each | Cheapest tiles, most layers — 3 colours + transparent per layer. Grids, menus, board games. |
| **1** | BG1+BG2 16-colour (4bpp), BG3 4-colour (2bpp) | 32 / 32 / 16 | Two rich layers + a cheap HUD/backdrop. The default — best colour-per-byte balance. |
| 2 | BG1+BG2 16-colour (4bpp) | 32 each | Two rich layers with per-column scroll (offset-per-tile); you give up BG3. |
| 3 | BG1 256-colour (8bpp), BG2 16-colour (4bpp) | 64 / 32 | One lavish layer at **2× tile cost** — and rich art dedupes poorly, so a full 8bpp screen runs ~40 KB. No room for a second big layer. See @ref examples_backgrounds_mode3. |
| 4 | BG1 256-colour (8bpp), BG2 4-colour (2bpp) | 64 / 16 | An 8bpp main layer plus a cheap second one, with per-column scroll. |
| 5 | BG1 16-colour (4bpp), BG2 4-colour (2bpp), hi-res | 32 / 16 | Double horizontal resolution (512 px) — the wider display means bigger maps and a tighter sprite budget. |
| 6 | BG1 16-colour (4bpp), hi-res | 32 | One hi-res layer with per-column scroll. |
| 7 | one 256-colour (8bpp) layer, rotate/scale | 64 | Rotation and scaling in hardware, at a **fixed** cost: the 128×128 map is always 16 KB and tiles cap at 256 (16 KB) — roughly 32 KB whatever you draw. See @ref examples_mode7_rotate_scale. |

Two patterns are worth internalising. **8bpp doubles your tile cost** (Modes 3,
4, 7), and a **painterly image dedupes badly** — a game tileset reuses sky and
bricks down to a few KB, but a near-photographic screen keeps most of its 896
cells (32×28) as unique tiles. Together that is why one Mode 3 screen can cost
40 KB while a Mode 1 playfield costs about 3 KB. Mode 7 is the odd one out: its
cost is *fixed* by the 128×128 map, not by your art.

> **See it as numbers.** `make asset-budget` prints exactly this per example —
> the tile/map/palette weight of every mode in the library — so you can weigh
> the trade against a real scene before committing to a mode.

## Scope a first game you can finish

The community's hardest-won lesson, repeated across SNESdev game-jam devlogs:
**ship something tiny.** First projects die from scope, not difficulty.

- **One mechanic, one screen, one win/lose.** @ref examples_basics_game_skeleton
  is that shape — title → play → game-over — and a fine thing to fork.
- **Grey-box first.** Prove the game is fun with solid-colour tiles before you
  commission art. Art is expensive and locks in palette/VRAM decisions.
- **Budget before you draw.** Fill in the VRAM/CGRAM worksheet above for your
  one screen. If it does not fit on paper, it will not fit in the ROM.
- **Cut the second level.** A finished one-level game beats an abandoned epic.
  You can always grow it — reused tiles make a bigger world nearly free.

## Checklist before you write code

- [ ] Genre → background mode chosen (table above)
- [ ] VRAM worksheet fits under 64 KB for your busiest screen
- [ ] Sub-palettes assigned (which tiles share which of the 8)
- [ ] Peak sprites-per-line under 32 in your busiest moment
- [ ] Scope trimmed to one mechanic you can finish

> **Sources for the numbers above:**
> [fullsnes](https://problemkaputt.de/fullsnes.htm),
> [Anomie's SNES docs](https://www.romhacking.net/documents/199/),
> [SNESdev Wiki](https://snes.nesdev.org/wiki),
> and [Bumbershoot Software](https://bumbershootsoft.wordpress.com/2023/10/14/dma-and-fastrom-on-the-snes-speed-at-any-cost/)
> for the VBlank DMA budget.
