#!/usr/bin/env python3
"""Ratchet on the hard-coded width classes in cproc's QBE emitter.

The w65816 target gives `int` 2 bytes, `long` 4 and a pointer 4, mapped to
QBE classes by `qbetype()` in `compiler/cproc/qbe.c`. Every other place in
that file that writes a width literal (`'w'`, `'l'`, `ILOADW`, `ISTOREW`,
`ILOADL`, `ISTOREL`) carries upstream's model (where `l` is 8 bytes) unless
someone re-read it for this target: `funccopy`'s chunk table, `zero()`,
the bit-field extraction and the cast scaling were each found that way,
one consumer at a time (compiler audit 2026-10-03, PF3). This script lists
those sites and fails on a NEW one until it is reviewed and added to the
baseline — the same ratchet shape as the never-executed list.

Run:  python3 devtools/check_cproc_widths.py          # in `make lint`
      python3 devtools/check_cproc_widths.py --update # after reviewing a new site
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SOURCE = REPO / "compiler" / "cproc" / "qbe.c"
BASELINE = REPO / "devtools" / "cproc_width_sites.txt"
WIDTH_RE = re.compile(r"'[wl]'|\bI(LOAD|STORE)[WL]\b")


def sites(src: Path) -> list[str]:
    """Width literals outside qbetype(), as `function: normalised line`."""
    out: list[str] = []
    fn = "(file scope)"
    depth = 0
    in_comment = False
    for raw in src.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw
        # strip block comments (one-line and multi-line) and // comments
        if in_comment:
            if "*/" in line:
                line = line.split("*/", 1)[1]
                in_comment = False
            else:
                continue
        while "/*" in line:
            head, rest = line.split("/*", 1)
            if "*/" in rest:
                line = head + rest.split("*/", 1)[1]
            else:
                line = head
                in_comment = True
        line = line.split("//", 1)[0]
        m = re.match(r"^([A-Za-z_]\w*)\(", raw)   # cproc style: name at column 0
        if m and depth == 0:
            fn = m.group(1)
        if fn != "qbetype" and WIDTH_RE.search(line):
            out.append(f"{fn}: {' '.join(line.split())}")
        depth += line.count("{") - line.count("}")
        if depth <= 0:
            depth = 0
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--update", action="store_true", help="rewrite the baseline from the current sites")
    ap.add_argument("--source", type=Path, default=SOURCE, help="the qbe.c to scan (tests)")
    ap.add_argument("--baseline", type=Path, default=BASELINE)
    args = ap.parse_args()
    if not args.source.is_file():
        print(f"cproc-widths: {args.source} missing (submodule not checked out) — skipped")
        return 0
    now = sites(args.source)
    if args.update:
        args.baseline.write_text("\n".join(now) + "\n", encoding="utf-8")
        print(f"cproc-widths: baseline rewritten, {len(now)} site(s)")
        return 0
    known = set(args.baseline.read_text(encoding="utf-8").splitlines()) if args.baseline.is_file() else set()
    new = [s for s in now if s not in known]
    gone = sorted(known - set(now))
    for s in new:
        print(f"  NEW hard-coded width in cproc: {s}")
    if new:
        print(f"cproc-widths: {len(new)} new site(s). A width literal outside qbetype() "
              f"must be read against this target (int 2, long 4, pointer 4 bytes) — "
              f"the funccopy / zero / bit-field class of silent miscompile. Review it, "
              f"then `--update`.")
        return 1
    print(f"cproc-widths: OK ({len(now)} known site(s)"
          + (f", {len(gone)} no longer present — `--update` to drop them" if gone else "") + ")")
    return 0


if __name__ == "__main__":
    sys.exit(main())
