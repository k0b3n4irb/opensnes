#!/usr/bin/env python3
"""Measured ROM coverage of the public lib API — which functions the corpus executes.

Thin orchestrator over `luna profile --pc-set` (luna v1.21.0): for every
example ROM luna writes the set of distinct 24-bit PCs that executed up to
the example's first manifest capture frame; this script folds each PC onto
the WLA-DX `.sym` label that contains it (FastROM / HiROM mirrors folded to
the linker bank, as symmap.py does) and unions the hit labels over the
corpus. The public function names come from `lib/include/snes/*.h`.
luna measures; this file only folds and diffs (luna_tooling.md).

What it answers: "which public lib functions does NO example ever run?" —
the measured version of the gaps review's static "96 declarations called
by zero examples" (2026-09-11). Runs are input-free (the visual pillar's
boot/idle path), so input-driven code is under-counted; the manifests'
scripted legs are the follow-up.

Usage
-----
    python3 tools/luna-test/rom_coverage.py            # report + check vs the committed list
    python3 tools/luna-test/rom_coverage.py --update   # rewrite baselines/never_executed.txt
    python3 tools/luna-test/rom_coverage.py --only games/

The committed list is a ratchet: a public function that is not executed
by any example and is NOT already in the list fails the check (a new
function shipped without an example or a libtest); a listed function that
became executed is reported so the list can shrink (`--update`).

Exit 0 = no new never-executed function, 1 = the ratchet grew, 2 = usage.
"""
from __future__ import annotations

import argparse
import bisect
import re
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent / "devtools" / "symmap"))
from luna_runner import (  # noqa: E402
    HERE, REPO_ROOT, LUNA_VERSION, capture_frames, discover_example_roms, example_key,
    find_luna, load_manifest, missing_firmware,
)
from symmap import rom_bank  # noqa: E402  (mirror folding, one source of truth)

HEADERS = REPO_ROOT / "lib" / "include" / "snes"
RATCHET = HERE / "baselines" / "never_executed.txt"
REPORT = HERE / "ROM_COVERAGE.md"


def public_functions() -> dict[str, str]:
    """name -> header, for every function declared in lib/include/snes/*.h."""
    out: dict[str, str] = {}
    for h in sorted(HEADERS.glob("*.h")):
        s = h.read_text()
        s = re.sub(r"/\*.*?\*/", "", s, flags=re.S)
        s = re.sub(r"//.*", "", s)
        for m in re.finditer(r"^\s*(?:extern\s+)?[A-Za-z_][\w\s\*]*?\b([a-zA-Z_]\w*)\s*\([^;{]*\)\s*;",
                             s, flags=re.M):
            name = m.group(1)
            if name in ("if", "while", "for", "switch", "return", "sizeof", "void",
                        "int", "char", "unsigned", "signed", "static", "inline"):
                continue
            out.setdefault(name, h.name)
    return out


def load_labels(sym: Path) -> dict[int, tuple[list[int], list[str]]]:
    """bank -> (sorted addresses, names) from the .sym [labels] section."""
    per: dict[int, list[tuple[int, str]]] = {}
    in_labels = False
    for line in sym.read_text(errors="replace").splitlines():
        if line.startswith("["):
            in_labels = line.strip() == "[labels]"
            continue
        if not in_labels:
            continue
        m = re.match(r"^([0-9a-f]{2}):([0-9a-f]{4}) (\S+)$", line.strip(), flags=re.I)
        if not m:
            continue
        per.setdefault(rom_bank(int(m.group(1), 16)), []).append((int(m.group(2), 16), m.group(3)))
    folded: dict[int, tuple[list[int], list[str]]] = {}
    for bank, items in per.items():
        items.sort()
        folded[bank] = ([a for a, _ in items], [n for _, n in items])
    return folded


def executed_labels(pc_set: Path, labels: dict) -> set[str]:
    data = pc_set.read_bytes()
    hit: set[str] = set()
    for (pc,) in struct.iter_unpack("<I", data):
        bank, addr = rom_bank((pc >> 16) & 0xFF), pc & 0xFFFF
        table = labels.get(bank)
        if not table:
            continue
        addrs, names = table
        i = bisect.bisect_right(addrs, addr) - 1
        # Several labels can share an address (a data label ending right
        # where a function starts, e.g. `brr_pop_end` at `audioInit`), and a
        # PC inside a QBE block label `fn@start.12` belongs to `fn`: credit
        # every label at that address and the `@`-stripped base name.
        j = i
        while j >= 0 and addrs[j] == addrs[i]:
            hit.add(names[j]); hit.add(names[j].split("@", 1)[0]); j -= 1
    return hit


def main() -> int:
    ap = argparse.ArgumentParser(description="measured ROM coverage of the public lib API (luna --pc-set)")
    ap.add_argument("--update", action="store_true", help="rewrite the never-executed ratchet list")
    ap.add_argument("--only", metavar="SUBSTR", help="restrict to example keys containing SUBSTR (no ratchet check)")
    args = ap.parse_args()

    luna = find_luna()
    manifest = load_manifest()
    public = public_functions()
    hits: dict[str, set[str]] = {}          # function -> examples that executed it
    tmp = Path("/tmp/luna-pcset"); tmp.mkdir(parents=True, exist_ok=True)
    roms = 0
    for rom in discover_example_roms():
        key = example_key(rom)
        if args.only and args.only not in key:
            continue
        if missing_firmware(key, manifest):
            continue
        sym = rom.with_suffix(".sym")
        if not sym.is_file():
            print(f"  SKIP  {key}: no .sym", file=sys.stderr)
            continue
        frame = capture_frames(key, manifest)[0]
        pcs = tmp / (key.replace("/", "_") + ".bin")
        proc = subprocess.run(
            [luna, "profile", str(rom), "--until-frame", str(frame), "--sym", str(sym),
             "--pc-set", str(pcs), "--out", "/dev/null", "--top", "0"],
            capture_output=True, text=True, timeout=600)
        if proc.returncode != 0 or not pcs.is_file():
            print(f"  ERROR {key}: {proc.stderr.strip()[:200]}", file=sys.stderr)
            return 2
        roms += 1
        for name in executed_labels(pcs, load_labels(sym)) & public.keys():
            hits.setdefault(name, set()).add(key)

    never = sorted(n for n in public if n not in hits)
    by_header: dict[str, list[str]] = {}
    for n in never:
        by_header.setdefault(public[n], []).append(n)

    lines = [
        "# Measured ROM coverage of the public lib API",
        "",
        f"luna {LUNA_VERSION} · `luna profile --pc-set` to each example's first manifest frame, "
        f"no input · {roms} ROMs · **{len(public) - len(never)} of {len(public)} public functions "
        f"executed, {len(never)} never**",
        "",
        "> Executed = at least one PC inside the function's `.sym` label range on at least one "
        "example (boot + idle path; input-driven code is under-counted). The never-executed "
        "list is the ratchet in `baselines/never_executed.txt`.",
        "",
        "| header | never executed |",
        "|---|---|",
    ]
    lines += [f"| `{h}` | {', '.join(f'`{n}`' for n in ns)} |" for h, ns in sorted(by_header.items())]
    lines += ["", "## Least-covered executed functions (one example only)", "",
              "| function | the one example |", "|---|---|"]
    lines += [f"| `{n}` | `{next(iter(ex))}` |" for n, ex in sorted(hits.items()) if len(ex) == 1]
    if not args.only:
        REPORT.write_text("\n".join(lines) + "\n")
    print(f"ROM coverage: {len(public) - len(never)}/{len(public)} public functions executed "
          f"by {roms} ROMs; {len(never)} never.")

    if args.only:
        return 0
    if args.update:
        RATCHET.write_text("\n".join(never) + "\n")
        print(f"wrote {RATCHET.relative_to(REPO_ROOT)} ({len(never)} names) and "
              f"{REPORT.relative_to(REPO_ROOT)}")
        return 0
    known = set(RATCHET.read_text().split()) if RATCHET.is_file() else set()
    new = sorted(set(never) - known)
    gone = sorted(known - set(never))
    for n in new:
        print(f"  NEW never-executed public function: {n} ({public[n]}) — add an example or a "
              f"libtest, or --update with a reason")
    for n in gone:
        print(f"  now executed (was listed): {n} — run --update to shrink the ratchet")
    if new:
        print(f"ROM coverage ratchet: {len(new)} new never-executed function(s)")
        return 1
    print(f"ROM coverage ratchet: OK ({len(never)} listed, {len(gone)} could be removed)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
