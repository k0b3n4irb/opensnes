#!/usr/bin/env python3
"""WRAM-state regression (H7) — a per-frame state oracle stronger than fbhash.

For each example, `luna wram-trace` emits a vblank-aligned FNV-1a hash of every
WRAM page for N consecutive frames. We SHA-256 that whole stream into one key and
compare it against a committed baseline. This catches runtime-state regressions
that never reach the screen (an uninitialised read, a mis-stepped counter, a
changed allocation) — which the framebuffer fbhash can't see.

LOCAL, SAME-ARCH TOOL — NOT a CI gate. Unlike the framebuffer (luna guarantees
`--print-fbhash` cross-arch), raw WRAM content is *not* a cross-arch guarantee:
most examples hash identically across hosts, but two (mapandobjects, slopemario)
diverge x86_64 ↔ aarch64. So the committed `baselines/wram.json` is only valid on
its capture arch; run `--update` on your own machine, then `--compare` to catch
"did my change alter invisible runtime state?" that the framebuffer can't see.

  make test-wram                                    # compare vs (your) baseline
  python3 tools/luna-test/wram_regress.py --update  # (re)baseline on this machine

Exit 0 = all match, 1 = any drift.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from luna_runner import find_luna, discover_example_roms, example_key  # noqa: E402

BASELINE = HERE / "baselines" / "wram.json"
FRAMES = 90   # consecutive vblank-aligned frames to hash


def stream_hash(luna: str, rom: Path) -> str:
    out = Path("/tmp/luna-wram") / f"{example_key(rom).replace('/', '_')}.txt"
    out.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run(
        [luna, "wram-trace", "-n", "0", "-c", str(FRAMES), "--out", str(out), str(rom)],
        capture_output=True, text=True, timeout=300,
    )
    if not out.is_file() or out.stat().st_size == 0:
        raise RuntimeError(f"wram-trace failed for {rom.name}: {proc.stderr.strip()[:200]}")
    return hashlib.sha256(out.read_bytes()).hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser(description="WRAM-state regression (H7)")
    ap.add_argument("--update", action="store_true", help="(re)write the baseline")
    ap.add_argument("--only", metavar="SUBSTR")
    args = ap.parse_args()
    luna = find_luna()
    db = json.loads(BASELINE.read_text()) if BASELINE.is_file() else {}

    fails = updated = count = 0
    for rom in discover_example_roms():
        label = example_key(rom).replace("/", "_")
        if args.only and args.only not in label:
            continue
        count += 1
        try:
            h = stream_hash(luna, rom)
        except RuntimeError as e:
            print(f"  ERROR {label}: {e}")
            fails += 1
            continue
        if args.update:
            db[label] = h
            updated += 1
            print(f"  BASELINE {label}  {h[:16]}…")
        elif label not in db:
            print(f"  MISS  {label}: no baseline — run --update first")
            fails += 1
        elif h == db[label]:
            print(f"  PASS  {label}")
        else:
            print(f"  FAIL  {label}: WRAM-state stream changed ({h[:16]}… != {db[label][:16]}…)")
            fails += 1

    if args.update:
        BASELINE.parent.mkdir(parents=True, exist_ok=True)
        BASELINE.write_text(json.dumps(dict(sorted(db.items())), indent=2) + "\n")
        print(f"\nwrote {BASELINE.relative_to(HERE.parent.parent)} ({updated} entries)")
    print(f"\nWRAM regression: {count - fails}/{count} ok"
          + (f", {fails} drift/err" if fails else ""))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
