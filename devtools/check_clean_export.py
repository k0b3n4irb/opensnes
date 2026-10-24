#!/usr/bin/env python3
"""Build every example from a clean export of a commit, as CI will.

The local suite builds the working tree, so a file an example needs that
the commit leaves out (an ignored pattern, a forgotten `git add`) passes
here and fails on every CI leg. 91419502 shipped speech_synth's five new
.brr samples that way: `*.brr` is in .gitignore, the older samples are
tracked by force, and CI's `make clean` deleted the prebuilt
player.spc700.bin whose rebuild needed them.

For each example of the commit: `git archive` it into a temporary tree,
then `make clean` and `make` there against this tree's SDK (`bin/`, `lib/`,
`make/`, `templates/`). A failure names the example and the make error.
A ROM that builds but differs from the working tree's is reported, not
failed: the local ROM may be stale.

usage: check_clean_export.py [REV]   (default HEAD)
"""
from __future__ import annotations

import os
import subprocess
import sys
import tarfile
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def git(*args: str) -> str:
    return subprocess.run(["git", *args], cwd=ROOT, check=True, capture_output=True, text=True).stdout


def build(tree: Path, ex: str) -> tuple[str, bool, str]:
    d = tree / ex
    env = dict(os.environ, MAKEFLAGS="")
    log = ""
    for target in (["clean"], []):
        r = subprocess.run(["make", "-s", "-C", str(d), f"OPENSNES={ROOT}", *target],
                           capture_output=True, text=True, env=env)
        log += r.stdout + r.stderr
        if r.returncode:
            err = [l for l in log.splitlines() if "No rule" in l or "rror" in l]
            return ex, False, "\n    ".join(err[:3] or log.splitlines()[-3:])
    if not list(d.glob("*.sfc")):
        return ex, False, "make succeeded but wrote no .sfc"
    return ex, True, ""


def main() -> int:
    rev = sys.argv[1] if len(sys.argv) > 1 else "HEAD"
    if not (ROOT / "bin" / "cc65816").exists() or not (ROOT / "lib" / "build").exists():
        print("check_clean_export: the SDK is not built here (make first); skipped")
        return 0
    examples = sorted(str(Path(p).parent) for p in git("ls-tree", "-r", "--name-only", rev, "examples").split()
                      if p.count("/") == 3 and p.endswith("/Makefile"))
    with tempfile.TemporaryDirectory(prefix="opensnes_export_") as tmp:
        tree = Path(tmp)
        arc = subprocess.run(["git", "archive", rev, "examples"], cwd=ROOT, check=True, capture_output=True).stdout
        tar_path = tree / "examples.tar"
        tar_path.write_bytes(arc)
        with tarfile.open(tar_path) as t:
            t.extractall(tree, filter="data")
        with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
            results = list(pool.map(lambda ex: build(tree, ex), examples))
        failed = [(ex, why) for ex, ok, why in results if not ok]
        stale, compared = [], 0
        for ex, ok, _ in results:
            if not ok:
                continue
            for rom in (tree / ex).glob("*.sfc"):
                local = ROOT / ex / rom.name
                if local.exists():
                    compared += 1
                    if local.read_bytes() != rom.read_bytes():
                        stale.append(f"{ex}/{rom.name}")
    for ex, why in failed:
        print(f"FAIL {ex}: does not build from {rev} alone\n    {why}")
    for rom in stale:
        print(f"note: {rom} differs from the working tree's ROM (stale local build, or an uncommitted change)")
    print(f"check_clean_export: {len(examples) - len(failed)}/{len(examples)} examples build from a clean export of {rev}; "
          f"{compared - len(stale)}/{compared} ROMs identical to the working tree's")
    if failed:
        print("A file these examples need is not in the commit: `git status --ignored` in their folders, "
              "then `git add -f` what .gitignore hides.")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
