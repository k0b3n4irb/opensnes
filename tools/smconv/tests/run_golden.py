#!/usr/bin/env python3
"""Golden-output tests for smconv (SNESMOD soundbank converter).

Copies the fixture .it module into a temp dir, runs smconv with the
canonical soundbank flags from make/common.mk, and byte-compares the
produced soundbank files against the committed goldens. smconv output
is deterministic, so any diff is a real behaviour change: either a
regression, or an intentional change that must be re-goldened.

Fixture provenance (PVSnesLib-derived asset already in the repo, see
ATTRIBUTION.md):
  fixtures/pollen8.it = examples/audio/snesmod_music/music/pollen8.it

Run:  python3 tools/smconv/tests/run_golden.py
"""
from __future__ import annotations

import filecmp
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TOOL = REPO / "bin" / "smconv"

FIXTURE = "pollen8.it"
# Canonical invocation from make/common.mk's soundbank rule.
FLAGS = ["-s", "-o", "soundbank", "-b", "1", "-n", "-p", "soundbank"]
OUTPUTS = ["soundbank.asm", "soundbank.h", "soundbank.bnk"]


def main() -> int:
    if not TOOL.is_file():
        sys.exit(f"ERROR: {TOOL} not found — run `make tools` first")
    errs = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        shutil.copy(HERE / "fixtures" / FIXTURE, work / FIXTURE)
        proc = subprocess.run([str(TOOL), *FLAGS, FIXTURE],
                              cwd=work, capture_output=True, text=True, timeout=60)
        if proc.returncode != 0:
            errs.append(f"exit {proc.returncode}: "
                        f"{(proc.stderr or proc.stdout).strip()[:200]}")
        else:
            for out in OUTPUTS:
                got = work / out
                want = HERE / "golden" / out
                if not got.is_file():
                    errs.append(f"{out}: not produced")
                elif not filecmp.cmp(got, want, shallow=False):
                    errs.append(f"{out}: differs from golden ({got.stat().st_size} "
                                f"vs {want.stat().st_size} bytes)")
    # Negative case (2026-09-26): a file that is not an IT module must fail
    # the run and write nothing. It used to exit 0 with an empty soundbank.
    neg_errs = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        (work / "not_an_it.it").write_bytes(b"\x89PNG\r\n\x1a\n" + bytes(64))
        proc = subprocess.run([str(TOOL), *FLAGS, "not_an_it.it"],
                              cwd=work, capture_output=True, text=True, timeout=60)
        if proc.returncode == 0:
            neg_errs.append("exit 0 on a non-IT file")
        if "IMPM" not in proc.stdout + proc.stderr:
            neg_errs.append("no message naming the IMPM signature")
        left = [out for out in OUTPUTS if (work / out).exists()]
        if left:
            neg_errs.append(f"wrote {', '.join(left)}")
    ok = (0 if errs else 1) + (0 if neg_errs else 1)
    if errs:
        print(f"  FAIL {FIXTURE}: " + "; ".join(errs))
    else:
        print(f"  PASS {FIXTURE} ({len(OUTPUTS)} outputs match)")
    if neg_errs:
        print("  FAIL not_an_it.it: " + "; ".join(neg_errs))
    else:
        print("  PASS not_an_it.it (refused, nothing written)")
    print(f"\nsmconv golden: {ok}/2 ok")
    return 0 if ok == 2 else 1


if __name__ == "__main__":
    sys.exit(main())
