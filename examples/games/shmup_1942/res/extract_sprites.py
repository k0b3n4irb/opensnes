#!/usr/bin/env python3
"""Extract the player and enemy ships from Kenney's `ships_packed.png`,
plus draw a programmatic bullet sprite for S5.

Ships:
  - #0  (row 0, col 0) — big blue chasseur → `player.png`
  - #9  (row 2, col 1) — medium red chasseur → `enemy.png`

Each sprite is exported as a 16-colour indexed PNG with the dark-blue
Kenney background forced to palette index 0, because gfx4snes treats
that index as transparent for sprites. PIL's adaptive quantizer puts
the background at an arbitrary index, so the script does an explicit
palette swap to put it at slot 0.

Bullet:
  A 32×32 sprite (same size class as player/enemy so we can keep
  OBJ_SIZE32_L64 globally) with a small yellow vertical pellet drawn
  in the top-centre — everything else is transparent. Drawn with PIL
  rather than extracted from the pack so we control its size precisely.
"""

from pathlib import Path
from subprocess import check_output

from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
SRC = HERE / "ships_packed.png"

TILE = 32


def extract(index: int, out_path: Path) -> None:
    """Crop ship at `index` (row-major in the 4×6 grid) and quantise to
    16 colours with the background at palette index 0."""
    src = Image.open(SRC).convert("RGB")
    row, col = divmod(index, 4)
    ship = src.crop((col * TILE, row * TILE, (col + 1) * TILE, (row + 1) * TILE))

    bg_colour = ship.getpixel((0, 0))

    quant = ship.convert(
        "P",
        palette=Image.Palette.ADAPTIVE,
        colors=16,
        dither=Image.Dither.NONE,
    )
    pal_flat = quant.getpalette()[:48]

    bg_idx = None
    for i in range(16):
        if (pal_flat[i * 3], pal_flat[i * 3 + 1], pal_flat[i * 3 + 2]) == bg_colour:
            bg_idx = i
            break
    if bg_idx is None:
        raise SystemExit(f"could not locate bg colour {bg_colour} in quantised palette")

    pixels = list(quant.getdata())
    swap = {bg_idx: 0, 0: bg_idx}
    new_pixels = [swap.get(p, p) for p in pixels]

    new_palette = list(pal_flat)
    a, b = bg_idx * 3, 0
    new_palette[b:b + 3], new_palette[a:a + 3] = pal_flat[a:a + 3], pal_flat[b:b + 3]
    new_palette += [0] * (768 - len(new_palette))

    out = Image.new("P", quant.size)
    out.putpalette(new_palette)
    out.putdata(new_pixels)
    out.save(out_path)


def build_bullet(out_path: Path) -> None:
    """Draw a 32×32 bullet sprite: small yellow ball at the CANVAS CENTRE,
    transparent everywhere else. The ball must sit at canvas y 12-19
    (centre-y) rather than canvas y 4-11 (top) so it can render at the
    top of the screen via OAM Y wrap-around without the SNES PPU
    dropping the sprite into the 224-255 hidden Y zone.

    Why: SNES OAM Y is 8-bit and the PPU treats top edges in 224-255
    as "below visible". For a 32×32 sprite with visible content at
    canvas y 4-11, putting the ball at world y 0 would require
    sprite_y = -4 → OAM_y = 251 → hidden. Moving the ball down the
    canvas means sprite_y stays near 0 (OAM_y near 0) when the ball
    crosses the top of the screen, and the wrap-around scanlines
    actually fall in the visible 0-31 band."""
    BG = (0, 0, 0)
    OUTLINE = (78, 51, 11)
    BODY = (252, 195, 51)
    HIGHLIGHT = (255, 232, 122)

    SIZE = 16  # 16x16 sprite: can render its top edge near scanline 0
    img = Image.new("RGB", (SIZE, SIZE), BG)
    d = ImageDraw.Draw(img)
    cx = SIZE // 2          # canvas centre x = 8
    cy = SIZE // 2          # canvas centre y = 8
    d.ellipse((cx - 4, cy - 4, cx + 3, cy + 3), fill=OUTLINE)
    d.ellipse((cx - 3, cy - 3, cx + 2, cy + 2), fill=BODY)
    d.rectangle((cx - 2, cy - 2, cx - 1, cy - 1), fill=HIGHLIGHT)

    quant = img.convert(
        "P",
        palette=Image.Palette.ADAPTIVE,
        colors=16,
        dither=Image.Dither.NONE,
    )
    pal_flat = quant.getpalette()[:48]
    bg_idx = None
    for i in range(16):
        if (pal_flat[i * 3], pal_flat[i * 3 + 1], pal_flat[i * 3 + 2]) == BG:
            bg_idx = i
            break
    if bg_idx is None or bg_idx == 0:
        # Already at slot 0 (or absent — shouldn't happen)
        if bg_idx == 0:
            quant.save(out_path)
            return
        raise SystemExit("bullet bg colour missing from quantised palette")
    pixels = list(quant.getdata())
    swap = {bg_idx: 0, 0: bg_idx}
    new_pixels = [swap.get(p, p) for p in pixels]
    new_palette = list(pal_flat)
    a, b = bg_idx * 3, 0
    new_palette[b:b + 3], new_palette[a:a + 3] = pal_flat[a:a + 3], pal_flat[b:b + 3]
    new_palette += [0] * (768 - len(new_palette))

    out = Image.new("P", quant.size)
    out.putpalette(new_palette)
    out.putdata(new_pixels)
    out.save(out_path)


def info(p: Path) -> str:
    return check_output(
        ["identify", "-format", "%wx%h  unique=%k", str(p)]
    ).decode().strip()


if __name__ == "__main__":
    if not SRC.exists():
        raise SystemExit(f"missing {SRC}")
    for (index, name) in [(0, "player.png"), (9, "enemy.png")]:
        out_path = HERE / name
        extract(index, out_path)
        print(f"{name}: {info(out_path)}")
    bullet_path = HERE / "bullet.png"
    build_bullet(bullet_path)
    print(f"bullet.png: {info(bullet_path)}")
