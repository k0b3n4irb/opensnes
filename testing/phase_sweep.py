#!/usr/bin/env python3
"""Replay input manifests at sixteen press phases (testing audit 2026-10-03, T4).

A race between a button press and the SPC700 shows up only on some frames:
the SNESMOD key-off defect of 2026-09-26 (`8d83859f`) failed 8 to 10 of the
161 press frames swept by hand, and a manifest that presses at one frame sees
it or not by luck. The sweep did not stay in the suite. This script keeps it:
for each manifest named below it writes sixteen copies whose every input
checkpoint is shifted by 0..15 frames (the asserted frame too) and runs them
through `luna test --jobs 0`. Sixteen consecutive phases cover every
alignment of a 60 Hz press against the driver's per-frame cycle.

It drives luna and reads its exit status; it computes nothing itself
(`.claude/rules/luna_tooling.md`).

Run:  python3 testing/phase_sweep.py            # in `make test-manifests`
      python3 testing/phase_sweep.py --phases 4  # quicker, for a local check
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from lib import find_luna  # noqa: E402

MANIFESTS = HERE / "manifests"
# The three SNESMOD transitions whose race the 2026-09-26 sweep exposed.
SWEPT = ["audio_snesmod_music_stop.toml",
         "audio_snesmod_music_pause.toml",
         "audio_snesmod_music_fade.toml"]
INPUT_RE = re.compile(r'^(input\s*=\s*")([^"]*)(")', re.M)
ROM_RE = re.compile(r'^(rom\s*=\s*")([^"]*)(")', re.M)
FRAMES_RE = re.compile(r"^(frames\s*=\s*)(\d+)", re.M)


def shifted(text: str, src: Path, k: int) -> str:
    """The manifest with every input checkpoint, and the asserted frame, k frames later."""
    def shift_input(m: re.Match) -> str:
        parts = []
        for cp in m.group(2).split(","):
            frame, mask = cp.split(":")
            parts.append(f"{int(frame) + k}:{mask}")
        return m.group(1) + ",".join(parts) + m.group(3)
    text, n = INPUT_RE.subn(shift_input, text)
    if n != 1:
        sys.exit(f"phase-sweep: {src.name} has no single `input = ` line")
    text = FRAMES_RE.sub(lambda m: f"{m.group(1)}{int(m.group(2)) + k}", text, count=1)
    # rom paths are relative to the manifest's directory: make them absolute
    text = ROM_RE.sub(lambda m: m.group(1) + str((src.parent / m.group(2)).resolve()) + m.group(3), text)
    return text


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--phases", type=int, default=16, help="press offsets 0..N-1 (default 16)")
    ap.add_argument("--manifest", action="append", help="sweep this manifest instead of the SNESMOD three")
    args = ap.parse_args()
    luna = find_luna()
    names = args.manifest or SWEPT
    with tempfile.TemporaryDirectory() as td:
        out = Path(td)
        for name in names:
            src = MANIFESTS / name
            if not src.is_file():
                sys.exit(f"phase-sweep: {src} missing")
            text = src.read_text(encoding="utf-8")
            for k in range(args.phases):
                (out / f"{src.stem}_phase{k:02}.toml").write_text(shifted(text, src, k), encoding="utf-8")
        proc = subprocess.run([luna, "test", "--jobs", "0", str(out)],
                              capture_output=True, text=True, timeout=1800)
        failed = [l for l in proc.stdout.splitlines() if l.strip().startswith("FAIL")]
        for l in failed:
            print("  " + l.strip())
        total = len(names) * args.phases
        print(f"Phase sweep: {total - len(failed)}/{total} ok "
              f"({len(names)} manifests x {args.phases} press phases)"
              + (f", {len(failed)} FAIL" if failed else ""))
        if proc.returncode and not failed:
            print(proc.stdout[-800:], proc.stderr[-800:])
        return 1 if (proc.returncode or failed) else 0


if __name__ == "__main__":
    sys.exit(main())
