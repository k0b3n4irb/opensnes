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
from lib import find_luna, trace_lines, assert_mem, sym_of, rom_path

# (label, rom, trace flag, minimum instructions expected, handshake-or-None).
# `handshake` = (wram_sym, hexval): a value the coprocessor writes into shared
# memory and the S-CPU reads back — proves the full chip→I-RAM→C handshake, not
# merely that instructions executed. sa1_hello's SA-1 does `sta.l $003000` with
# #$A5, copied into `sa1_status`. sa1_starfield's SYNC flag is set/cleared each
# frame (transient), so it gets execution-only coverage.
CASES = [
    ("superfx_hello", "memory/superfx_hello/superfx_hello.sfc",     "--superfx-trace", 1, None),
    ("superfx_3d",    "graphics/effects/superfx_3d/superfx_3d.sfc", "--superfx-trace", 1, None),
    ("sa1_hello",     "memory/sa1_hello/sa1_hello.sfc",             "--sa1-trace",     1, ("sa1_status", "A5")),
    ("sa1_starfield", "memory/sa1_starfield/sa1_starfield.sfc",     "--sa1-trace",     1, None),
]
STEPS = 1_000_000


def run() -> tuple[bool, str]:
    luna = find_luna()
    results = []
    for label, rel, flag, want, handshake in CASES:
        rom = rom_path(rel)
        if not rom.is_file():
            return False, f"{label}: ROM missing ({rom})"
        n = trace_lines(luna, rom, STEPS, flag)
        if n < want:
            return False, f"{label}: {n} coproc instructions executed (< {want}) — chip not running"
        tag = str(n)
        if handshake:
            sym, hexval = handshake
            bank, off = sym_of(rom, sym)
            ok, det = assert_mem(luna, rom, STEPS, [(bank, off, hexval)])
            if not ok:
                return False, f"{label}: ran ({n}) but handshake {sym}={hexval} failed ({det})"
            tag += f"+hs:{sym}={hexval}"
        results.append((label, tag))
    detail = ", ".join(f"{lbl}={tag}" for lbl, tag in results)
    return True, f"all coprocessors executed ({detail})"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "coproc: " + msg)
    sys.exit(0 if ok else 1)
