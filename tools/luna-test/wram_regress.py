#!/usr/bin/env python3
"""WRAM-state regression (H7) — a per-frame state oracle stronger than fbhash.

For each example, `luna wram-trace` emits a vblank-aligned FNV-1a hash of every
WRAM page for N consecutive frames. We SHA-256 that whole stream into one key and
compare it against a committed baseline. This catches runtime-state regressions
that never reach the screen (an uninitialised read, a mis-stepped counter, a
changed allocation) — which the framebuffer fbhash can't see.

CI gate with a cross-arch exclusion list. Unlike the framebuffer (luna
guarantees `--print-fbhash` cross-arch), raw WRAM content is *not* a cross-arch
guarantee: most examples hash identically across hosts, but two (mapandobjects,
slopemario) diverge x86_64 ↔ aarch64. Those two are skipped by default
(CROSS_ARCH_EXCLUDE below) so the remaining 54 gate CI on both arches; pass
`--all` on your own machine to cover them too against a same-arch baseline.

  make test-wram                                    # compare vs baseline (54/56)
  python3 tools/luna-test/wram_regress.py --all     # incl. arch-fragile pair
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

# The only two examples whose WRAM stream hashes differently x86_64 ↔ aarch64
# (long interactive game loops; divergence root cause untracked). Skipped by
# default so the other 54 can gate CI on both arches; include with --all when
# running against a baseline captured on your own arch.
CROSS_ARCH_EXCLUDE = {"games_mapandobjects", "maps_slopemario"}


def stream_hash(luna: str, rom: Path) -> str:
    out = Path("/tmp/luna-wram") / f"{example_key(rom).replace('/', '_')}.txt"
    out.parent.mkdir(parents=True, exist_ok=True)
    # Unlink first: a stale file from a previous run would pass the is_file()
    # check below and get hashed if wram-trace fails silently — which poisons
    # the baseline with an outdated stream instead of surfacing the failure.
    out.unlink(missing_ok=True)
    proc = subprocess.run(
        [luna, "wram-trace", "-n", "0", "-c", str(FRAMES), "--out", str(out), str(rom)],
        capture_output=True, text=True, timeout=300,
    )
    if proc.returncode != 0 or not out.is_file() or out.stat().st_size == 0:
        raise RuntimeError(f"wram-trace failed for {rom.name} "
                           f"(exit {proc.returncode}): {proc.stderr.strip()[:200]}")
    return hashlib.sha256(out.read_bytes()).hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser(description="WRAM-state regression (H7)")
    ap.add_argument("--update", action="store_true", help="(re)write the baseline")
    ap.add_argument("--only", metavar="SUBSTR")
    ap.add_argument("--all", action="store_true",
                    help="include the arch-fragile pair (same-arch baseline only)")
    args = ap.parse_args()
    luna = find_luna()
    db = json.loads(BASELINE.read_text()) if BASELINE.is_file() else {}

    fails = updated = count = skipped = 0
    for rom in discover_example_roms():
        label = example_key(rom).replace("/", "_")
        if args.only and args.only not in label:
            continue
        # --update always refreshes the full set (incl. the fragile pair, so a
        # same-arch --all run has a current baseline); only compares skip them.
        if (label in CROSS_ARCH_EXCLUDE and not args.all and not args.only
                and not args.update):
            skipped += 1
            print(f"  SKIP  {label} (cross-arch-fragile — use --all on a same-arch baseline)")
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
          + (f", {fails} drift/err" if fails else "")
          + (f", {skipped} skipped (cross-arch)" if skipped else ""))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
