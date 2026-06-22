"""Probe: VBlank DMA→VRAM budget (H2).

The SNES can DMA ~4 KB to VRAM per VBlank; more silently corrupts on real
hardware (KNOWN_LIMITATIONS 🟡). luna's `--dma-trace` logs every byte an MDMA
writes to $2118/$2119. We measure *steady-state* (post-boot) VRAM-DMA volume per
PPU frame and flag examples that exceed the budget.

NOTE — estimate, not exact: `--dma-trace` rows are `seq,src,vram_word,reg,value`
with **no frame/scanline column**, so per-VBlank bucketing isn't possible
directly. We approximate bytes/frame over a post-boot window (frame delta from
two `state` snapshots). The exact per-VBlank check needs a luna enhancement —
filed as feature request **L13** (frame/scanline column on `--dma-trace`).
Boot-time uploads run in forced blank (unbudgeted) and are excluded by warming
past them.
"""
from __future__ import annotations

import json
import subprocess
import sys
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


def _frame(luna: str, rom: Path, steps: int) -> int:
    p = subprocess.run([luna, "state", "-n", str(steps), "--out", "-", str(rom)],
                       capture_output=True, text=True, timeout=300)
    return json.loads(p.stdout)["scheduler"]["frame_count"]


def _vram_dma_bytes(luna: str, rom: Path) -> int:
    out = Path("/tmp/luna-dma") / (rom.stem + ".csv")
    out.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([luna, "state", "-n", str(WARM + WINDOW),
                    "--dma-trace", str(out), "--dma-trace-from", str(WARM),
                    "--out", "/dev/null", str(rom)],
                   capture_output=True, text=True, timeout=300)
    if not out.is_file():
        return 0
    n = sum(1 for _ in out.open()) - 1   # minus header; one row = one byte
    return max(0, n)


def run() -> tuple[bool, str]:
    luna = find_luna()
    results = []
    for rel in EXAMPLES:
        rom = rom_path(rel)
        f1 = _frame(luna, rom, WARM)
        f2 = _frame(luna, rom, WARM + WINDOW)
        frames = max(1, f2 - f1)
        per_frame = _vram_dma_bytes(luna, rom) / frames
        ok = per_frame <= BUDGET_BYTES
        results.append((ok, f"{rom.parent.name}: ~{per_frame:.0f} B/frame"))
    bad = [m for ok, m in results if not ok]
    if bad:
        return False, f"OVER BUDGET (>{BUDGET_BYTES} B/VBlank): " + "; ".join(bad)
    return True, f"{len(results)} examples within ~{BUDGET_BYTES} B/VBlank (estimate)"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "dma_budget: " + msg)
    sys.exit(0 if ok else 1)
