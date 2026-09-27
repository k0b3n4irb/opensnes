#!/usr/bin/env python3
"""Audio regression by WAV hash (gaps review R6).

luna captures the APU's 32 kHz stereo output with `luna run --audio-out`;
this script hashes the samples of a few audio examples and compares them
with `baselines/audio.json`. It computes nothing about the sound — no
pitch, no envelope, no FFT (the earlier analysis prototype was dropped for
being unreliable, see .claude/rules/luna_tooling.md): a changed hash means
"the APU output changed, go listen", exactly like fbhash means "the frame
changed, go look". The capture is deterministic run to run (same seed of
nothing: the SPC700/DSP are integer machines).

Examples covered: the seven that play without input (apu_switch, snesmod_music,
pitch_mod, play_noise, echo, speech_synth, snesmod_music_large) and the three
that need a button (soundboard, snesmod_sfx, sfx_from_wav), each pressed with
the script of its luna test manifest. The button-driven ones joined on luna
v1.29.0: before it, `--audio-out` under `--input` kept only 16 384 samples
from the boot and then jumped to the last checkpoint.

Run:  python3 tools/luna-test/audio_regress.py            # compare
      python3 tools/luna-test/audio_regress.py --update   # re-capture after an intended change
Exit 0 = every hash matches, 1 = any mismatch or missing ROM.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from luna_runner import REPO_ROOT, LUNA_VERSION, find_luna  # noqa: E402

BASELINE = HERE / "baselines" / "audio.json"
FRAMES = 300  # 5 s NTSC — past every driver's boot + sample upload

# example key -> ROM path (relative to examples/)
EXAMPLES = {
    "audio/apu_switch":    ("audio/apu_switch/apu_switch.sfc", None),
    "audio/snesmod_music": ("audio/snesmod_music/music.sfc", None),
    "audio/pitch_mod":     ("audio/pitch_mod/pitch_mod.sfc", None),
    "audio/play_noise":    ("audio/play_noise/play_noise.sfc", None),
    "audio/echo":          ("audio/echo/echo.sfc", None),
    "audio/speech_synth":  ("audio/speech_synth/speech_synth.sfc", None),
    "audio/snesmod_music_large": ("audio/snesmod_music_large/music_large.sfc", None),
    # joypad-1 scripts from their manifests (soundboard.toml,
    # audio_snesmod_sfx_play.toml, audio_sfx_from_wav.toml)
    "audio/soundboard":    ("audio/soundboard/soundboard.sfc", "220:0x0080"),
    "audio/snesmod_sfx":   ("audio/snesmod_sfx/sfx.sfc",
                            "40:0x0100,44:0,50:0x0100,54:0,80:0x0080,84:0"),
    "audio/sfx_from_wav":  ("audio/sfx_from_wav/sfx_from_wav.sfc",
                            "100:0x0080,104:0,130:0x8000,134:0"),
}


def capture(luna: str, rom: Path, script: str | None = None) -> dict:
    """Run the ROM for FRAMES frames (pressing `script`, if any); return
    {sha256 of the PCM, sample count, peak}."""
    with tempfile.TemporaryDirectory() as td:
        out = Path(td) / "out.wav"
        if script is None:
            cmd = [luna, "run", "--until-frame", str(FRAMES), "--audio-out", str(out), str(rom)]
        else:
            cmd = [luna, "state", "--until-frame", str(FRAMES), "--input", script,
                   "--audio-out", str(out), "--out", os.devnull, str(rom)]
        subprocess.run(cmd, capture_output=True, text=True, timeout=300, check=True)
        with wave.open(str(out)) as w:
            n = w.getnframes()
            pcm = w.readframes(n)
    peak = max((abs(int.from_bytes(pcm[i:i + 2], "little", signed=True))
                for i in range(0, len(pcm), 2)), default=0)
    return {"sha256": hashlib.sha256(pcm).hexdigest(), "samples": n, "peak": peak}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--update", action="store_true", help="rewrite baselines/audio.json")
    args = ap.parse_args()
    luna = find_luna()
    got = {}
    for key, (rel, script) in EXAMPLES.items():
        rom = REPO_ROOT / "examples" / rel
        if not rom.is_file():
            print(f"  MISSING {key} ({rel}) — build the examples first")
            return 1
        got[key] = capture(luna, rom, script)
    if args.update:
        BASELINE.write_text(json.dumps({"luna": LUNA_VERSION, "frames": FRAMES, "examples": got},
                                       indent=2) + "\n", encoding="utf-8")
        print(f"wrote {BASELINE.relative_to(REPO_ROOT)} ({len(got)} examples)")
        return 0
    if not BASELINE.is_file():
        print(f"no baseline: run with --update first ({BASELINE.relative_to(REPO_ROOT)})")
        return 1
    want = json.loads(BASELINE.read_text(encoding="utf-8"))["examples"]
    fails = 0
    for key, cur in got.items():
        ref = want.get(key)
        if ref is None:
            print(f"  NEW   {key}  sha256={cur['sha256'][:16]} (not in baseline — --update)")
            fails += 1
        elif ref["sha256"] == cur["sha256"]:
            print(f"  MATCH {key}  ({cur['samples']} samples, peak {cur['peak']})")
        else:
            print(f"  DIFF  {key}  sha256 {ref['sha256'][:16]} -> {cur['sha256'][:16]}, "
                  f"peak {ref['peak']} -> {cur['peak']}  (listen with `luna run --audio-out`; "
                  f"--update once explained)")
            fails += 1
    silent = [k for k, c in got.items() if c["peak"] == 0]
    for k in silent:
        print(f"  SILENT {k} — the capture holds no sound; a hash of silence guards nothing")
    fails += len(silent)
    print(f"\nAudio regression: {len(got) - fails}/{len(got)} match")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
