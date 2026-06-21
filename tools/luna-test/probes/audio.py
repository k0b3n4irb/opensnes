"""Probe: audio output (H5) — the harness has never tested sound.

For the SNESMOD music examples, assert the SPC700/S-DSP is actually producing
audio: ≥1 active voice (`state.apu.active_voices`) AND non-silent PCM (RMS of
luna's `--audio-out` 32 kHz stereo WAV). For the SFX example, the driver must be
alive (`spc_stopped == false`) — it only keys voices on a trigger.
"""
from __future__ import annotations

import json
import subprocess
import sys
import wave
from pathlib import Path

from lib import find_luna, rom_path

MUSIC = [
    "audio/snesmod_music/music.sfc",
    "audio/snesmod_music_hirom/music_hirom.sfc",
    "audio/snesmod_music_large/music_large.sfc",
]
SFX = "audio/snesmod_sfx/sfx.sfc"
STEPS = 5_000_000  # past the driver upload + first notes


def _apu(luna: str, rom: Path) -> dict:
    p = subprocess.run([luna, "state", "-n", str(STEPS), "--out", "-", str(rom)],
                       capture_output=True, text=True, timeout=300)
    return json.loads(p.stdout)["apu"]


def _wav_rms(luna: str, rom: Path) -> float:
    out = Path("/tmp/luna-audio") / (rom.stem + ".wav")
    out.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([luna, "run", "-n", str(STEPS), "--audio-out", str(out), str(rom)],
                   capture_output=True, text=True, timeout=300)
    with wave.open(str(out), "rb") as w:
        frames = w.readframes(w.getnframes())
    if not frames:
        return 0.0
    # 16-bit samples; mean square without numpy
    import array
    a = array.array("h")
    a.frombytes(frames)
    return (sum(s * s for s in a) / len(a)) ** 0.5


def run() -> tuple[bool, str]:
    luna = find_luna()
    results = []
    for rel in MUSIC:
        rom = rom_path(rel)
        apu = _apu(luna, rom)
        voices = apu.get("active_voices", 0)
        rms = _wav_rms(luna, rom)
        ok = voices >= 1 and rms > 100.0
        results.append((ok, f"{rom.parent.name}: {voices} voices, RMS={rms:.0f}"))
    # SFX: driver alive (voices key on trigger, not at boot)
    rom = rom_path(SFX)
    apu = _apu(luna, rom)
    alive = not apu.get("spc_stopped", True)
    results.append((alive, f"snesmod_sfx: SPC driver {'alive' if alive else 'STOPPED'}"))

    bad = [m for ok, m in results if not ok]
    if bad:
        return False, "FAILED: " + "; ".join(bad)
    return True, "; ".join(m for _, m in results)


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "audio: " + msg)
    sys.exit(0 if ok else 1)
