"""Probe: coprocessor execution proof (luna v1.0.0 instruction traces).

The old snes9x-WASM harness famously could not even *detect* the Super FX GSU
("GSU: NOT DETECTED") and needed a Mesen2 side channel. luna runs SA-1 / Super FX
natively, and v1.0.0 exposes per-instruction traces (`--superfx-trace`,
`--sa1-trace`). This probe turns "luna runs the chip" into a *positive,
regression-guarding assertion*: each coprocessor example must execute ≥1
coprocessor instruction. If a future codegen/template change silently stops the
GSU/SA-1 from running, the framebuffer might still look plausible — this catches
it where the visual gate can't.
"""
from __future__ import annotations

import sys
from lib import find_luna, trace_lines, rom_path

# (label, rom, trace flag, minimum instructions expected).
CASES = [
    ("superfx_hello",   "memory/superfx_hello/superfx_hello.sfc",          "--superfx-trace", 1),
    ("superfx_3d",      "graphics/effects/superfx_3d/superfx_3d.sfc",      "--superfx-trace", 1),
    ("sa1_hello",       "memory/sa1_hello/sa1_hello.sfc",                  "--sa1-trace",     1),
    ("sa1_starfield",   "memory/sa1_starfield/sa1_starfield.sfc",         "--sa1-trace",     1),
]
STEPS = 1_000_000


def run() -> tuple[bool, str]:
    luna = find_luna()
    results = []
    for label, rel, flag, want in CASES:
        rom = rom_path(rel)
        if not rom.is_file():
            return False, f"{label}: ROM missing ({rom})"
        n = trace_lines(luna, rom, STEPS, flag)
        results.append((label, n, want))
        if n < want:
            return False, f"{label}: {n} coproc instructions executed (< {want}) — chip not running"
    detail = ", ".join(f"{lbl}={n}" for lbl, n, _ in results)
    return True, f"all coprocessors executed ({detail})"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "coproc: " + msg)
    sys.exit(0 if ok else 1)
