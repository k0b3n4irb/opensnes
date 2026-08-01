"""Probe: DMA→VRAM and DMA→CGRAM byte-exact copies (ported from dma/cgram).

fundamentals/text_glyphs boots with:
  - dmaCopyVram(font_tiles, 0x0000, 144) → VRAM[0..144] must equal font_tiles.
  - dmaCopyCGram(bg_palette, 0, 4)       → CGRAM[0..2 colours] must equal bg_palette.
Proves the DMA engine moved the exact source bytes to the right destination.
"""
from __future__ import annotations

import sys
from lib import (find_luna, sym_size, peek, dump_vram, cgram_words,
                 rom_path)

ROM = "fundamentals/text_glyphs/text_glyphs.sfc"


def _dma_vram(luna) -> tuple[bool, str]:
    rom = rom_path(ROM)
    size = sym_size(rom, "font_tiles")            # 144
    src = peek(luna, rom, 1_000_000, "font_tiles", size)
    vram = dump_vram(luna, rom, 1_000_000)[:size]  # dest word 0x0000 → byte 0
    ok = bytes(src) == vram
    return ok, f"dma→VRAM {size}B {'match' if ok else 'MISMATCH'}"


def _dma_cgram(luna) -> tuple[bool, str]:
    rom = rom_path(ROM)
    size = sym_size(rom, "bg_palette")            # 4 bytes = 2 colours
    src = peek(luna, rom, 1_000_000, "bg_palette", size)
    cg = cgram_words(luna, rom, 1_000_000)
    ok = True
    for i in range(size // 2):
        want = (src[2 * i] | (src[2 * i + 1] << 8)) & 0x7FFF
        if (cg[i] & 0x7FFF) != want:
            ok = False
            break
    return ok, f"dma→CGRAM {size // 2} colours {'match' if ok else 'MISMATCH'}"


def run() -> tuple[bool, str]:
    luna = find_luna()
    checks = [_dma_vram(luna), _dma_cgram(luna)]
    bad = [m for ok, m in checks if not ok]
    if bad:
        return False, "FAILED: " + "; ".join(bad)
    return True, "; ".join(m for _, m in checks)


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "dma_cgram: " + msg)
    sys.exit(0 if ok else 1)
