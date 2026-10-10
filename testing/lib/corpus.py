"""The example corpus: which ROMs the harness runs, and how it names them."""
from __future__ import annotations

from pathlib import Path

from .luna import REPO_ROOT


def discover_example_roms() -> list[Path]:
    """Canonical corpus = one ROM per example *that has a main.c*.

    Discovering via main.c (not a loose `*.sfc` glob) excludes stale build
    residue (a source-less example directory once inflated the count by one).
    One .sfc is expected per example dir.
    """
    roms: list[Path] = []
    for main_c in sorted(REPO_ROOT.glob("examples/**/main.c")):
        sfcs = sorted(main_c.parent.glob("*.sfc"))
        if sfcs:
            roms.append(sfcs[0])
    return roms


def example_key(rom: Path) -> str:
    """Example path relative to examples/ (the dir holding main.c)."""
    return str(rom.parent.relative_to(REPO_ROOT / "examples"))
