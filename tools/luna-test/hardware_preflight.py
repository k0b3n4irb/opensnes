#!/usr/bin/env python3
"""Preflight of the real-console protocol ROMs on luna (2026-10-03).

A console does not power on with zeroed RAM, and a PAL console is not the
NTSC one every baseline was captured on: those are the two cheapest ways a
ROM that is green on luna's defaults can fail on the hardware kit. Before a
console session (`docs/HARDWARE_VERIFICATION.md`, `make hardware-kit`), this
script reads the protocol's own table — the same lines `scripts/hardware-kit.sh`
reads, so the kit and the preflight cannot drift apart — and replays each
ROM:

  - alive at its manifest's latest capture point from pseudo-random RAM, one
    run per seed (`luna state --power-on random=<seed>`; the default seeds
    are 1, 2, 3 — `--seeds` changes them);
  - alive under `--force-region pal`;
  - every VRAM DMA byte in blank or force blank (`[asserts.dma]
    unsafe_writes = 0`, the oracle of vram_dma_blank.py).

Liveness is luna_runner.py's: NMI advancing, CPU not halted. A row whose
example needs device input luna cannot drive (manifest `input_dependent`) is
checked for boot and liveness only, like the coverage pass. A firmware-gated
row (DSP-1) is SKIPPED when the firmware is not installed, like everywhere
else. Nothing is compared to a baseline: the question is only "does it run".

Run:  python3 tools/luna-test/hardware_preflight.py            # all rows
      python3 tools/luna-test/hardware_preflight.py --rows 1-7  # the gate rows
      make hardware-preflight
Exit 0 = every checked row passed, 1 = any failure or missing ROM.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import luna_runner as lr  # noqa: E402
from vram_dma_blank import check_blank  # noqa: E402

PROTOCOL = lr.REPO_ROOT / "docs" / "HARDWARE_VERIFICATION.md"
ROW_RE = re.compile(r"^\| (\d+) \| `([a-z0-9_/]+)` \|")


def protocol_rows() -> list[tuple[int, str]]:
    """(row number, example key) for every ROM row of the protocol's table —
    the regex is scripts/hardware-kit.sh's, so both read the same lines."""
    rows = []
    for line in PROTOCOL.read_text(encoding="utf-8").splitlines():
        m = ROW_RE.match(line)
        if m:
            rows.append((int(m.group(1)), m.group(2)))
    return rows


def parse_rows(spec: str | None, rows: list[tuple[int, str]]) -> list[tuple[int, str]]:
    if not spec:
        return rows
    wanted: set[int] = set()
    for part in spec.split(","):
        a, _, b = part.partition("-")
        lo, hi = int(a), int(b or a)
        wanted.update(range(lo, hi + 1))
    return [r for r in rows if r[0] in wanted]


def alive(luna: str, rom: Path, frame: int, extra: list[str]) -> str | None:
    """None when the ROM is live at `frame`, else the reason."""
    try:
        state = lr.render_state(luna, rom, frame, None, extra=extra)
    except Exception as e:  # noqa: BLE001 — a luna error is a failure, not a crash
        return str(e)[:120]
    live, why = lr.liveness(state)
    return None if live else why


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--rows", help="rows to check, e.g. 1-7 or 1,3,16-18 (default: all)")
    ap.add_argument("--seeds", default="1,2,3",
                    help="random power-on seeds, comma-separated (default 1,2,3)")
    ap.add_argument("--frames", type=int, default=200,
                    help="frames for the VRAM-DMA-in-blank check (default 200)")
    args = ap.parse_args()
    seeds = [int(s) for s in args.seeds.split(",") if s]
    luna = lr.find_luna()
    manifest = lr.load_manifest()
    roms = {lr.example_key(r): r for r in lr.discover_example_roms()}
    rows = parse_rows(args.rows, protocol_rows())
    if not rows:
        sys.exit("hardware-preflight: no protocol row matched")

    fails = 0
    skipped = 0
    checked_roms: list[Path] = []
    for num, key in rows:
        rom = roms.get(key)
        if rom is None:
            print(f"  FAIL  {num:2} {key}: no built ROM (make examples)")
            fails += 1
            continue
        fw = lr.missing_firmware(key, manifest)
        if fw:
            print(f"  SKIP  {num:2} {key}: firmware {fw} not installed")
            skipped += 1
            continue
        frame = max(lr.capture_frames(key, manifest))
        extra = lr.res_args(key, manifest)
        problems = []
        for seed in seeds:
            why = alive(luna, rom, frame, extra + ["--power-on", f"random={seed}"])
            if why:
                problems.append(f"random={seed}: {why}")
        why = alive(luna, rom, frame, extra + ["--force-region", "pal"])
        if why:
            problems.append(f"pal: {why}")
        tag = "INPUT-DEP" if manifest["examples"].get(key, {}).get("input_dependent") else "OK"
        if problems:
            fails += 1
            print(f"  FAIL  {num:2} {key}: " + "; ".join(problems))
        else:
            print(f"  {tag:9} {num:2} {key}  (frame {frame}, seeds {args.seeds}, pal)")
        checked_roms.append(rom)

    if checked_roms:
        for f in check_blank(luna, checked_roms, args.frames):
            print(f"  FAIL  VRAM DMA outside blank: {f}")
            fails += 1

    print(f"\nHardware preflight: {len(rows) - fails - skipped} of {len(rows)} rows ready"
          f"{f', {skipped} skipped (firmware)' if skipped else ''}"
          f"{f', {fails} FAILED' if fails else ''}.")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
