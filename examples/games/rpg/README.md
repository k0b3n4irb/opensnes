# rpg — an RPG template driven by a Tiled map

![Screenshot](screenshot.png)

The SDK's modules composed into a playable RPG skeleton. Two things
make it a *template* rather than a demo:

**1. The map is a real Tiled map.** `res/town.tmj` holds the terrain,
the per-tile collision (each tile's `attribute` property), the entity
positions (the `Entities` object layer: spawn, villagers, chest) *and*
each villager's line of dialogue (a `text` property on the object).
Adding a villager is a map edit, not a code edit. None of it is
hardcoded — open the map in
[Tiled](https://www.mapeditor.org/), edit it, re-run the generator,
rebuild. The map is validated by the SDK's own `tmx2snes` converter.

**2. A real bordered dialog box, and a HUD.** Both are 9-slice panels
on BG2 with their text on BG3 above — the classic SNES RPG window, with
the layers stacking scene < panel < text. The HUD carries hearts and a
purse; the purse goes up by 10 when you open the chest.

**3. Two scenes.** The town, and the inside of the blue-roofed house.
A scene is a tileset + a palette + a tilemap + a collision map +
entities, and the interior is its own Tiled map (`res/house.tmj`) with
its own 16-colour palette — neither scene gives up colours for the
other. Walking onto the door swaps all five; the host greets you with
no button press.

| Input | Action |
|---|---|
| D-pad | walk (one tile per step) |
| A | talk to a villager (face it) / open the chest (step on it) |
| walk into the blue door | enter the house; the mat inside takes you back |

ROM mode: LoROM (project default).

## SNES Concepts

- **Tiled as the content pipeline**: collision and entities are data.
  `gen_assets.py` reads `town.tmj` and emits the SNES tilemap, a
  64×64 collision map and `entities.inc`.
- **Tile-exact collision**: the hero *occupies one tile*; its 16×16
  sprite is drawn straddling that tile (feet on it, body overhanging
  upward) — the standard top-down convention. Drawing the sprite at
  the tile corner instead puts the visible body half a tile from what
  collides, which reads as random "too early / too late" blocking.
- **Several characters, one sprite sheet**: the villagers reuse the
  hero's tiles with a second OBJ palette (CGRAM 144) — recolored, not
  redrawn. The sheet is converted with `-s 16` because it holds 16×16
  frames (see `docs/tutorials/sprites.md`).
- **Off-camera entities must be hidden, not just drawn**: OAM X is 9
  bits and Y is 8, so an entity outside the camera does not vanish —
  its coordinates *wrap* and it reappears somewhere plausible, e.g. a
  villager standing inside the town wall. `draw_char()` parks anything
  off-camera at `OBJ_HIDE_Y`.
- **Forced blank, not brightness 0, around a big VRAM upload**:
  `setScreenOff()` sets INIDISP bit 7, the only state besides VBlank in
  which the PPU accepts VRAM writes. `setBrightness(0)` just makes the
  screen black — the PPU keeps fetching and the write is dropped. The
  2 KB dialog panel got away with it because it happened to land inside
  VBlank; the 8 KB town tilemap on the way out of the house did not, and
  came back as garbage.
- **A fixed 16-colour palette per scene**, authored by hand instead of
  quantised from RGB. With a quantiser, adding one tile re-derives the
  whole palette and every existing tile shifts hue — the town's paths
  turned pink the first time the blue roof was added.
- **Two 9-slice panels on one layer** via the `panel` module: the HUD at
  the top and the dialog box at the bottom share BG2's tilemap, so one
  upload covers both and opening a dialog never disturbs the HUD. The
  module does that upload under forced blank, which is the part that is
  easy to get wrong. `text_config.priority` plus BGMODE bit 3 put the
  text in front of the opaque town.
- **Collision through the SDK's `collideTile()`** over the Tiled map. Its
  `tilemap` parameter is `const`, so the lookup is a bank-honouring far
  read (#121) — the 4 KB map needs neither bank $00 nor a byte of RAM.

## Editing the map

```bash
# open res/town.tmj in Tiled, edit terrain / drag the Entities objects
python3 gen_assets.py --keep-map    # re-read town.tmj + house.tmj, rebuild
make
```

Tile collision is the `attribute` property on each tileset tile
(`FF00` = solid, `0` = walkable). Entities are objects in the
`Entities` layer, typed `spawn`, `npc` or `chest`. An `npc` blocks its
tile (you talk face-to-face) and carries its dialogue in a `text`
property; a `chest` does not block (you step onto it).

Run `python3 gen_assets.py` without `--keep-map` to regenerate the
whole map from the script's layout.

`town.tmj` is a standard Tiled map, so the SDK's own converter reads it
too — a useful check that the file is well-formed:

```bash
../../../bin/tmx2snes -e -Q res/town.tmj res/tileset.map
```

`-Q` writes the quadrant-ordered 64×64 tilemap this example scrolls with
the `background` module, and `-e` writes the entities as C defines —
byte-identical to what `gen_assets.py` produces. The generator still runs
the conversion itself because it also builds the per-cell collision grid
and marks the villagers' tiles blocked, which is game logic rather than
map data; moving the rest onto the tool is a Makefile change.


## How to Build

```bash
make
```

## Modules Used

console, dma, background, sprite, text, input, collision, panel
