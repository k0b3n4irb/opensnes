#!/usr/bin/env python3
"""Code that does not fit bank $00 runs from the next banks (issue #168).

For each example below: a copy is built with a filler pinned to bank $00
that takes its free space and 3000 bytes more, so the linker has to place
C functions and library routines in bank $01. The copy must then show, frame for frame, what the
normal build shows (`luna diff`, tolerance 0), and its .sym must list code
sections outside bank $00 — otherwise the test proved nothing.

The filler is an assembly section marked KEEP: the link drops what nothing
refers to (`-d`), so a filler written as a C function that nobody calls
would fill nothing (a real game found that out while reproducing this).

Orchestration only: make, wlalink's .sym and luna give the answers.
    python3 testing/bank_spill.py [--keep]
"""
import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LUNA = ROOT / "testing" / "bin" / "luna"
WORK = ROOT / "build" / "spill"          # three levels under the root, like examples/<cat>/<name>
EXAMPLES = ["games/likemario", "games/breakout", "sprites/dynamic_sprite", "text/print_string"]
FRAMES = "60,120,200,300"
EVICT = 3000                             # bytes of code the filler pushes out of bank $00


def sections(sym: Path):
    """(bank, size, name) of every section of a wlalink .sym; the FastROM /
    HiROM window ($80, $C0) folded back to the linker bank."""
    out, on = [], False
    for line in sym.read_text().splitlines():
        if line.startswith("["):
            on = line.strip() == "[sections]"
            continue
        p = line.split()
        if on and len(p) == 5:
            out.append((int(p[1].split(":")[0], 16) & 0x3F, int(p[3], 16), p[4]))
    return out


def bank0_used(sym: Path) -> int:
    top = 0x8000
    for line in sym.read_text().splitlines():
        m = re.match(r"([0-9a-fA-F]{2}):([0-9a-fA-F]{4}) (\S+)$", line)
        if m and int(m.group(1), 16) & 0x3F == 0 and int(m.group(2), 16) >= 0x8000 \
                and not m.group(3).startswith("RAM_USAGE_"):
            top = max(top, int(m.group(2), 16))
    return top - 0x8000


def run(cmd, cwd):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)


def one(example: str, keep: bool) -> str:
    src = ROOT / "examples" / example
    rom = next(src.glob("*.sfc"), None)
    if rom is None:
        return f"{example}: not built (make examples first)"
    dst = WORK / example.replace("/", "_")
    shutil.rmtree(dst, ignore_errors=True)
    shutil.copytree(src, dst, ignore=shutil.ignore_patterns("*.o", "*.sfc", "*.sym", "*.c.asm", "*.wrap.asm"))
    free = 0x8000 - bank0_used(rom.with_suffix(".sym"))
    fill = free + EVICT
    (dst / "bank0_filler.asm").write_text(
        "; test only (testing/bank_spill.py): bank $00 is given no room\n"
        '.SECTION "bank0_filler" SEMIFREE BANK 0 KEEP\n'
        f"bank0_filler:\n    .dsb {fill}, $EA\n.ENDS\n")
    mk = (dst / "Makefile").read_text()
    mk = re.sub(r"^(include .*common\.mk)", r"ASMSRC += bank0_filler.asm\n\1", mk, count=1, flags=re.M)
    (dst / "Makefile").write_text(mk)
    r = run(["make", f"OPENSNES={ROOT}"], dst)
    spill = next(dst.glob("*.sfc"), None)
    if r.returncode or spill is None:
        return f"{example}: the build with a full bank $00 failed:\n{(r.stdout + r.stderr)[-600:]}"
    moved = [(s, n) for b, s, n in sections(spill.with_suffix(".sym")) if b > 0 and n.startswith(".text.")]
    if not moved:
        return f"{example}: no code section left bank $00 (filler {fill}) — the test proves nothing"
    d = run([str(LUNA), "diff", str(rom), str(spill), "--frames", FRAMES, "--tolerance", "0"], ROOT)
    if d.returncode:
        return f"{example}: the ROM with code in bank $01 does not show the same frames:\n{d.stdout[-500:]}"
    print(f"  ok  {example}: {len(moved)} code sections ({sum(s for s, _ in moved)} bytes) outside bank $00, "
          f"frames {FRAMES} identical")
    if not keep:
        shutil.rmtree(dst, ignore_errors=True)
    return ""


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--keep", action="store_true", help="keep the built copies under build/spill/")
    args = ap.parse_args()
    errs = [e for e in (one(x, args.keep) for x in EXAMPLES) if e]
    for e in errs:
        print("  FAIL " + e)
    print(f"bank spill: {len(EXAMPLES) - len(errs)}/{len(EXAMPLES)} ok")
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main())
