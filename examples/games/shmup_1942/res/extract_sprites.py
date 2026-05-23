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
    """Draw a 32×32 bullet sprite: small yellow ball anchored top-centre,
    transparent everywhere else. Background colour goes to palette index
    0 = transparent (same trick as the ships). The ball is 6×6 px so the
    collision hit-box around its centre stays tight."""
    BG = (0, 0, 0)
    OUTLINE = (78, 51, 11)
    BODY = (252, 195, 51)
    HIGHLIGHT = (255, 232, 122)

    img = Image.new("RGB", (TILE, TILE), BG)
    d = ImageDraw.Draw(img)
    # 8×8 ball centred horizontally at canvas (16, 8). Drawn as a 6×6
    # body with a 1-px outline ring and a 2-px highlight crescent on the
    # top-left — gives the ball a sense of volume at SNES resolution.
    cx = TILE // 2
    cy = 8
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
