#!/usr/bin/env python3
"""Golden tests for opensnes-tileset (background pictures; the 1.x successor of gfx4snes -m).

The family's contract: reproduce the absorbed tool's golden suite byte for
byte. The bg case compares against gfx4snes's goldens; the two banks
cases run gfx4snes's pixel oracle (decode .pic/.pal/.map and compare every
pixel's colour with the source, with and without --rearrange — the two
silent defects of 2026-10-05); the refusals name their limit. Fixtures are
copies of gfx4snes's.

Run:  python3 tools/opensnes-tileset/tests/run_golden.py
"""
from __future__ import annotations

import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-tileset", __file__)
GFX = g.here.parents[1] / "lib" / "gfx4snes" / "tests" / "golden"

g.expect_outputs("convert bg.png --colors 16 == gfx4snes -s 8 -o 16 -u 16 -p -m (the data)", ["convert", "-q", "--colors", "16", "bg.png"],
                 copy=["bg.png"], outputs=["bg.pic", "bg.pal", "bg.map"], want_dir=GFX)
g.expect_outputs("convert bg.png: the glue in asset.h's naming (.inc with DECLARE_BG_ASSET, _data.as)", ["convert", "-q", "--colors", "16", "bg.png"],
                 copy=["bg.png"], outputs=["bg.inc", "bg_data.as"])



def column_major():
    """--column-major: <stem>.cmap holds the entries of <stem>.map column after
    column — entry (c, r) at byte (c * height + r) * 2 — on a 16x8 map, so that
    a width taken for a height shows; --offset is in both files."""
    def run():
        W, H = 16, 8
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            proc = g.run(["convert", "-q", "--colors", "16", "--offset", "3", "--column-major", "wide.png"], work, copy=["wide.png"])
            if g.failure(proc):
                return [g.failure(proc)]
            rows = (work / "wide.map").read_bytes()
            cols = (work / "wide.cmap").read_bytes()
            inc = (work / "wide.inc").read_text()
            asm = (work / "wide_data.as").read_text()
        errs = []
        if len(cols) != W * H * 2:
            errs.append(f"wide.cmap is {len(cols)} bytes, {W * H * 2} expected")
        at = lambda blob, k: blob[k * 2:k * 2 + 2]            # noqa: E731
        bad = [(c, r) for c in range(W) for r in range(H) if at(cols, c * H + r) != at(rows, r * W + c)]
        if bad:
            errs.append(f"{len(bad)} entries differ from the .map, first at column {bad[0][0]} row {bad[0][1]}")
        if len({at(rows, k) for k in range(W * H)}) < 20:
            errs.append("the fixture's map is too uniform to prove an order")
        if min(struct.unpack_from("<H", rows, k * 2)[0] & 0x3FF for k in range(W * H)) < 3:
            errs.append("--offset 3 is not in the entries")
        for needle, where, text in (("extern const u8 wide_cols[], wide_cols_end[];", "wide.inc", inc),
                                    ("(c * 8 + r) * 2", "wide.inc", inc),
                                    ('ASSET_SECTION "wide_cols"', "wide_data.as", asm),
                                    ('.incbin "wide.cmap"', "wide_data.as", asm)):
            if needle not in text:
                errs.append(f"{where} lacks {needle!r}")
        return errs
    return run


g.check("convert wide.png --column-major: the .cmap is the .map by columns, declared as wide_cols", "transposition checked independently", column_major())

g.expect_refused("convert --column-major --mode 7 (one byte per entry)", ["convert", "--mode", "7", "--bpp", "8", "--column-major", "bg.png"],
                 copy=["bg.png"], needles=["--column-major with Mode 7"])
g.expect_refused("convert --column-major --pages (not row after row)", ["convert", "--colors", "16", "--pages", "--column-major", "bg.png"],
                 copy=["bg.png"], needles=["--column-major with --pages"])

def _read_pixels(path: Path):
    pal, rows = [], []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("palette "):
            pal = [tuple(int(h[i:i + 2], 16) for i in (0, 2, 4)) for h in line.split()[1:]]
        elif line.startswith("row "):
            rows.append([int(v) for v in line.split()[1:]])
    return pal, rows


def roundtrip(argv):
    """Every pixel of the converted picture decodes to the source's colour (gfx4snes's oracle)."""
    def run():
        pal, rows = _read_pixels(g.fixtures / "banks.pixels")
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            proc = g.run(argv, work, copy=["banks.png"])
            if g.failure(proc):
                return [g.failure(proc)]
            pic = (work / "banks.pic").read_bytes()
            palb = (work / "banks.pal").read_bytes()
            cols = [struct.unpack_from("<H", palb, i * 2)[0] for i in range(len(palb) // 2)]
            mp = (work / "banks.map").read_bytes()
            entries = [struct.unpack_from("<H", mp, i * 2)[0] for i in range(len(mp) // 2)]

        def rgb(c):
            return ((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3)

        def tile(n):
            t = pic[n * 32:(n + 1) * 32]
            out = [[0] * 8 for _ in range(8)]
            for y in range(8):
                b0, b1, b2, b3 = t[y * 2], t[y * 2 + 1], t[16 + y * 2], t[16 + y * 2 + 1]
                for x in range(8):
                    bit = 7 - x
                    out[y][x] = (((b0 >> bit) & 1) | (((b1 >> bit) & 1) << 1)
                                 | (((b2 >> bit) & 1) << 2) | (((b3 >> bit) & 1) << 3))
            return out

        width = len(rows[0]) // 8
        bad = 0
        for tx in range(width):
            e = entries[tx]
            tn, pb, hf, vf = e & 0x3FF, (e >> 10) & 7, (e >> 14) & 1, (e >> 15) & 1
            t = tile(tn)
            for y in range(8):
                for x in range(8):
                    idx = t[7 - y if vf else y][7 - x if hf else x]
                    src = rows[y][tx * 8 + x]
                    if idx == 0 and src % 16 == 0:
                        continue
                    want = tuple((v >> 3) << 3 for v in pal[src])
                    if rgb(cols[pb * 16 + idx]) != want:
                        bad += 1
        return [f"{bad} pixel(s) decode to a colour that is not the source's"] if bad else []
    return run


g.check("convert banks.png --colors 48: every pixel round-trips", "pixel oracle", roundtrip(["convert", "-q", "--colors", "48", "banks.png"]))
g.check("convert banks.png --colors 48 --rearrange: every pixel round-trips", "pixel oracle after the rearrangement",
        roundtrip(["convert", "-q", "--colors", "48", "--rearrange", "banks.png"]))
def _indexed_png(rows, ncol=16, palette=None):
    """An indexed PNG from rows of palette indices, written here so the test needs no imaging module."""
    import zlib

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    if palette is None:
        palette = [((i * 53) % 256, (i * 97) % 256, (i * 29) % 256) for i in range(ncol)]
    plte = b"".join(bytes(c) for c in palette)
    raw = b"".join(b"\0" + bytes(r) for r in rows)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", len(rows[0]), len(rows), 8, 3, 0, 0, 0))
            + chunk(b"PLTE", plte) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def split_load():
    """A tileset larger than one 32 KB bank is written in parts; the .inc
    declares each and gives <stem>_tiles_load(vram), one dmaCopyVram per part,
    part n at vram + n * 0x4000 words. 576 distinct 8bpp tiles: 36 864 bytes,
    two parts."""
    def run():
        T = 24
        rows = [[0] * (T * 8) for _ in range(T * 8)]
        for ty in range(T):
            for tx in range(T):
                k = ty * T + tx
                for y in range(8):
                    for x in range(8):
                        rows[ty * 8 + y][tx * 8 + x] = (x * 3 + y * 11 + 1) & 0xFF
                rows[ty * 8][tx * 8] = k & 0xFF
                rows[ty * 8][tx * 8 + 1] = k >> 8
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            (work / "big.png").write_bytes(_indexed_png(rows, ncol=256))
            proc = g.run(["convert", "-q", "--bpp", "8", "--colors", "256", "big.png"], work)
            if g.failure(proc):
                return [g.failure(proc)]
            inc = (work / "big.inc").read_text()
            asm = (work / "big_data.as").read_text()
        errs = []
        for needle, where, text in (("extern const u8 big_tiles_1[], big_tiles_1_end[];", "big.inc", inc),
                                    ("#define big_tiles_load(vram) do {", "big.inc", inc),
                                    ("dmaCopyVram(big_tiles, (u16)((vram) + 0x0000), (u16)(big_tiles_end - big_tiles));", "big.inc", inc),
                                    ("dmaCopyVram(big_tiles_1, (u16)((vram) + 0x4000), (u16)(big_tiles_1_end - big_tiles_1));", "big.inc", inc),
                                    ('ASSET_SECTION "big_tiles_1"', "big_data.as", asm)):
            if needle not in text:
                errs.append(f"{where} lacks {needle!r}")
        if "big_tiles_2" in inc:
            errs.append("big.inc declares a third part for 36 864 bytes")
        return errs
    return run


g.check("convert a 36 KB tileset: two parts, and big_tiles_load() loads both", "the .inc read for each part", split_load())


def metatiles_16():
    """--metatiles 16: a 64x48 picture of 4x3 blocks drawn from four 16x16
    patterns (one used three times, one the mirror of another). The .mtm names
    one metatile per block, identical blocks share one, and every pixel decodes
    through .mtm -> .mtl -> .pic -> .pal to the source's colour."""
    def run():
        def pattern(p, x, y):
            return [(x * 5 + y * 3) % 15 + 1, (x ^ y) % 7 + 8, (x // 4 + y // 4) % 2 * 9 + 2, 0][p] if p < 3 \
                else ((15 - x) * 5 + y * 3) % 15 + 1          # 3: pattern 0 mirrored
        layout = [[0, 1, 0, 2], [3, 0, 1, 1], [2, 2, 0, 3]]
        W, H = 4, 3
        rows = [[pattern(layout[y // 16][x // 16], x % 16, y % 16) for x in range(W * 16)] for y in range(H * 16)]
        palette = [((i * 53) % 256, (i * 97) % 256, (i * 29) % 256) for i in range(16)]
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            (work / "lvl.png").write_bytes(_indexed_png(rows, ncol=16, palette=palette))
            proc = g.run(["convert", "-q", "--colors", "16", "--flip", "--metatiles", "16", "lvl.png"], work)
            if g.failure(proc):
                return [g.failure(proc)]
            pic = (work / "lvl.pic").read_bytes()
            palb = (work / "lvl.pal").read_bytes()
            mtl = (work / "lvl.mtl").read_bytes()
            mtm = (work / "lvl.mtm").read_bytes()
            inc = (work / "lvl.inc").read_text()
            asm = (work / "lvl_data.as").read_text()
            has_map = (work / "lvl.map").exists()
        errs = []
        cols = [struct.unpack_from("<H", palb, i * 2)[0] for i in range(len(palb) // 2)]
        meta = [struct.unpack_from("<4H", mtl, i * 8) for i in range(len(mtl) // 8)]
        mm = [struct.unpack_from("<H", mtm, i * 2)[0] for i in range(len(mtm) // 2)]
        if len(mm) != W * H:
            errs.append(f"lvl.mtm holds {len(mm)} entries, {W * H} expected (no padding)")
            return errs
        if len(meta) != 4:
            errs.append(f"{len(meta)} metatiles for 4 distinct blocks")
        for by in range(H):
            for bx in range(W):
                for oy in range(H):
                    for ox in range(W):
                        same = layout[by][bx] == layout[oy][ox]
                        if same != (mm[by * W + bx] == mm[oy * W + ox]):
                            errs.append(f"blocks ({bx},{by}) and ({ox},{oy}) {'differ' if same else 'share a metatile'} in the .mtm")
        if has_map:
            errs.append("lvl.map written: the metatile map replaces it")

        def rgb(c):
            return ((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3)

        def tile(n):
            t = pic[n * 32:(n + 1) * 32]
            return [[(((t[y * 2] >> (7 - x)) & 1) | (((t[y * 2 + 1] >> (7 - x)) & 1) << 1)
                      | (((t[16 + y * 2] >> (7 - x)) & 1) << 2) | (((t[16 + y * 2 + 1] >> (7 - x)) & 1) << 3))
                     for x in range(8)] for y in range(8)]
        bad = 0
        for y in range(H * 16):
            for x in range(W * 16):
                q = meta[mm[(y // 16) * W + x // 16]] if mm[(y // 16) * W + x // 16] < len(meta) else (0, 0, 0, 0)
                e = q[((y % 16) // 8) * 2 + (x % 16) // 8]
                tn, pb, hf, vf = e & 0x3FF, (e >> 10) & 7, (e >> 14) & 1, (e >> 15) & 1
                ty, tx = y % 8, x % 8
                idx = tile(tn)[7 - ty if vf else ty][7 - tx if hf else tx]
                src = rows[y][x]
                if idx == 0 and src == 0:
                    continue
                if rgb(cols[pb * 16 + idx]) != tuple((v >> 3) << 3 for v in palette[src]):
                    bad += 1
        if bad:
            errs.append(f"{bad} pixel(s) decode through the metatiles to a colour that is not the source's")
        for needle in ("#define LVL_METATILES 4", "#define LVL_METAMAP_W 4", "#define LVL_METAMAP_H 3",
                       "extern const u8 lvl_metatiles[], lvl_metatiles_end[];", "extern const u8 lvl_metamap[], lvl_metamap_end[];"):
            if needle not in inc:
                errs.append(f"lvl.inc lacks {needle!r}")
        for needle in ('ASSET_SECTION "lvl_metatiles"', 'ASSET_SECTION "lvl_metamap"'):
            if needle not in asm:
                errs.append(f"lvl_data.as lacks {needle!r}")
        if "lvl_map" in inc or "lvl_map" in asm:
            errs.append("lvl_map declared without a .map")
        return errs
    return run


g.check("convert --metatiles 16: one metatile per distinct 16x16 block, every pixel decodes through them", "pixel oracle through .mtm/.mtl", metatiles_16())
g.expect_refused("convert --metatiles 8", ["convert", "--metatiles", "8", "bg.png"], copy=["bg.png"], needles=["--metatiles 8: 16"])
g.expect_refused("convert --metatiles 16 --mode 7", ["convert", "--metatiles", "16", "--mode", "7", "--bpp", "8", "bg.png"], copy=["bg.png"], needles=["a metatile is four entries"])
g.expect_refused("convert --metatiles 16 --pages", ["convert", "--metatiles", "16", "--pages", "bg.png"], copy=["bg.png"], needles=["the metatile map replaces the map"])


def vram_key():
    """`vram = N` before the table is not the tool's: accepted, kept by
    --save, and a value outside 0..65536 is refused."""
    def run():
        errs = []
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            (work / "bg.png.toml").write_text('tool = "opensnes-tileset"\nvram = 4096\n\n[convert]\ncolors = 16\n')
            proc = g.run(["convert", "-q", "--save", "bg.png"], work, copy=["bg.png"])
            if g.failure(proc):
                return [g.failure(proc)]
            if "vram = 4096" not in (work / "bg.png.toml").read_text():
                errs.append("--save dropped `vram = 4096`")
            (work / "bg.png.toml").write_text('tool = "opensnes-tileset"\nvram = 70000\n\n[convert]\n')
            proc = g.run(["convert", "-q", "bg.png"], work)
            if proc.returncode == 0 or "0 to 65536" not in proc.stderr:
                errs.append("vram = 70000 not refused")
        return errs
    return run


g.check("a settings file with vram = N (the scene count's, not the tool's)", "accepted, kept, bounded", vram_key())

def blank_tile(first_blank, extra, mode5=False):
    """--blank: every cell of the map shows its own block, whether the picture
    starts with a blank block (it is tile 0, nothing is added) or not (a blank
    tile is added and the map counts from 1). Until 2026-10-13 the blank tile
    was written in both cases and the map shifted in one only: a picture that
    started with a blank block came out one tile off in every cell. In Mode 5 a
    cell is 16 pixels wide, two tiles side by side, and the blank is two tiles
    (one was written, and every cell but the first was one tile off)."""
    def run():
        W, H, BW = 4, 2, (16 if mode5 else 8)
        rows = [[0] * (W * BW) for _ in range(H * 8)]
        for k in range(1 if first_blank else 0, W * H):
            r, c = divmod(k, W)
            for y in range(8):
                for x in range(BW):
                    rows[r * 8 + y][c * BW + x] = 1 + (k + x * 3 + y * 5) % 15   # no symmetry: --flip folds nothing
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            (work / "b.png").write_bytes(_indexed_png(rows))
            proc = g.run(["convert", "-q", "--colors", "16", "--blank", *extra, "b.png"], work)
            if g.failure(proc):
                return [g.failure(proc)]
            pic = (work / "b.pic").read_bytes()
            mp = (work / "b.map").read_bytes()
        entries = [struct.unpack_from("<H", mp, i * 2)[0] for i in range(W * H)]
        errs = []
        want_tiles = (W * H if first_blank else W * H + 1) * (BW // 8)
        if len(pic) != want_tiles * 32:
            errs.append(f"b.pic holds {len(pic) // 32} tiles, {want_tiles} expected")
        if any(pic[:32]):
            errs.append("tile 0 is not blank")
        bad = []
        for k, e in enumerate(entries):
            r, c = divmod(k, W)
            for y in range(8):
                for x in range(BW):
                    n = (e & 0x3FF) + x // 8
                    t = pic[n * 32:n * 32 + 32]
                    bit = 7 - x % 8
                    idx = (((t[y * 2] >> bit) & 1) | (((t[y * 2 + 1] >> bit) & 1) << 1)
                           | (((t[16 + y * 2] >> bit) & 1) << 2) | (((t[17 + y * 2] >> bit) & 1) << 3)) if len(t) == 32 else -1
                    if idx != rows[r * 8 + y][c * BW + x]:
                        bad.append(k)
                        break
                else:
                    continue
                break
        if bad:
            errs.append(f"cells {bad} do not show their own block (map tiles {[e & 0x3FF for e in entries]})")
        return errs
    return run


for _first, _name in ((True, "a picture that starts with a blank block"), (False, "a picture with no blank block")):
    for _extra in ([], ["--no-reduce"], ["--flip"]):
        g.check(" ".join(["convert --blank", *_extra]) + ": " + _name, "every cell decoded against the source",
                blank_tile(_first, _extra))
    g.check("convert --blank --mode 5: " + _name, "every 16-pixel cell decoded against the source",
            blank_tile(_first, ["--mode", "5"], mode5=True))
def opaque_like_zero(shade, extra):
    """Index 0 is transparent by its place, not by its colour: an opaque
    square of index 5 whose colour is index 0's (black, or (2, 2, 2), black on
    five bits) keeps a non-zero index. --rearrange used to fold every index
    onto the first of the same colour, index 0 among them, and the square came
    out transparent (2026-10-15)."""
    def run():
        rows = [[0] * 16 for _ in range(8)]
        for y in range(2, 6):
            for x in range(2, 6):
                rows[y][x] = 5
            for x in range(10, 14):
                rows[y][x] = 7
        pal = [(0, 0, 0)] * 16
        pal[5] = shade
        pal[7] = (200, 100, 50)
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            (work / "o.png").write_bytes(_indexed_png(rows, palette=pal))
            proc = g.run(["convert", "-q", "--colors", "16", *extra, "o.png"], work)
            if g.failure(proc):
                return [g.failure(proc)]
            pic = (work / "o.pic").read_bytes()
            mp = (work / "o.map").read_bytes()
        bad = 0
        for tx in range(2):
            n = struct.unpack_from("<H", mp, tx * 2)[0] & 0x3FF
            t = pic[n * 32:n * 32 + 32]
            for y in range(8):
                for x in range(8):
                    bit = 7 - x
                    idx = (((t[y * 2] >> bit) & 1) | (((t[y * 2 + 1] >> bit) & 1) << 1)
                           | (((t[16 + y * 2] >> bit) & 1) << 2) | (((t[17 + y * 2] >> bit) & 1) << 3))
                    if (idx == 0) != (rows[y][tx * 8 + x] == 0):
                        bad += 1
        return [f"{bad} pixel(s) changed between transparent and opaque"] if bad else []
    return run


for _shade in ((0, 0, 0), (2, 2, 2)):
    for _extra in ([], ["--rearrange"]):
        g.check(" ".join(["convert", *_extra]) + f": an opaque index of colour {_shade}, index 0's colour",
                "stays opaque", opaque_like_zero(_shade, _extra))
g.expect_stdout("convert --json", ["convert", "--json", "--colors", "16", "bg.png"], golden="convert.json", copy=["bg.png"])
g.expect_stdout("inspect --json", ["inspect", "--json", "bg.png", "banks.png"], golden="inspect.json", copy=["bg.png", "banks.png"])
g.expect_outputs("convert --save writes bg.png.toml", ["convert", "-q", "--colors", "16", "--flip", "--save", "bg.png"],
                 copy=["bg.png"], outputs=["bg.png.toml"])
g.expect_refused("convert toomany_bg.png (1056 distinct tiles)", ["convert", "--no-palette", "toomany_bg.png"],
                 copy=["toomany_bg.png"], needles=["1024 at most"])
g.expect_refused("convert toomany_m7.png --mode 7 (tile 256 in a byte)", ["convert", "--bpp", "8", "--mode", "7", "--no-palette", "toomany_m7.png"],
                 copy=["toomany_m7.png"], needles=["exceeds 255"])
g.expect_refused("convert mixed_map.png (two palette banks in a tile)", ["convert", "--colors", "32", "mixed_map.png"],
                 copy=["mixed_map.png"], needles=["palette banks"])
g.expect_refused("convert --size 12", ["convert", "--size", "12", "bg.png"], copy=["bg.png"], needles=["8 or 16"], nothing_written=["bg.pic"])
g.expect_refused("convert --mode 2 (the map format is 1, 5, 6 or 7)", ["convert", "--mode", "2", "bg.png"], copy=["bg.png"], needles=["1 (Modes 0 to 4), 5, 6 or 7"])
sys.exit(g.report())
