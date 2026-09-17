#!/usr/bin/env python3
"""Every lib module links — alone, and all together (gaps review L2a, 2026-09-13).

Two facts the corpus never proved:

  1. Each module links ALONE with only the dependencies make/common.mk
     declares for it (`_DEP_<module>`). The one example that first linked
     `console` without `dma` found a missing `_DEP_console` the hard way;
     this is that check for all of them, every run.
  2. Every non-chip module links into a ROM with (almost) all the others.
     A module no example lists (`profile`, `math_ease`, `lzss`, `debug`,
     `mosaic`, `scene`, ...) can rot without anyone noticing; here it is
     linked on every `make tests`. Two groups, not one: `hdma` keeps
     ~2.8 KB of tables in plain RAM and the dynamic-sprite family
     (`sprite_dynamic*`, `object`) ~2.9 KB (a 2 KB `oambuffer` plus the
     upload queue), and the plain C RAM band is 8 KB, so the whole lib
     does not fit one ROM's RAM. Group A is everything but hdma, group B
     everything but the dynamic-sprite family; each module is in at
     least one.

Modules are discovered from lib/build/lorom (one token per `<mod>.o` /
`<mod>-asm.o`). `runtime`, `mul32` and `div32` are always linked by
common.mk and are not modules. Chip / engine modules need their mode:
sa1 (USE_SA1), superfx (USE_SUPERFX), dsp1 (USE_DSP1), sram (USE_SRAM),
snesmod (USE_SNESMOD) — they link alone in that mode and stay out of the
all-in-one ROM (snesmod and the audio v2 engine are two APU drivers).

Each build is a throwaway project under devtools/link_modules/build/<name>
with a `main.c` that does nothing: the point is the link, not the code.
Exit 0 = every module links; 1 = a build failed (its make output follows).
"""
from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LIBDIR = ROOT / "lib" / "build" / "lorom"
BUILD = ROOT / "devtools" / "link_modules" / "build"

ALWAYS = {"runtime", "mul32", "div32"}          # linked by common.mk, not modules
MODE = {                                          # module -> the make flag its objects need
    "sa1": "USE_SA1=1",
    "superfx": "USE_SUPERFX=1",
    "dsp1": "USE_DSP1=1",
    "sram": "USE_SRAM=1",
    "snesmod": "USE_SNESMOD=1",
}
NOT_IN_ALL = set(MODE)                            # chip modes are exclusive; snesmod is the other APU engine
DYNAMIC = {"sprite_dynamic", "sprite_dynamic_dispatch", "sprite_dynamic_helpers",
           "sprite_dynamic_meta", "object"}       # ~2.9 KB of plain RAM
GROUPS = {                                        # name -> modules excluded from that all-in-one ROM
    "all_but_hdma": {"hdma"},
    "all_but_dynamic": DYNAMIC,
}

MAIN_C = """/* link-only smoke project (devtools/link_modules.py): the ROM must link, that is all */
int main(void) {
    for (;;) {
    }
    return 0;
}
"""

MAKEFILE = """OPENSNES := {root}
TARGET   := {name}.sfc
ROM_NAME := LINK {upper}
USE_LIB  := 1
LIB_MODULES := {modules}
CSRC := main.c
{flags}
include $(OPENSNES)/make/common.mk
"""


def discover() -> list[str]:
    mods: set[str] = set()
    for o in LIBDIR.glob("*.o"):
        name = re.sub(r"(-asm)?\.o$", "", o.name)
        mods.add(name)
    return sorted(mods - ALWAYS)


def build(name: str, modules: list[str], flags: list[str]) -> tuple[bool, str, str]:
    """Returns (ok, bank0-free line or '', output)."""
    d = BUILD / name
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    (d / "main.c").write_text(MAIN_C)
    (d / "Makefile").write_text(MAKEFILE.format(
        root=ROOT, name=name, upper=name.upper()[:20], modules=" ".join(modules),
        flags="\n".join(f"{f.split('=')[0]} := {f.split('=')[1]}" for f in flags)))
    proc = subprocess.run(["make", "-s"], cwd=d, capture_output=True, text=True, timeout=600)
    out = (proc.stdout or "") + (proc.stderr or "")
    ok = proc.returncode == 0 and (d / f"{name}.sfc").is_file()
    m = re.search(r"bank \$00 ROM free: (\d+) bytes", out) or re.search(r"nearly full \((\d+) bytes free", out)
    r = re.search(r"C RAM band \$0000-\$1FFF: (\d+) bytes free", out)
    free = (f"bank $00 free {m.group(1):>5} B" if m else "") + (f"  RAM free {r.group(1):>4} B" if r else "")
    return ok, free, out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--only", metavar="MODULE", help="build one module alone (and skip the all-in-one)")
    ap.add_argument("--keep", action="store_true", help="keep the build directories")
    args = ap.parse_args()
    if not LIBDIR.is_dir():
        sys.exit("link-modules: lib not built (make lib first)")
    mods = discover()
    failures: list[tuple[str, str]] = []
    n = 0
    for mod in mods:
        if args.only and mod != args.only:
            continue
        n += 1
        ok, free, out = build(f"only_{mod}", [mod], [MODE[mod]] if mod in MODE else [])
        if ok:
            print(f"  PASS  {mod:<24} links alone  {free}")
        else:
            print(f"  FAIL  {mod:<24} does not link alone")
            failures.append((mod, out))
    groups_ok = True
    if not args.only:
        for gname, excluded in GROUPS.items():
            gmods = [m for m in mods if m not in NOT_IN_ALL and m not in excluded]
            ok, free, out = build(gname, gmods, [])
            if ok:
                print(f"  PASS  {gname:<24} {len(gmods)} modules together  {free}")
            else:
                print(f"  FAIL  {gname:<24} {len(gmods)} modules together do not link")
                failures.append((gname, out))
                groups_ok = False
    for name, out in failures:
        print(f"\n--- {name}: make output (tail)\n{out.strip()[-1800:]}")
    if not args.keep:
        shutil.rmtree(BUILD, ignore_errors=True)
    alone_fail = sum(1 for f, _ in failures if f not in GROUPS)
    print(f"\nlink-modules: {n - alone_fail}/{n} modules link alone"
          f"{'' if args.only else ', all-together groups ' + ('OK' if groups_ok else 'FAILED')}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
