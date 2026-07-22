#!/usr/bin/env python3
"""Post-link check: C code must not read bank-blind from bank $01+ data
(issue #104 — the read-side symmetric of the bank-$00 spill ratchet).

cc65816 dereferences are bank-$00-implicit: `lda.w sym` (direct read) and
`lda.w #sym` (address materialization then `lda.l $0000,x`) both reach
bank $00 regardless of where `sym` was linked. Passing a symbol as a far
POINTER argument is safe (the `pea.w :sym` / `pea.w sym` pair carries the
bank — how mapLoad/dmaCopyVram consume banked data).

Heuristic, per C-compiled intermediate (*.c.asm):
  - a symbol referenced as `#sym` or `.w sym` (16-bit absolute) with NO
    `:sym` bank reference anywhere in the same TU is consumed bank-blind;
  - if the linker then placed `sym` at bank >= $01, $8000+ (per the .sym),
    every such read returns garbage -> hard fail naming symbol + TU.

Usage: check_bank_reads.py <rom.sym> <dir-with-.c.asm>   (exit 1 on hit)
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

# Two reference classes with DIFFERENT rules (gap closed 2026-07-20:
# a TU that builds a far pointer — `lda.w #sym` + `lda.w #:sym` — used
# to exempt the whole symbol, hiding a constant-folded DIRECT read
# `lda.w sym+4478` of bank-$01 data in the same TU; caught live on
# mode7_racing's track_class):
#  - MEM_RE: NON-immediate `.w sym[+off]` memory operands. These read
#    through DB=$00 unconditionally -> never exempted.
#  - IMM_RE: immediate `#sym` address materialisation -> exempt when
#    the TU also takes `#:sym` (far-pointer building, the safe idiom).
MEM_RE = re.compile(
    r'\b(?:lda|sta|ldx|stx|ldy|sty|adc|sbc|cmp|cpx|cpy|and|ora|eor)\.w\s+'
    r'(?![#$:])([A-Za-z_][A-Za-z0-9_]*)\b')
IMM_RE = re.compile(
    r'\b(?:lda|sta|ldx|stx|ldy|sty|adc|sbc|cmp|cpx|cpy|and|ora|eor)\.w\s+'
    r'#(?!:)([A-Za-z_][A-Za-z0-9_]*)\b')
BANKREF_RE = re.compile(r'#:([A-Za-z_][A-Za-z0-9_]*)\b')
PEA_RE = re.compile(r'\bpea\.w\s+(?!:)([A-Za-z_][A-Za-z0-9_]*)\b')
PEA_BANK_RE = re.compile(r'\bpea\.w\s+:([A-Za-z_][A-Za-z0-9_]*)\b')


def sym_table(sym_path: Path) -> dict[str, tuple[int, int]]:
    out: dict[str, tuple[int, int]] = {}
    for line in sym_path.read_text().splitlines():
        m = re.match(r'^([0-9a-fA-F]{2}):([0-9a-fA-F]{4})\s+(\S+)$', line.strip())
        if m:
            out[m.group(3)] = (int(m.group(1), 16), int(m.group(2), 16))
    return out


def main() -> int:
    symf, srcdir = Path(sys.argv[1]), Path(sys.argv[2])
    syms = sym_table(symf)
    bad: list[str] = []
    for casm in sorted(srcdir.glob("*.c.asm")):
        text = casm.read_text()
        banked = set(BANKREF_RE.findall(text)) | set(PEA_BANK_RE.findall(text))
        blind = (set(IMM_RE.findall(text)) | set(PEA_RE.findall(text))) - banked
        blind |= set(MEM_RE.findall(text))     # direct reads: no exemption
        for name in sorted(blind):
            if name not in syms:
                continue                       # registers/defines, not linked symbols
            bank, addr = syms[name]
            if bank >= 0x01 and addr >= 0x8000:
                bad.append(f"  ${bank:02X}:{addr:04X}  {name}  (read bank-blind in {casm.name})")
    if bad:
        print("FAIL: C code reads bank $01+ data with bank-$00 addressing:")
        print("\n".join(bad))
        print("These reads return garbage at runtime (cc65816 near deref).")
        print("FIX: keep C-read data in bank $00 / RAM, or pass it to a lib")
        print("     function as a far pointer instead of dereferencing it.")
        return 1
    print("OK: no bank-blind C reads of bank $01+ data")
    return 0


def selftest() -> int:
    """Non-vacuity proof: a synthetic violation must fail, the far-pointer
    pattern must pass. Run by `make lint`."""
    import tempfile
    with tempfile.TemporaryDirectory() as td:
        d = Path(td)
        (d / "rom.sym").write_text("[labels]\n02:8000 mapdata\n00:1000 ramvar\n")
        # bank-blind deref of bank-2 data -> must FAIL
        (d / "bad.c.asm").write_text("\tlda.w #mapdata\n\ttax\n\tlda.l $0000,x\n")
        sys.argv = ["x", str(d / "rom.sym"), str(d)]
        if main() != 1:
            print("SELFTEST FAIL: bank-blind read not detected"); return 1
        # far-pointer arg (bank carried) -> must PASS
        (d / "bad.c.asm").unlink()
        (d / "good.c.asm").write_text("\tpea.w :mapdata\n\tpea.w mapdata\n\tjsl mapLoad\n\tlda.w ramvar\n")
        if main() != 0:
            print("SELFTEST FAIL: safe far-pointer pattern flagged"); return 1
    print("selftest: OK (violation detected, safe pattern accepted)")
    return 0


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--selftest":
        sys.exit(selftest())
    sys.exit(main())
