#!/usr/bin/env python3
"""Golden-output tests for wav2brr (WAV -> SNES BRR converter).

Runs the built tool on a fixture WAV in both one-shot and looping modes
and byte-compares the produced .brr against committed goldens. The BRR
encoder is deterministic (no RNG, fixed amplitude-backoff schedule), so
any diff is a real behaviour change: a regression, or an intentional
change that must be re-goldened by re-copying the output here.

Fixture provenance: tests/fixtures/tone.wav is a synthesized 8-bit mono
sine (220 samples @ 16 kHz), generated deterministically by this repo -
not a third-party asset. Regenerate with the snippet in the git history
of this file if ever needed.

Run:  python3 tools/wav2brr/tests/run_golden.py
"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TOOL = REPO / "bin" / "wav2brr"

FIXTURE = HERE / "fixtures" / "tone.wav"
GOLDEN = HERE / "golden"

# (golden-name, extra CLI args) — one-shot and looping paths.
CASES = [
    ("tone.brr", []),
    ("tone_loop.brr", ["--loop", "40", "200"]),
]


def main() -> int:
    if not TOOL.is_file():
        sys.exit(f"ERROR: {TOOL} not found — run `make tools` first")

    errs = []
    with tempfile.TemporaryDirectory() as td:
        for name, extra in CASES:
            out = Path(td) / name
            proc = subprocess.run(
                [str(TOOL), *extra, str(FIXTURE), str(out)],
                capture_output=True, text=True, timeout=60,
            )
            if proc.returncode != 0:
                errs.append(f"{name}: exit {proc.returncode}: "
                            f"{(proc.stderr or proc.stdout).strip()[:200]}")
                continue
            got = out.read_bytes()
            want = (GOLDEN / name).read_bytes()
            if got != want:
                errs.append(f"{name}: {len(got)} bytes differ from golden "
                            f"({len(want)} bytes)")

    if errs:
        print("wav2brr golden tests FAILED:")
        for e in errs:
            print(f"  - {e}")
        return 1
    print(f"wav2brr golden tests OK ({len(CASES)} cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
