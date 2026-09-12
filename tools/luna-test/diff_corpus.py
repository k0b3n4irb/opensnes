#!/usr/bin/env python3
"""A/B the corpus at equal PPU frame — the Class A validation protocol.

Thin orchestrator over `luna diff` (it drives luna and asserts on its exit
codes; it computes nothing itself — .claude/rules/luna_tooling.md). For every
example ROM in the current tree it finds the same-named ROM in a REFERENCE
tree (a copy of `examples/` built before the change, or a git worktree of
the base commit built with the old toolchain) and runs

    luna diff <ref rom> <new rom> --frames <manifest frames> --tolerance N

`MATCH (offset k)` means the new build renders the reference frame F at
F ± k — the boot-length shift a codegen change can introduce is reported,
not hidden. `DIFF` is a real rendering change and the example is listed.
This is the proof a compiler / library change wants BEFORE any re-baseline:
"same picture at the same frame on every example, modulo k".

Usage
-----
    cp -r examples /tmp/examples_before        # before the change (built)
    ... change, make clean && make ...
    python3 tools/luna-test/diff_corpus.py --ref /tmp/examples_before
    python3 tools/luna-test/diff_corpus.py --ref /tmp/examples_before --tolerance 3 --only games/

Exit 0 = every compared frame matched, 1 = at least one DIFF or a missing
reference ROM, 2 = usage error.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from luna_runner import (  # noqa: E402
    REPO_ROOT, capture_frames, discover_example_roms, example_key, find_luna,
    load_manifest, missing_firmware,
)


def main() -> int:
    ap = argparse.ArgumentParser(description="luna diff over the example corpus")
    ap.add_argument("--ref", required=True, metavar="DIR",
                    help="reference examples/ tree (same layout, built before the change)")
    ap.add_argument("--tolerance", type=int, default=0,
                    help="accept a match up to N frames away (boot-length shift)")
    ap.add_argument("--only", metavar="SUBSTR", help="restrict to example keys containing SUBSTR")
    ap.add_argument("--power-on", metavar="MODE", help="luna --power-on for both machines")
    ap.add_argument("--screenshot-dir", metavar="DIR", default="/tmp/luna-diff",
                    help="where luna writes frame_<F>_a/b.png for every DIFF frame")
    args = ap.parse_args()

    ref_root = Path(args.ref).resolve()
    if not ref_root.is_dir():
        print(f"ERROR: --ref {ref_root} is not a directory", file=sys.stderr)
        return 2
    luna = find_luna()
    manifest = load_manifest()
    total = matched = diffs = missing = skipped = 0
    for rom in discover_example_roms():
        key = example_key(rom)
        if args.only and args.only not in key:
            continue
        if missing_firmware(key, manifest):
            skipped += 1
            continue
        ref_rom = ref_root / rom.relative_to(REPO_ROOT / "examples")
        total += 1
        if not ref_rom.is_file():
            missing += 1
            print(f"  MISSING {key}: no reference ROM at {ref_rom}")
            continue
        frames = ",".join(map(str, capture_frames(key, manifest)))
        cmd = [luna, "diff", str(ref_rom), str(rom), "--frames", frames,
               "--tolerance", str(args.tolerance),
               "--screenshot-dir", str(Path(args.screenshot_dir) / key.replace("/", "_"))]
        if args.power_on:
            cmd += ["--power-on", args.power_on]
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
        lines = [l for l in proc.stdout.splitlines() if l.startswith("# frame")]
        summary = "; ".join(l.removeprefix("# ") for l in lines) or proc.stdout.strip()[:120]
        if proc.returncode == 0:
            matched += 1
            print(f"  MATCH   {key}: {summary}")
        elif proc.returncode == 1:
            diffs += 1
            print(f"  DIFF    {key}: {summary}  (PNGs: {args.screenshot_dir}/{key.replace('/', '_')})")
        else:
            diffs += 1
            print(f"  ERROR   {key}: {proc.stderr.strip()[:200]}")
    if total == 0:
        print("ERROR: no example ROM found in the current tree (build the corpus first)"
              + (f"; no key matched --only {args.only!r}" if args.only else ""), file=sys.stderr)
        return 2
    print(f"\nDIFF CORPUS: {matched}/{total} match, {diffs} diff, {missing} missing"
          + (f", {skipped} skipped (firmware)" if skipped else "")
          + f"  (tolerance ±{args.tolerance})")
    return 1 if (diffs or missing) else 0


if __name__ == "__main__":
    sys.exit(main())
