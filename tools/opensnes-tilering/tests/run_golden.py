#!/usr/bin/env python3
"""Golden tests of opensnes-tilering.

The tool has no ancestor to reproduce: its reference is the picture itself.
Besides the byte-for-byte goldens of a small map, every conversion here is
decoded back (characters, flips, palettes, the map in its 32 KB parts) and
compared with the PNG's pixels, and the numbering is checked against its
rule: first appearance, row by row from the bottom, left to right.
"""
from __future__ import annotations

import struct
import sys
import tempfile
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-tilering", __file__)
SMALL = ["small.png"]
OUTS = ["small.chr", "small.trm", "small.tpl", "small.trh", "small.pal", "small.inc", "small_data.as"]


# -- a minimal indexed-PNG reader and writer (filter 0 only, 8 bits) --------

def png_write(path: Path, w: int, h: int, pix: list[list[int]], ncolors: int = 128) -> None:
    def chunk(t: bytes, d: bytes) -> bytes:
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    plte = b"".join(bytes(((i * 8) % 256, (i * 29) % 256, (i * 71) % 256)) for i in range(ncolors))
    raw = b"".join(b"\x00" + bytes(row) for row in pix)
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 3, 0, 0, 0))
                     + chunk(b"PLTE", plte) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


def png_read(path: Path) -> list[list[int]]:
    data, pos, idat, w, h, depth = path.read_bytes(), 8, b"", 0, 0, 8
    while pos < len(data):
        n, t = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if t == b"IHDR":
            w, h, depth = struct.unpack(">IIB", body[:9])
        elif t == b"IDAT":
            idat += body
        pos += 12 + n
    raw, stride, rows, prev = zlib.decompress(idat), (w * depth + 7) // 8, [], None
    for y in range(h):
        f, line = raw[y * (stride + 1)], bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - 1] if i else 0
            b = prev[i] if prev else 0
            c = prev[i - 1] if prev and i else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pr = a if abs(p - a) <= abs(p - b) and abs(p - a) <= abs(p - c) else (b if abs(p - b) <= abs(p - c) else c)
                line[i] = (line[i] + pr) & 255
        prev = bytes(line)
        if depth == 8:
            rows.append(list(line))
        else:
            per = 8 // depth
            rows.append([(line[x // per] >> (8 - depth * (x % per + 1))) & ((1 << depth) - 1) for x in range(w)])
    return rows


def decode(work: Path, stem: str) -> list[str]:
    """Rebuild the picture from the outputs; compare with the PNG; check the order."""
    errs = []
    hdr = struct.unpack("<12H", (work / f"{stem}.trh").read_bytes())
    magic, _, width, height, nchars, rpp = hdr[:6]
    if magic != 0x5254:
        return ["header: no TR magic"]
    chr_ = (work / f"{stem}.chr").read_bytes()
    tpl = (work / f"{stem}.tpl").read_bytes()
    trm = (work / f"{stem}.trm").read_bytes()
    if len(chr_) != 32 * nchars or len(tpl) != nchars:
        errs.append(f"{nchars} characters, but .chr {len(chr_)} bytes and .tpl {len(tpl)}")
    parts = (height + rpp - 1) // rpp
    if len(trm) != (parts - 1) * 32768 + 2 * width * (height - (parts - 1) * rpp):
        errs.append(f".trm is {len(trm)} bytes for {parts} parts of {rpp} rows")
    pix = png_read(work / f"{stem}.png")
    seen, order_ok = set(), True
    expect_next = 0
    for cy in reversed(range(height)):
        for cx in range(width):
            at = (cy // rpp) * 32768 + (cy % rpp) * 2 * width + 2 * cx
            e = trm[at] | trm[at + 1] << 8
            c, hf, vf = e & 0x0FFF, e >> 14 & 1, e >> 15
            if c not in seen:
                if c != expect_next:
                    order_ok = False
                seen.add(c)
                expect_next += 1
            b = chr_[32 * c:32 * c + 32]
            pal = tpl[c] >> 2
            for y in range(8):
                for x in range(8):
                    sx, sy = (7 - x if hf else x), (7 - y if vf else y)
                    bit = 7 - sx
                    v = (b[sy * 2] >> bit & 1) | (b[sy * 2 + 1] >> bit & 1) << 1 \
                        | (b[16 + sy * 2] >> bit & 1) << 2 | (b[17 + sy * 2] >> bit & 1) << 3
                    want = pix[cy * 8 + y][cx * 8 + x]
                    got = pal * 16 + v if v else 0
                    if (want if want & 15 else 0) != got and len(errs) < 4:
                        errs.append(f"cell ({cx}, {cy}) pixel ({x}, {y}): {got} decoded, {want} in the PNG")
    if not order_ok:
        errs.append("characters are not numbered by first appearance from the bottom row")
    if len(seen) != nchars:
        errs.append(f"the map uses {len(seen)} characters, the header says {nchars}")
    return errs


def roundtrip(argv: list[str], stem: str, write: dict | None = None, copy=()) -> list[str]:
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(argv, work, copy=copy, write=write)
        err = g.failure(proc)
        return [err] if err else decode(work, stem)


g.expect_outputs("convert --window 4x3: characters, map, palettes, header, glue",
                 ["convert", "-q", "--window", "4x3", "small.png"], copy=SMALL, outputs=OUTS)
g.check("convert: decoded back, the picture; numbered from the bottom", "round trip",
        lambda: roundtrip(["convert", "-q", "--window", "4x3", "small.png"], "small", copy=SMALL))
g.check("convert --no-mirrors: decoded back, the picture", "round trip",
        lambda: roundtrip(["convert", "-q", "--window", "4x3", "--no-mirrors", "small.png"], "small", copy=SMALL))
g.expect_stdout("inspect", ["inspect", "--window", "4x3", "small.png"], golden="inspect.txt", copy=SMALL)
g.expect_stdout("inspect --json", ["inspect", "--json", "--window", "4x3", "small.png"], golden="inspect.json", copy=SMALL)


# A map of more than 1,024 characters and more than 32 KB of map: two
# character parts, two map parts (128 rows of 256 bytes a part).
def big_map() -> bytes:
    w, h = 1024, 1040
    pix = [[0] * w for _ in range(h)]
    for cy in range(h // 8):
        for cx in range(w // 8):
            n = (cy * 128 + cx) % 1500 if (cx + cy) % 3 else 1500 + (cx % 5)
            for y in range(8):
                for x in range(8):
                    # n's bits along the rows, a corner no mirror keeps in place
                    if (x, y) == (0, 0):
                        v = 15
                    elif (x, y) == (7, 7):
                        v = 0
                    else:
                        v = 1 + 2 * ((n >> (x + 8 * (y % 2))) & 1) + 4 * (y // 2 % 2)
                    pix[cy * 8 + y][cx * 8 + x] = (16 * (n % 3) + v) if v else 0
    with tempfile.TemporaryDirectory() as td:
        png_write(Path(td) / "big.png", w, h, pix)
        return (Path(td) / "big.png").read_bytes()


BIG = big_map()


def big_case() -> list[str]:
    errs = roundtrip(["convert", "-q", "big.png"], "big", write={"big.png": BIG})
    with tempfile.TemporaryDirectory() as td:
        said = g.run(["inspect", "big.png"], Path(td), write={"big.png": BIG}).stdout
        if "1505 distinct" not in said:
            errs.append(f"inspect: {said.strip()}")
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(["convert", "-q", "big.png"], work, write={"big.png": BIG})
        if proc.returncode == 0:
            inc = (work / "big.inc").read_text()
            for want in ("big_char_parts[2] = { big_chars, big_chars_1 }",
                         "big_map_parts[2] = { big_map, big_map_1 }"):
                if want not in inc:
                    errs.append(f"big.inc lacks {want!r}")
    return errs


g.check("convert: 1,500 characters and 33 KB of map, in parts, decoded back", "round trip, parts named", big_case)

g.expect_refused("refused: a cell of two palettes", ["inspect", "--window", "4x3", "mixed.png"],
                 copy=["mixed.png"], needles=["mixes palettes 0 and 1"])
g.expect_refused("refused: not indexed", ["inspect", "--window", "4x3", "rgb.png"],
                 copy=["rgb.png"], needles=["not an indexed PNG"])
g.expect_refused("refused: 60 pixels wide", ["inspect", "--window", "4x3", "odd.png"],
                 copy=["odd.png"], needles=["multiples of 8"])
g.expect_refused("refused: smaller than the window", ["convert", "small.png"],
                 copy=SMALL, needles=["smaller than the window"], nothing_written=["small.chr", "small.trm"])
g.expect_refused("refused: a window past 64x32", ["inspect", "--window", "65x20", "small.png"],
                 copy=SMALL, needles=["at most 64x32"])

sys.exit(g.report())
