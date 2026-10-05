#!/usr/bin/env python3
"""Golden-output tests for wav2brr (WAV -> SNES BRR converter).

The fixture WAV in one-shot and looping modes, byte-compared against the
committed .brr. The BRR encoder is deterministic (no RNG, fixed
amplitude-backoff schedule), so any diff is a real behaviour change.

Fixture provenance: fixtures/tone.wav is a synthesized 8-bit mono sine
(220 samples @ 16 kHz), generated deterministically by this repo — not a
third-party asset (the snippet is in this file's git history).

Run:  python3 tools/wav2brr/tests/run_golden.py
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("wav2brr", __file__)
# (golden name, extra CLI args) — the one-shot and the looping path
for name, extra in [("tone.brr", []), ("tone_loop.brr", ["--loop", "40", "200"])]:
    g.expect_outputs(f"tone.wav [{' '.join(extra) or 'one-shot'}] -> {name}",
                     [*extra, "tone.wav", name], copy=["tone.wav"], outputs=[name])
sys.exit(g.report())
