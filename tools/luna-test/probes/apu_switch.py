"""Probe: APU hot-swap (apu_switch) — input-driven program switching.

Assertions on examples/audio/apu_switch:
1. boot (no input): current_song == 0 (drums) and audio is non-silent;
2. press B: current_song flips to 1 (apuReset -> cello upload completed —
   the whole cooperative reset handshake ran, or the blocking upload would
   never have returned).

3. cello->drums direction: press B then A -> current_song flips back to 0
   (main.c selects a specific song per button; A = drums, B = cello).

History: this direction assert was disabled while `luna state --input`
misapplied checkpoint frames (luna#126 — first mask latched at boot, list
re-fired periodically). luna#126 is fixed as of v1.13.0 (verified with a
frame-exact repro; checkpoints now land on their scheduled frame and fire
once), so the B->A direction is asserted again here.
"""
from __future__ import annotations

import subprocess
import sys

from lib import find_luna, rom_path, peek, A, B

ROM_REL = "audio/apu_switch/apu_switch.sfc"
STEPS = 12_000_000  # ~5 s: past boot + at least one swap


def _song(luna: str, rom, script: str | None) -> int:
    return peek(luna, rom, STEPS, "current_song", 1, input_script=script)[0]


def _rms(luna: str, rom) -> float:
    from pathlib import Path
    import wave, array
    out = Path("/tmp/luna-audio") / "apu_switch_probe.wav"
    out.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([luna, "run", "-n", str(STEPS), "--audio-out", str(out), str(rom)],
                   capture_output=True, text=True, timeout=300)
    with wave.open(str(out), "rb") as w:
        frames = w.readframes(w.getnframes())
    if not frames:
        return 0.0
    a = array.array("h")
    a.frombytes(frames)
    return (sum(s * s for s in a) / len(a)) ** 0.5


def run() -> tuple[bool, str]:
    luna = find_luna()
    rom = rom_path(ROM_REL)

    boot = _song(luna, rom, None)
    rms = _rms(luna, rom)
    after_b = _song(luna, rom, f"60:{B:#x},63:0")
    after_ba = _song(luna, rom, f"60:{B:#x},63:0,120:{A:#x},123:0")

    checks = [
        (boot == 0, f"boot song={boot} (want 0)"),
        (rms > 100.0, f"drums RMS={rms:.0f} (want >100)"),
        (after_b == 1, f"after B song={after_b} (want 1)"),
        (after_ba == 0, f"after B+A song={after_ba} (want 0)"),
    ]
    bad = [m for ok, m in checks if not ok]
    if bad:
        return False, "FAILED: " + "; ".join(bad)
    return True, "; ".join(m for _, m in checks)


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "apu_switch: " + msg)
    sys.exit(0 if ok else 1)
