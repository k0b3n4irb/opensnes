"""The OpenSNES logo: Kit the mascot over an OPENSNES wordmark (assets/).

Imported by build.py. The mascot and his head come from
assets/neo-lumen/logo/ (generated); the wordmark is drawn here, letter by
letter, on a 6x7 grid with two-pixel stems: OPEN in the cyan neon, SNES in
the magenta, each with an ink outline and a dark halo.
"""
from __future__ import annotations

import io
from pathlib import Path

from PIL import Image

GLYPHS = {
    "O": [" #### ", "##  ##", "##  ##", "##  ##", "##  ##", "##  ##", " #### "],
    "P": ["##### ", "##  ##", "##  ##", "##### ", "##    ", "##    ", "##    "],
    "E": ["######", "##    ", "##    ", "##### ", "##    ", "##    ", "######"],
    "N": ["##  ##", "### ##", "######", "## ###", "##  ##", "##  ##", "##  ##"],
    "S": [" #####", "##    ", "##    ", " #### ", "    ##", "    ##", "##### "],
}
INK = (8, 8, 16, 255)
NIGHT = (16, 24, 40, 255)
# per word: the halo, then the fill from the top row to the bottom one
NEON = {
    "cyan": ((24, 96, 112, 255), [(232, 232, 240), (112, 224, 224), (112, 224, 224), (40, 160, 176), (40, 160, 176), (24, 96, 112), (24, 96, 112)]),
    "magenta": ((96, 24, 56, 255), [(232, 232, 240), (224, 80, 144), (224, 80, 144), (160, 40, 96), (160, 40, 96), (96, 24, 56), (96, 24, 56)]),
}


def wordmark(scale: int, outline: int) -> Image.Image:
    """OPENSNES at `scale` (pixels per glyph cell), outlined `outline` px in ink, haloed 1 px."""
    adv = 7 * scale
    pad = outline + 1
    w = 8 * adv - scale + 2 * pad
    h = 7 * scale + 2 * pad
    fill = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    word = Image.new("L", (w, h), 0)
    for k, ch in enumerate("OPENSNES"):
        tone = "cyan" if k < 4 else "magenta"
        for r, row in enumerate(GLYPHS[ch]):
            for c, cell in enumerate(row):
                if cell != "#":
                    continue
                colour = NEON[tone][1][r] + (255,)
                for yy in range(scale):
                    for xx in range(scale):
                        x, y = pad + k * adv + c * scale + xx, pad + r * scale + yy
                        fill.putpixel((x, y), colour)
                        word.putpixel((x, y), 1 if k < 4 else 2)
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px = word.load()
    def near(x, y, d):
        for dy in range(-d, d + 1):
            for dx in range(-d, d + 1):
                xx, yy = x + dx, y + dy
                if 0 <= xx < w and 0 <= yy < h and px[xx, yy]:
                    return px[xx, yy]
        return 0
    for y in range(h):
        for x in range(w):
            if px[x, y]:
                continue
            if near(x, y, outline):
                out.putpixel((x, y), INK)
            else:
                t = near(x, y, outline + 1)
                if t:
                    out.putpixel((x, y), NEON["cyan" if t == 1 else "magenta"][0])
    out.alpha_composite(fill)
    return out


# Kit's head at 16x16, drawn by hand: no reduction of a bigger head keeps both eyes
ICON16 = [
    "..K..........K..",
    ".KOK........KOK.",
    ".KCOK......KOCK.",
    ".KCOOKKKKKKOOCK.",
    ".KOOOOOOOOOOOOK.",
    "KOOOOOOOOOOOOOOK",
    "KOOKKOOOOOOKKOOK",
    "KOOKKOOOOOOKKOOK",
    "KOOOOCCCCCCOOOOK",
    ".KOOCCCKKCCCOOK.",
    ".KDCCCCCCCCCCDK.",
    "..KKCCCCCCCCKK..",
    "..TTKKCCCCKKTT..",
    ".TTTTTKKKKTTTTT.",
    ".TtTTTTTTTTTTtT.",
    "................",
]
ICON16_COLOURS = {"K": (8, 8, 16), "O": (224, 120, 48), "D": (176, 72, 24), "C": (240, 184, 128),
                  "T": (72, 160, 160), "t": (48, 112, 120)}


def icon16() -> Image.Image:
    out = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    for y, row in enumerate(ICON16):
        for x, ch in enumerate(row):
            if ch in ICON16_COLOURS:
                out.putpixel((x, y), ICON16_COLOURS[ch] + (255,))
    return out


def trimmed(path: Path) -> Image.Image:
    im = Image.open(path).convert("RGBA")
    return im.crop(im.getbbox())


def compose(size: tuple[int, int], art: Image.Image, scale: int, outline: int, backdrop=None, gap: int = 6) -> Image.Image:
    w, h = size
    word = wordmark(scale, outline)
    canvas = Image.new("RGBA", size, backdrop or (0, 0, 0, 0))
    total = art.height + gap + word.height
    top = (h - total) // 2
    canvas.alpha_composite(art, ((w - art.width) // 2, top))
    canvas.alpha_composite(word, ((w - word.width) // 2, top + art.height + gap))
    return canvas


def icon(src: Image.Image, n: int) -> Image.Image:
    """A square picture brought down to n x n: each target pixel takes the colour that covers
    most of its source block (no new colours), transparent where the block is mostly empty."""
    f = src.width // n
    out = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    for y in range(n):
        for x in range(n):
            counts: dict = {}
            for yy in range(f):
                for xx in range(f):
                    p = src.getpixel((x * f + xx, y * f + yy))
                    key = p if p[3] >= 128 else None
                    counts[key] = counts.get(key, 0) + 1
            best = max(counts, key=counts.get)
            if best is not None:
                out.putpixel((x, y), best)
    return out


def svg(im: Image.Image) -> bytes:
    """The pixel picture as one rect per horizontal run of a colour."""
    w, h = im.size
    px = im.load()
    rects = []
    for y in range(h):
        x = 0
        while x < w:
            p = px[x, y]
            if p[3] < 128:
                x += 1
                continue
            x0 = x
            while x < w and px[x, y] == p:
                x += 1
            rects.append(f'<rect x="{x0}" y="{y}" width="{x - x0}" height="1" fill="#{p[0]:02x}{p[1]:02x}{p[2]:02x}"/>')
    body = "\n".join(rects)
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w * 2}" height="{h * 2}" '
            f'shape-rendering="crispEdges">\n<title>OpenSNES</title>\n'
            f'<!-- Kit, the OpenSNES mascot, and the wordmark; CC0, devtools/art_assets/logo.py -->\n{body}\n</svg>\n').encode()


def png(im: Image.Image) -> bytes:
    buf = io.BytesIO()
    im.save(buf, format="PNG")
    return buf.getvalue()


def ico(ims: list[Image.Image]) -> bytes:
    buf = io.BytesIO()
    ims[-1].save(buf, format="ICO", sizes=[im.size for im in ims], append_images=ims[:-1])
    return buf.getvalue()


def build_logo(art_dir: Path) -> dict[str, bytes]:
    mascot = trimmed(art_dir / "logo" / "kit_mascot.png")
    head = trimmed(art_dir / "logo" / "kit_head.png")
    big = compose((256, 256), mascot, 4, 2)
    screen = compose((256, 224), mascot, 4, 2, NIGHT, gap=4)
    mid = compose((128, 128), head, 2, 1, gap=4)
    small = Image.open(art_dir / "logo" / "kit_icon.png").convert("RGBA")   # drawn at 32x32
    i32, i16 = small, icon16()
    full64 = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    full64.alpha_composite(head, ((64 - head.width) // 2, (64 - head.height) // 2))
    return {"logo_256x256.png": png(big), "logo_256x224.png": png(screen), "logo_128x128.png": png(mid),
            "logo_64x64.png": png(full64), "logo_32x32.png": png(i32), "logo_16x16.png": png(i16),
            "favicon.ico": ico([i16, i32]), "logo.svg": svg(big)}
