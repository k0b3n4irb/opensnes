# gen_hud_bar.py — HUD Health Bar Sprite Sheet Generator

Generates a 48x8 pixel sprite sheet for SNES HUD health bars, composed of
6 tiles (8x8 each): 3 empty states + 3 filled states.

## When to use

- Generating placeholder HUD bar graphics for RPG or action game projects
- Creating custom health/mana/stamina bar sprite sheets

## Dependencies

- **Pillow** >= 10.0 (`pip install pillow` or handled by `uv sync`)

## Usage

```bash
uv run gen_hud_bar.py
```

The script outputs `res/hud_bar.png` (48x8, indexed 16-color palette).

## Tile Layout

```
Tile:    0          1          2          3          4          5
     back_left  back_mid  back_right fill_left  fill_mid  fill_right
     (empty L)  (empty M) (empty R)  (full L)   (full M)  (full R)
```

## Kenney RPG Assets (Optional)

If Kenney RPG Expansion PNG files are found in the working directory,
they are composited into the sprite sheet for a polished look.
Otherwise, a procedural bar is generated with simple colored rectangles.
