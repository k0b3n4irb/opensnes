#!/usr/bin/env python3
"""Lint: lib C modules must not carry ROM const data.

A `static const` array/struct in lib/source/*.c gets a SUPERFREE `.rodata.N`
section. On a tight example, WLA-DX can place that section in bank $01+ —
and section placement varies by platform/link order — where the compiler's
16-bit C deref reads garbage (the KNOWN_LIMITATIONS bank $00 class). The
symmap ratchet hard-fails such a build after the fact; this lint removes
the cause: lib data that C code reads must be RAM (initialized statics are
copied by the bank-aware data_init DMA loop) or computed.

Caught live twice on 2026-07-07 (window example): hdma.c's sine_quarter
spilled locally, then channel_mask spilled on the macOS CI leg only.

Exempt: `__opensnes_force_emit_*` anchors — const ROM data by design,
referenced only by the linker, never dereferenced by C.

Run: python3 devtools/check_lib_rodata.py   (wired into `make lint`)
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

LIB_C = Path(__file__).resolve().parent.parent / "lib" / "source"

# `static const <type> name[...] = {` / `static const <type> name = {`
# but NOT `static const <type> *name` (a mutable pointer to const = RAM).
PATTERN = re.compile(
    r"^\s*static\s+const\s+[A-Za-z_][A-Za-z0-9_ ]*\s+(?!\*)"
    r"(?P<name>\**[A-Za-z_][A-Za-z0-9_]*)\s*(\[[^\]]*\])?\s*=",
    re.M)


def main() -> int:
    findings = []
    for src in sorted(LIB_C.glob("*.c")):
        text = src.read_text()
        for m in PATTERN.finditer(text):
            name = m.group("name").lstrip("*")
            if name.startswith("__opensnes_force_emit_"):
                continue
            line = text[:m.start()].count("\n") + 1
            findings.append(f"{src.relative_to(LIB_C.parent.parent)}:{line}: "
                            f"static const data '{name}' in a lib C module")
    if findings:
        print("ERROR: ROM const data in lib C modules (bank $01+ spill risk,")
        print("platform-dependent — see devtools/check_lib_rodata.py):\n")
        for f in findings:
            print(f"  {f}")
        print("\nFIX: drop the const (initialized RAM statics are copied by the")
        print("bank-aware data_init loop) or compute the values.")
        return 1
    print("OK: no ROM const data in lib C modules")
    return 0


if __name__ == "__main__":
    sys.exit(main())
