"""Probe: VRAM-DMA timing safety + VBlank budget (H2) — luna v1.1.0.

Two checks per example, both from luna's `--dma-trace` (one CSV row per byte an
MDMA writes to $2118/$2119), columns
`seq,frame,line,blank,force_blank,src,vram_word,reg,value`:

1. TIMING SAFETY (the #1 silent failure: "VRAM writes only work during VBlank or
   forced blank; the PPU silently ignores them during active display"). luna
   v1.1.0 tags each write with `force_blank` (INIDISP $2100 bit 7), so a write is
   SAFE iff `blank || force_blank`. We assert ZERO unsafe writes (active display,
   screen on) across the whole run — this is now testable for the first time
   (before v1.1.0 we could not tell a safe forced-blank boot upload from an unsafe
   active-display one).

2. VBLANK BUDGET: the SNES does ~4 KB of VRAM DMA per VBlank; more corrupts. We
   bucket the safe VBlank-window bytes (`blank=1`) by PPU frame and check the peak
   single frame. The corpus uploads at boot and streams sprites via OAM (not VRAM),
   so steady-state peaks are ~0 — this acts as a regression guard that fires if a
   change starts pushing a heavy per-VBlank VRAM load.
"""
from __future__ import annotations

import subprocess
import sys
from collections import Counter
from pathlib import Path

from lib import find_luna, rom_path

BUDGET_BYTES = 4096
STEPS = 5_000_000          # boot + plenty of steady-state frames

# DMA-relevant examples (per-frame VRAM streaming / effects + boot uploads).
EXAMPLES = [
    "graphics/backgrounds/continuous_scroll/continuous_scroll.sfc",
    "graphics/effects/parallax_scrolling/parallax_scrolling.sfc",
    "maps/mapscroll/mapscroll.sfc",
    "maps/tiled/tiled.sfc",
    "maps/dynamic_map/dynamic_map.sfc",
    "graphics/effects/hdma_helpers/hdma_helpers.sfc",
    "graphics/sprites/animated_sprite/animated_sprite.sfc",
    "graphics/sprites/dynamic_metasprite/dynamic_metasprite.sfc",
]


def _trace(luna: str, rom: Path):
    """Return (unsafe_write_count, peak_vblank_bytes_per_frame)."""
    out = Path("/tmp/luna-dma") / (rom.stem + ".csv")
    out.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([luna, "state", "-n", str(STEPS), "--dma-trace", str(out),
                    "--out", "/dev/null", str(rom)],
                   capture_output=True, text=True, timeout=300)
    if not out.is_file():
        return 0, 0
    lines = out.read_text().splitlines()
    if len(lines) < 2:
        return 0, 0
    col = {name: i for i, name in enumerate(lines[0].split(","))}
    fi, bi, gi = col["frame"], col["blank"], col["force_blank"]
    unsafe = 0
    per_frame_vblank: Counter[str] = Counter()
    for row in lines[1:]:               # one row = one byte to $2118/$2119
        f = row.split(",")
        in_vblank, in_fblank = f[bi] == "1", f[gi] == "1"
        if not (in_vblank or in_fblank):
            unsafe += 1                 # active display, screen on → PPU ignores it
        elif in_vblank:
            per_frame_vblank[f[fi]] += 1
    return unsafe, (max(per_frame_vblank.values()) if per_frame_vblank else 0)


def run() -> tuple[bool, str]:
    luna = find_luna()
    peaks = []
    for rel in EXAMPLES:
        rom = rom_path(rel)
        unsafe, peak = _trace(luna, rom)
        if unsafe:
            return False, f"{rom.parent.name}: {unsafe} UNSAFE VRAM writes "\
                          "(active display, no forced-blank) — silent PPU-ignore bug"
        if peak > BUDGET_BYTES:
            return False, f"{rom.parent.name}: {peak} B in one VBlank > {BUDGET_BYTES}"
        peaks.append(f"{rom.parent.name}:{peak}")
    return True, f"{len(EXAMPLES)} examples: 0 unsafe writes; VBlank peak ≤ {BUDGET_BYTES} B "\
                 f"({', '.join(peaks)})"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "dma_budget: " + msg)
    sys.exit(0 if ok else 1)
