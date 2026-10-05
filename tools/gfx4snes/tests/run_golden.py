#!/usr/bin/env python3
"""Golden-output tests for gfx4snes.

Each case copies a fixture PNG into a temp dir, runs gfx4snes with the
canonical flags used by the example Makefiles, and byte-compares every
produced file against the committed golden. gfx4snes output is
deterministic (no timestamps in generated files), so any diff is a real
behaviour change: either a regression, or an intentional change that
must be re-goldened (re-run the case and copy the outputs over golden/,
reviewing the diff).

Fixture provenance (both PVSnesLib-derived assets already in the repo,
see ATTRIBUTION.md):
  fixtures/bg.png  = examples/backgrounds/mode1/res/opensnes.png
  fixtures/spr.png = examples/games/likemario/res/mario_sprite.png

Run:  python3 tools/gfx4snes/tests/run_golden.py
"""
from __future__ import annotations

import filecmp
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TOOL = REPO / "bin" / "gfx4snes"

# (fixture, flags, expected outputs — all byte-compared against golden/)
CASES = [
    ("bg.png", ["-s", "8", "-o", "16", "-u", "16", "-p", "-m"],
     ["bg.pic", "bg.pal", "bg.map", "bg.inc", "bg_data.as"]),
    ("spr.png", ["-s", "16", "-p"],
     ["spr.pic", "spr.pal", "spr.inc", "spr_data.as"]),
    # Metasprite path (-T) with flip-aware dedup (-F): flip.png holds one
    # asymmetric 16x16 tile and its exact H-mirror, so the golden pins the
    # OBJ_FLIPX emission and the canonical-tile reference (issue #97).
    ("flip.png", ["-s", "16", "-o", "16", "-u", "16", "-p", "-T",
                  "-X", "32", "-Y", "16", "-P", "2", "-F"],
     ["flip.pic", "flip.pal", "flip.inc", "flip_data.as", "flip_meta.inc"]),
    # Metasprite tile field is an 8x8 CHARACTER NAME, not a block index
    # (issue #100). two.png = two distinct 16x16 blocks -> the second block's
    # tiles live at char name 2 (2 columns of 8x8), not index 1. Pins the
    # positional conversion in metasprites.c.
    ("two.png", ["-s", "16", "-o", "16", "-u", "16", "-p", "-T",
                 "-X", "32", "-Y", "16", "-P", "2"],
     ["two_meta.inc"]),
    # three.png = [A, H-mirror of A, distinct B]. The mirror makes the
    # compacted tile counter lag the reading position, so a naive index would
    # emit B at 2 instead of its true char name 4. Pins the position-based
    # canonical reference in maps.c (issue #100 multi-mirror drift).
    ("three.png", ["-s", "16", "-o", "16", "-u", "16", "-p", "-T",
                   "-X", "48", "-Y", "16", "-P", "2", "-F"],
     ["three_meta.inc"]),
]


# Inputs the tool must REFUSE (exit != 0, stderr naming the limit). Each one
# was converted silently before 2026-10-04: the BG tile field is 10 bits, the
# Mode 7 map one byte per tile, and a 2bpp/4bpp tile takes its palette bank
# from its first pixel (build-tools audit S7, S8, S12). The fixtures are
# generated: toomany_bg.png holds 1056 distinct 8x8 tiles, toomany_m7.png
# 272, index5_2bpp.png a bank-0 tile with one pixel of colour 5 (bank 1).
# Pixel oracle (2026-10-05, build-tools audit C): decode .pic/.pal/.map and
# compare every pixel's colour with the source. Two silent defects measured
# before the fix on banks.png (four tiles over three palette banks, one
# bank-2 tile starting with the transparent index): the map entry took the
# palette bank of the tile's FIRST pixel (63 of 64 pixels of that tile shown
# in bank 0's colours), and `-a` rearranged the .pal on the row-major image
# while the tiles kept their old indices (191 of 256 pixels wrong).
# (fixture, flags, pixel file)
ROUNDTRIP = [
    ("banks.png", ["-s", "8", "-o", "48", "-u", "16", "-p", "-m"], "banks.pixels"),
    ("banks.png", ["-s", "8", "-o", "48", "-u", "16", "-p", "-m", "-a"], "banks.pixels"),
]


def _read_pixels(path: Path):
    pal, rows = [], []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("palette "):
            pal = [tuple(int(h[i:i + 2], 16) for i in (0, 2, 4)) for h in line.split()[1:]]
        elif line.startswith("row "):
            rows.append([int(v) for v in line.split()[1:]])
    return pal, rows


def run_roundtrip(fixture: str, flags: list[str], pixels: str) -> list[str]:
    import struct
    pal, rows = _read_pixels(HERE / "fixtures" / pixels)
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        shutil.copy(HERE / "fixtures" / fixture, work / fixture)
        proc = subprocess.run([str(TOOL), *flags, "-i", fixture], cwd=work,
                              capture_output=True, text=True, timeout=60)
        if proc.returncode != 0:
            return [f"exit {proc.returncode}: {(proc.stderr or proc.stdout).strip()[:200]}"]
        base = fixture.rsplit(".", 1)[0]
        pic = (work / f"{base}.pic").read_bytes()
        palb = (work / f"{base}.pal").read_bytes()
        cols = [struct.unpack_from("<H", palb, i * 2)[0] for i in range(len(palb) // 2)]
        mp = (work / f"{base}.map").read_bytes()
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
                    continue                       # transparent in both
                want = tuple((v >> 3) << 3 for v in pal[src])
                if rgb(cols[pb * 16 + idx]) != want:
                    bad += 1
    return [f"{bad} pixel(s) decode to a colour that is not the source's"] if bad else []


REFUSED = [
    ("toomany_bg.png", ["-s", "8", "-u", "16", "-m"], "1024 at most"),
    ("toomany_m7.png", ["-s", "8", "-u", "256", "-m", "-M", "7"], "exceeds 255"),
    ("index5_2bpp.png", ["-s", "8", "-u", "4"], "one 4-colour palette per tile"),
    # a map tile whose opaque pixels sit in two palette banks: one entry, one palette (2026-10-05)
    ("mixed_map.png", ["-s", "8", "-o", "32", "-u", "16", "-p", "-m"], "palette banks"),
]


def run_refused(fixture: str, flags: list[str], needle: str) -> list[str]:
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        shutil.copy(HERE / "fixtures" / fixture, work / fixture)
        proc = subprocess.run([str(TOOL), *flags, "-i", fixture],
                              cwd=work, capture_output=True, text=True, timeout=60)
        if proc.returncode == 0:
            return ["accepted (exit 0) — must be refused"]
        text = (proc.stderr or "") + (proc.stdout or "")
        if needle not in text:
            return [f"refused, but without '{needle}': {text.strip()[:160]}"]
    return []


def run_case(fixture: str, flags: list[str], outputs: list[str]) -> list[str]:
    errs = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        shutil.copy(HERE / "fixtures" / fixture, work / fixture)
        proc = subprocess.run([str(TOOL), *flags, "-i", fixture],
                              cwd=work, capture_output=True, text=True, timeout=60)
        if proc.returncode != 0:
            return [f"exit {proc.returncode}: {(proc.stderr or proc.stdout).strip()[:200]}"]
        for out in outputs:
            got = work / out
            want = HERE / "golden" / out
            if not got.is_file():
                errs.append(f"{out}: not produced")
            elif not filecmp.cmp(got, want, shallow=False):
                errs.append(f"{out}: differs from golden ({got.stat().st_size} vs "
                            f"{want.stat().st_size} bytes)")
    return errs


def main() -> int:
    if not TOOL.is_file():
        sys.exit(f"ERROR: {TOOL} not found — run `make tools` first")
    fails = 0
    for fixture, flags, outputs in CASES:
        errs = run_case(fixture, flags, outputs)
        name = f"{fixture} [{' '.join(flags)}]"
        if errs:
            print(f"  FAIL {name}: " + "; ".join(errs))
            fails += 1
        else:
            print(f"  PASS {name} ({len(outputs)} outputs match)")
    for fixture, flags, needle in REFUSED:
        errs = run_refused(fixture, flags, needle)
        name = f"{fixture} [{' '.join(flags)}] refused"
        if errs:
            print(f"  FAIL {name}: " + "; ".join(errs))
            fails += 1
        else:
            print(f"  PASS {name}")
    for fixture, flags, pixels in ROUNDTRIP:
        errs = run_roundtrip(fixture, flags, pixels)
        name = f"roundtrip {fixture} [{' '.join(flags)}]"
        if errs:
            print(f"  FAIL {name}: " + "; ".join(errs))
            fails += 1
        else:
            print(f"  PASS {name} (every pixel decodes to its source colour)")
    total = len(CASES) + len(REFUSED) + len(ROUNDTRIP)
    print(f"\ngfx4snes golden: {total - fails}/{total} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
