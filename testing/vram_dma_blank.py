#!/usr/bin/env python3
"""VRAM DMA only in blank, and double-buffered frames only whole.

The PPU silently drops a VRAM write made during active display (CLAUDE.md,
the first of the silent failures): no error, a missing byte. luna tags every
byte an MDMA writes to `$2118/$2119` with V-blank and INIDISP force blank; a
byte is safe iff one of them is set.

Two checks:

1. **Every example** (the corpus, `--frames` frames): luna's own
   `[asserts.dma] unsafe_writes = 0`, one generated manifest per example,
   run by `luna test --jobs 0` — luna counts, this script only writes the
   question. (Until luna v1.30.2 the assert stopped at 1 000 000 trace
   events and the Super FX examples were run over 55 frames.)
2. **Presented Super FX frames** (`PRESENT` below, gsuPresent — superfx
   runtime chantier, phase D): the framebuffer moves as whole frames,
   into alternating VRAM blocks, each byte at the same offset it had in Game
   Pak RAM; and every BG12NBA write (luna `--trace-writes 210B`) is made in
   blank and shows a block that has just received exactly one whole frame —
   never a half-landed one.

Run:  python3 testing/vram_dma_blank.py            # gate (make tests)
      python3 testing/vram_dma_blank.py --only chips/superfx_game_skeleton
"""
from __future__ import annotations

import argparse
import csv
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from lib import discover_example_roms, example_key, find_luna  # noqa: E402

FRAMES = 200

# Examples that present Super FX frames with gsuPresent: (key, frame bytes,
# first frame to look at — after boot, when frames flow).
PRESENT = [
    ("chips/superfx_game_skeleton", 16384, 100),
]


def _hex(text: str) -> int:
    return int(text.replace("$", "").replace(":", ""), 16)


def dma_rows(luna: str, rom: Path, frames: int, extra: list[str], tmp: Path) -> list[dict]:
    trace = tmp / "dma.csv"
    cmd = [luna, "state", "--until-frame", str(frames), "--dma-trace", str(trace),
           "--dma-trace-max", "4000000", "--out", "/dev/null" if sys.platform != "win32" else "NUL",
           *extra, str(rom)]
    subprocess.run(cmd, check=True, capture_output=True, text=True)
    with trace.open(encoding="utf-8") as f:
        return list(csv.DictReader(f))


def outside_blank(rows: list[dict]) -> list[dict]:
    return [r for r in rows if r["blank"] != "1" and r["force_blank"] != "1"]


def check_blank(luna: str, roms: list[Path], frames: int) -> list[str]:
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        for rom in roms:
            name = example_key(rom).replace("/", "__")
            (tmp / f"{name}.toml").write_text(
                f'rom = "{rom.as_posix()}"\nframes = {frames}\n'
                "[asserts.dma]\nunsafe_writes = 0\n", encoding="utf-8")
        run = subprocess.run([luna, "test", "--jobs", "0", str(tmp)],
                             capture_output=True, text=True)
    out = run.stdout.splitlines()
    fails, block = [], None
    for line in out:
        if line.startswith("FAIL "):
            block = [line[5:].replace("__", "/")]
            fails.append(block)
        elif block is not None and line.startswith(" "):
            block.append(line.strip())
        else:
            block = None
    passed = sum(1 for line in out if line.startswith("PASS "))
    if run.returncode not in (0, 1) or passed + len(fails) != len(roms):
        sys.exit(f"vram-dma-blank: luna test failed to run ({run.returncode}): "
                 f"{(run.stderr or run.stdout)[-400:]}")
    return [" — ".join(b) for b in fails]


def check_present(luna: str, rom: Path, size: int, first: int) -> list[str]:
    """Whole frames, alternating blocks, offsets kept; swaps in blank after
    exactly one whole frame into the block they show."""
    problems: list[str] = []
    frames = first + 60
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        writes = tmp / "writes.csv"
        rows = dma_rows(luna, rom, frames,
                        ["--mem-trace", str(writes), "--trace-writes", "210B",
                         "--mem-trace-max", "100000"], tmp)
        with writes.open(encoding="utf-8") as f:
            swaps = [r for r in csv.DictReader(f)
                     if r["kind"] == "W" and r["addr"].upper().endswith("210B")]
    rows = [r for r in rows if int(r["frame"]) >= first]
    swaps = [s for s in swaps if int(s["frame_ntsc"]) >= first]
    if outside_blank(rows):
        problems.append(f"{len(outside_blank(rows))} framebuffer bytes outside blank")

    # runs: consecutive bytes into one VRAM block at consecutive offsets
    runs: list[list] = []   # [vram block, cart block, bytes, next offset]
    for r in rows:
        src = _hex(r["src"])
        dst = _hex(r["vram_word"]) * 2 + (1 if r["reg"] == "$19" else 0)
        off = dst % size
        if off != (src & 0xFFFF) % size:
            problems.append(f"byte from {r['src']} landed at VRAM byte {dst:#x}: offset moved")
            break
        vblk, sblk = dst - off, src - (src & 0xFFFF) % size
        if runs and runs[-1][0] == vblk and runs[-1][3] == off:
            runs[-1][2] += 1
            runs[-1][3] = off + 1
        else:
            runs.append([vblk, sblk, 1, off + 1])
    whole = runs[1:-1]  # the first and last may be cut by the window
    if not whole:
        problems.append("no whole frame in the window")
    for run in whole:
        if run[2] != size:
            problems.append(f"a frame of {run[2]} bytes into VRAM block {run[0]:#x}, not {size}")
            break
    for a, b in zip(whole, whole[1:]):
        if a[0] == b[0]:
            problems.append(f"two frames in a row into VRAM block {a[0]:#x}: no double buffer")
            break

    # every swap in blank, after exactly one whole frame into the shown block
    for prev, cur in zip(swaps, swaps[1:]):
        if cur["blank"] != "1" and cur["force_blank"] != "1":
            problems.append(f"BG12NBA written on a visible line (frame {cur['frame_ntsc']} line {cur['line']})")
            break
        lo = (int(prev["frame_ntsc"]), int(prev["line"]))
        hi = (int(cur["frame_ntsc"]), int(cur["line"]))
        shown = (_hex(cur["value"]) & 0x0F) * 0x2000   # BG1 char base, 8 KB steps
        into = [r for r in rows if lo < (int(r["frame"]), int(r["line"])) <= hi]
        n_shown = sum(1 for r in into if (_hex(r["vram_word"]) * 2) // size * size == shown)
        if n_shown != size or len(into) != n_shown:
            problems.append(f"swap at frame {cur['frame_ntsc']} line {cur['line']} shows block "
                            f"{shown:#x} after {n_shown} bytes into it and "
                            f"{len(into) - n_shown} elsewhere (want {size} and 0)")
            break
    if len(swaps) < 10:
        problems.append(f"only {len(swaps)} BG12NBA writes in 60 frames")
    return problems


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--only", help="one example key (e.g. chips/superfx_game_skeleton)")
    ap.add_argument("--frames", type=int, default=FRAMES)
    args = ap.parse_args()
    luna = find_luna()

    roms = discover_example_roms()
    if args.only:
        roms = [r for r in roms if example_key(r) == args.only]
        if not roms:
            sys.exit(f"vram-dma-blank: no built example {args.only}")
    blank_fails = check_blank(luna, roms, args.frames)
    for f in blank_fails:
        print(f"  FAIL  {f}")
    fails = len(blank_fails)
    print(f"VRAM DMA in blank: {len(roms) - fails}/{len(roms)} examples clean over "
          f"{args.frames} frames ([asserts.dma] unsafe_writes = 0)")

    for key, size, first in PRESENT:
        if args.only and args.only != key:
            continue
        rom = next((r for r in roms if example_key(r) == key), None)
        if rom is None:
            sys.exit(f"vram-dma-blank: {key} not built")
        problems = check_present(luna, rom, size, first)
        for p in problems:
            print(f"  FAIL  {key} (gsuPresent): {p}")
        fails += bool(problems)
        if not problems:
            print(f"gsuPresent {key}: whole {size}-byte frames, alternating blocks, swaps in blank")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
