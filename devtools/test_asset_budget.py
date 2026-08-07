#!/usr/bin/env python3
"""Tests for devtools/asset_budget.py — the static PPU asset-budget report.

Run: python3 devtools/test_asset_budget.py
"""
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from asset_budget import Budget, scan_example, VRAM_LIMIT, CGRAM_LIMIT


def _write(d: Path, name: str, size: int) -> None:
    (d / name).write_bytes(b"\x00" * size)


def test_categorisation_and_totals():
    """tiles+maps -> VRAM bytes; palette bytes -> colours (2 bytes each)."""
    with tempfile.TemporaryDirectory() as td:
        d = Path(td)
        _write(d, "gfx.pic", 2592)   # tiles
        _write(d, "gfx.map", 2048)   # map
        _write(d, "gfx.pal", 32)     # palette -> 16 colours
        b = scan_example(d)
        assert b.tiles == 2592, b.tiles
        assert b.maps == 2048, b.maps
        assert b.vram == 2592 + 2048, b.vram
        assert b.colours == 16, b.colours
        assert b.files == 3, b.files


def test_mode7_and_nested_res_dir():
    """.pc7/.mp7 count as tiles/maps; rglob reaches a res/ subdir."""
    with tempfile.TemporaryDirectory() as td:
        d = Path(td)
        res = d / "res"
        res.mkdir()
        _write(res, "world.pc7", 4096)   # mode7 tiles
        _write(res, "world.mp7", 16384)  # mode7 map
        _write(res, "world.pal", 512)    # 256 colours
        b = scan_example(d)
        assert b.tiles == 4096, b.tiles
        assert b.maps == 16384, b.maps
        assert b.vram == 4096 + 16384, b.vram
        assert b.colours == 256, b.colours


def test_ignores_unrelated_files():
    """PNG sources, .c, .o etc. must not count toward the budget."""
    with tempfile.TemporaryDirectory() as td:
        d = Path(td)
        _write(d, "gfx.png", 5000)
        _write(d, "main.c", 800)
        _write(d, "gfx.pic", 1024)
        b = scan_example(d)
        assert b.vram == 1024, b.vram
        assert b.files == 1, b.files


def test_worst_pct_over_limit():
    """worst_pct exceeds 100 when an inventory overflows a limit (streaming/swaps)."""
    b = Budget("x")
    b.tiles = VRAM_LIMIT + 1
    assert b.worst_pct > 100.0
    b2 = Budget("y")
    b2.pal_bytes = (CGRAM_LIMIT + 1) * 2   # more colours than fit at once
    assert b2.worst_pct > 100.0


def test_empty_example_has_no_files():
    with tempfile.TemporaryDirectory() as td:
        b = scan_example(Path(td))
        assert b.files == 0
        assert b.vram == 0
        assert b.colours == 0


def test_oneline_prints_for_assets_and_is_silent_when_empty():
    """--oneline (used post-build): one compact line for assets, nothing when none."""
    import subprocess
    script = str(Path(__file__).resolve().parent / "asset_budget.py")
    with tempfile.TemporaryDirectory() as td:
        d = Path(td)
        _write(d, "g.pic", 1024)
        _write(d, "g.pal", 32)
        out = subprocess.run([sys.executable, script, "--oneline", td],
                             capture_output=True, text=True).stdout
        assert "[ASSETS]" in out, out
        assert out.strip().count("\n") == 0, "expected exactly one line"
    with tempfile.TemporaryDirectory() as td2:
        out2 = subprocess.run([sys.executable, script, "--oneline", td2],
                              capture_output=True, text=True).stdout
        assert out2.strip() == "", repr(out2)


if __name__ == "__main__":
    fns = [v for k, v in sorted(globals().items()) if k.startswith("test_")]
    for fn in fns:
        fn()
        print(f"ok  {fn.__name__}")
    print(f"\n{len(fns)} tests passed")
