#!/usr/bin/env python3
"""Golden tests for opensnes-rom check.

Synthetic .sym and .c.asm fixtures, one per failure the check must catch:
the bank $00 ratchet, a RAM section across $2000, a data-init sentinel that
is not last, a bank-blind read of bank $01+ data, a WRAM-port write
reachable from an NMI callback; and the healthy case, whose --json is the
golden. The verdicts and figures against the five Python scripts were
compared on the 99 built ROMs of the repository on 2026-10-05 (identical).

Run:  python3 tools/opensnes-rom/tests/run_golden.py
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-rom", __file__)
g.expect_stdout("check good.sym --json (healthy)", ["check", "--json", "good.sym"], golden="good.json", copy=["good.sym", "good.c.asm"])
g.expect_refused("check good.sym --bank0-fail 30000 (ratchet)", ["check", "--bank0-fail", "30000", "good.sym"],
                 copy=["good.sym", "good.c.asm"], needles=["imminent overflow"])
g.expect_refused("check good.sym --ram-fail 8000 (ratchet)", ["check", "--ram-fail", "8000", "good.sym"],
                 copy=["good.sym", "good.c.asm"], needles=["C RAM band nearly full"])
g.expect_refused("check ram_crosser.sym (a section across $2000)", ["check", "--no-nmi-race", "ram_crosser.sym"],
                 copy=["ram_crosser.sym"], needles=["crosses $2000"])
g.expect_refused("check datainit_late.sym (sentinel not last)", ["check", "--no-nmi-race", "datainit_late.sym"],
                 copy=["datainit_late.sym"], needles=["data-init sentinel is not last"])
g.expect_refused("check bankblind.sym (bank-blind read)", ["check", "bankblind.sym"],
                 copy=["bankblind.sym", "bankblind.c.asm"], needles=["read bank-blind in bankblind.c.asm", "mapdata"])
g.expect_refused("check nmirace.sym (WRAM port from an NMI callback)", ["check", "nmirace.sym"],
                 copy=["nmirace.sym", "nmirace.c.asm"], needles=["NMI / WRAM port race", "fill", "sta.w $2180"])
g.expect_refused("check missing.sym", ["check", "missing.sym"], needles=["cannot open"])
sys.exit(g.report())
