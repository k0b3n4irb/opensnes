#!/usr/bin/env python3
"""VRAM base-alignment linter (stdlib only).

The SNES programs BG/sprite VRAM bases through registers that store only the HIGH
bits of the address, so the lib silently MASKS off the low bits. A misaligned
base is therefore a silent failure — the value you wrote is not the value the PPU
uses, with no error at build or run time:

  bgSetGfxPtr(0, 0x0800)  -> BG12NBA = 0x0800>>12      = 0      -> tiles  @ 0x0000
  bgSetMapPtr(0, 0x0401)  -> BGnSC   = (0x0401>>8)&0xFC = 0x04  -> tilemap @ 0x0400
  OBJSEL(size, 0x1000)    -> (0x1000>>13)&7             = 0      -> sprites @ 0x0000

This scans each example `.c` for those three calls, resolves the VRAM-address
argument (a literal, or a simple `#define NAME 0xHEX`), and FAILS the build if a
*resolved* base is not aligned to its hardware granularity. Arguments it cannot
resolve statically (a variable, an expression, a computed `#define`) are skipped
— the linter never false-fails.

Granularities mirror the SDK source so the check can't drift from the lib:
  bgSetGfxPtr -> 0x1000  (background.c BG12NBA = vramAddr>>12)
  bgSetMapPtr -> 0x400   (background.c BGnSC   = (vramAddr>>8)&0xFC)
  OBJSEL      -> 0x2000  (sprite.h OBJ_BASE    = (vram_addr>>13)&0x07)

Pure stdlib, no emulator, no build. Run via `make lint-vram` (wired into
`make lint` and the lint CI workflow).
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

# call name -> (alignment in words, 0-based index of the VRAM-address argument)
CHECKS = {
    "bgSetGfxPtr": (0x1000, 1),
    "bgSetMapPtr": (0x400, 1),
    "OBJSEL": (0x2000, 1),
}

_DEFINE_RE = re.compile(r"^[ \t]*#define[ \t]+(\w+)[ \t]+(0[xX][0-9A-Fa-f]+|\d+)\b", re.M)
_NUM_RE = re.compile(r"^(0[xX][0-9A-Fa-f]+|\d+)$")


def _resolve(tok: str, defines: dict[str, int]):
    tok = tok.strip()
    if _NUM_RE.match(tok):
        return int(tok, 0)
    return defines.get(tok)  # None if a variable / expression / computed macro


def _args(argstr: str) -> list[str]:
    """Split a call's argument list on top-level commas."""
    out, depth, cur = [], 0, ""
    for ch in argstr:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    out.append(cur)
    return out


def lint_text(text: str):
    """Return [(line, call, addr, align, masked)] for misaligned resolved bases."""
    defines = {m.group(1): int(m.group(2), 0) for m in _DEFINE_RE.finditer(text)}
    issues = []
    for name, (align, idx) in CHECKS.items():
        for m in re.finditer(rf"\b{name}\s*\(([^)]*)\)", text):
            args = _args(m.group(1))
            if idx >= len(args):
                continue
            addr = _resolve(args[idx], defines)
            if addr is None or addr % align == 0:
                continue
            line = text[: m.start()].count("\n") + 1
            issues.append((line, name, addr, align, addr & ~(align - 1)))
    return issues


def main() -> int:
    failures = 0
    for c in sorted((REPO / "examples").rglob("*.c")):
        for line, name, addr, align, masked in lint_text(c.read_text(errors="replace")):
            rel = c.relative_to(REPO)
            print(f"{rel}:{line}: {name} base 0x{addr:04X} not aligned to 0x{align:X}"
                  f" -> hardware silently uses 0x{masked:04X}")
            failures += 1
    if failures:
        print(f"\nVRAM alignment: {failures} misaligned base(s) — silent-failure bug.")
        return 1
    print("VRAM alignment: OK (no misaligned BG/sprite bases).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
