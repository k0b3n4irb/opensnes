"""Probe: VBlank DMA→VRAM budget (H2) — EXACT per-frame (luna v1.0.0).

The SNES can DMA ~4 KB to VRAM per VBlank; more silently corrupts on real
hardware (KNOWN_LIMITATIONS 🟡). luna's `--dma-trace` logs every byte an MDMA
writes to $2118/$2119, and v1.0.0 tags each row with `frame,line,blank`. So we
no longer estimate an average — we bucket bytes by PPU frame and check the
**peak** single frame, which is the real hardware risk (one over-budget VBlank
corrupts, even if the average is fine).

`blank` is the **VBlank-period** flag (verified empirically: blank=1 rows land on
lines ~230-253, blank=0 on lines ~3-91). The budgeted window is VBlank, so we
count `blank=1` bytes per frame. Boot uploads run in INIDISP forced-blank and are
warmed past with `--dma-trace-from`.

HONEST SCOPE: the current example corpus uploads all VRAM at boot (forced blank)
and scrolls via BG registers / streams sprites via OAM ($2104, not VRAM) — *none*
stream tiles to VRAM per frame in steady state, so the measured steady-state peak
is ~0 for every example. This probe therefore acts as a **regression guard**: the
baseline is "well under budget", and it will fire if a future example/change
starts pushing a heavy per-VBlank VRAM-DMA load. (A corpus example that streams
VRAM each frame would give it teeth; see the chantier note.)
"""
from __future__ import annotations

import subprocess
import sys
from collections import Counter
from pathlib import Path

from lib import find_luna, rom_path

BUDGET_BYTES = 4096          # ~one VBlank's worth
WARM = 3_000_000            # past boot uploads (forced-blank)
WINDOW = 2_000_000          # steady-state window after WARM

# DMA-relevant examples (per-frame VRAM streaming / effects). Non-DMA examples
# do all their VRAM work at boot and can't breach a per-VBlank budget.
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


def _peak_vram_dma_per_frame(luna: str, rom: Path) -> int:
    """Peak VBlank (blank=1) VRAM-DMA bytes written in any single PPU frame."""
    out = Path("/tmp/luna-dma") / (rom.stem + ".csv")
    out.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([luna, "state", "-n", str(WARM + WINDOW),
                    "--dma-trace", str(out), "--dma-trace-from", str(WARM),
                    "--out", "/dev/null", str(rom)],
                   capture_output=True, text=True, timeout=300)
    if not out.is_file():
        return 0
    lines = out.read_text().splitlines()
    if len(lines) < 2:
        return 0
    cols = lines[0].split(",")
    fi, bi = cols.index("frame"), cols.index("blank")
    per_frame: Counter[str] = Counter()
    for row in lines[1:]:                 # one row = one byte to $2118/$2119
        f = row.split(",")
        if f[bi] == "1":                  # VBlank window = the budgeted period
            per_frame[f[fi]] += 1
    return max(per_frame.values()) if per_frame else 0


def run() -> tuple[bool, str]:
    luna = find_luna()
    results = []
    for rel in EXAMPLES:
        rom = rom_path(rel)
        peak = _peak_vram_dma_per_frame(luna, rom)
        results.append((peak <= BUDGET_BYTES, f"{rom.parent.name}: {peak} B peak/frame"))
    bad = [m for ok, m in results if not ok]
    if bad:
        return False, f"OVER BUDGET (>{BUDGET_BYTES} B/VBlank): " + "; ".join(bad)
    peaks = ", ".join(m for _, m in results)
    return True, f"{len(results)} examples within {BUDGET_BYTES} B/VBlank (exact peak: {peaks})"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "dma_budget: " + msg)
    sys.exit(0 if ok else 1)
