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

Provenance (issue #120): each baseline entry records the sha256 of the ROM
it was captured from. On a mismatch, the report distinguishes "ROM changed —
rebaseline or investigate the build" from "ROM identical but stream changed —
emulator/timing suspect". `--update` refuses to capture from a stale tree
(the corpus-fresh guard, #105, plus a per-example source-mtime check) so a
stale-ROM rebaseline — which shipped a wrong aim_target baseline on
2026-07-17 — fails loudly at capture time instead of in CI. Legacy entries
(bare stream strings) are still readable; a full --update migrates them.

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


# Build-input suffixes for the per-example staleness check. Deliberately
# excludes docs (README.md, screenshot regen) so editing prose never blocks a
# capture; includes generated intermediates (.bin, .pic, .pal, .map) because a
# ROM older than its own inputs is stale by definition.
SOURCE_SUFFIXES = {".c", ".h", ".asm", ".inc", ".s", ".sfx", ".py",
                   ".png", ".bmp", ".pic", ".pal", ".map", ".brr", ".it", ".bin"}


def rom_sha256(rom: Path) -> str:
    return hashlib.sha256(rom.read_bytes()).hexdigest()


def entry_parts(entry) -> tuple[str, str | None]:
    """Return (stream_hash, rom_sha256|None), accepting legacy bare strings."""
    if isinstance(entry, str):
        return entry, None
    return entry["stream"], entry.get("rom_sha256")


def stale_sources(rom: Path) -> list[Path]:
    """Build inputs in the example dir newer than the ROM (empty = fresh)."""
    rom_m = rom.stat().st_mtime
    stale = []
    for f in rom.parent.rglob("*"):
        if (f.is_file() and f.suffix in SOURCE_SUFFIXES
                and f.name != "screenshot.png"  # README artifact, not an input
                and f.stat().st_mtime > rom_m):
            stale.append(f)
    return stale


def corpus_is_fresh() -> bool:
    """Run the corpus-fresh guard (#105) — the make-tests gate, enforced here
    too so a direct --update can't capture from a stale tree."""
    sys.path.insert(0, str(HERE.parent.parent / "devtools"))
    import check_corpus_fresh  # noqa: E402
    return check_corpus_fresh.main() == 0


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
    ap.add_argument("--force-stale", action="store_true",
                    help="capture from a stale tree anyway (debugging only — "
                         "NEVER commit a baseline captured with this)")
    args = ap.parse_args()
    luna = find_luna()
    db = json.loads(BASELINE.read_text()) if BASELINE.is_file() else {}

    if args.update and not args.force_stale and not corpus_is_fresh():
        print("REFUSED: --update from a stale tree writes wrong baselines "
              "(#120). Rebuild first, or --force-stale for local debugging.")
        return 1

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
            newer = stale_sources(rom)
            if newer and not args.force_stale:
                print(f"  REFUSED {label}: ROM older than "
                      f"{newer[0].relative_to(rom.parent)}"
                      + (f" (+{len(newer) - 1} more)" if len(newer) > 1 else "")
                      + " — rebuild before capturing (#120)")
                fails += 1
                continue
            db[label] = {"stream": h, "rom_sha256": rom_sha256(rom)}
            updated += 1
            print(f"  BASELINE {label}  {h[:16]}…")
        elif label not in db:
            print(f"  MISS  {label}: no baseline — run --update first")
            fails += 1
        else:
            base_h, base_rom = entry_parts(db[label])
            if h == base_h:
                print(f"  PASS  {label}")
            else:
                if base_rom is None:
                    why = "no ROM provenance recorded (legacy entry)"
                elif rom_sha256(rom) == base_rom:
                    why = ("ROM IDENTICAL to baseline capture — emulator/"
                           "timing suspect (luna version change?)")
                else:
                    why = ("ROM bytes differ from baseline capture — "
                           "if intentional, rebuild clean then --update")
                print(f"  FAIL  {label}: WRAM-state stream changed "
                      f"({h[:16]}… != {base_h[:16]}…; {why})")
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
