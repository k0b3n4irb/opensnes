#!/usr/bin/env python3
"""Golden-output tests for opensnes-sample (WAV -> BRR, the 1.x successor of wav2brr).

The contract of the family (docs/tools/CONVENTIONS.md): a tool that absorbs
a 0.x tool reproduces that tool's golden suite byte for byte. So the .brr
cases compare against wav2brr's own goldens (`tools/wav2brr/tests/golden`),
not against copies. The rest is this tool's: the generated header, the
--json shapes, the settings file beside the asset, the refusals.

Fixture: fixtures/tone.wav, the same synthesized 8-bit mono sine as
wav2brr's (220 samples @ 16 kHz, generated in this repo).

Run:  python3 tools/opensnes-sample/tests/run_golden.py
"""
from __future__ import annotations

import filecmp
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-sample", __file__)
WAV2BRR = g.here.parents[1] / "wav2brr" / "tests" / "golden"   # the absorbed tool's goldens
LOOP = ["--loop", "40", "200"]
SETTINGS = 'tool = "opensnes-sample"\n[encode]\nloop = [40, 200]\n'

# the bytes: wav2brr's goldens, by name for the one-shot, by content for the loop
g.expect_outputs("encode tone.wav (one-shot) == wav2brr tone.brr", ["encode", "-q", "tone.wav"],
                 copy=["tone.wav"], outputs=["tone.brr"], want_dir=WAV2BRR)


def loop_matches(argv, write=None):
    def run():
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            proc = g.run(argv, work, copy=["tone.wav"], write=write)
            if g.failure(proc):
                return [g.failure(proc)]
            if not filecmp.cmp(work / "tone.brr", WAV2BRR / "tone_loop.brr", shallow=False):
                return ["tone.brr differs from wav2brr's tone_loop.brr"]
        return []
    return run


g.check("encode tone.wav --loop 40 200 == wav2brr tone_loop.brr", "same bytes", loop_matches(["encode", "-q", *LOOP, "tone.wav"]))
# the settings file beside the asset drives the run when the option is absent
g.check("encode tone.wav with tone.wav.toml (loop from the settings)", "same bytes as --loop",
        loop_matches(["encode", "-q", "tone.wav"], write={"tone.wav.toml": SETTINGS}))
# this tool's own outputs
g.expect_outputs("encode --loop: the generated header", ["encode", "-q", *LOOP, "tone.wav"],
                 copy=["tone.wav"], outputs=["tone.h"])
g.expect_outputs("encode --save writes the settings beside the input", ["encode", "-q", *LOOP, "--save", "tone.wav"],
                 copy=["tone.wav"], outputs=["tone.wav.toml"])
g.expect_stdout("encode --json", ["encode", "--json", *LOOP, "tone.wav"], golden="encode.json", copy=["tone.wav"])
g.expect_stdout("inspect --json (wav, then its brr)", ["inspect", "--json", "tone.wav", "tone.brr"],
                golden="inspect.json", copy=["tone.wav", "tone.brr"])
# refusals: the limit is named, nothing is written
g.expect_refused("encode --loop 100 50 (out of range)", ["encode", "--loop", "100", "50", "tone.wav"],
                 copy=["tone.wav"], needles=["out of range"], nothing_written=["tone.brr", "tone.h"])
g.expect_refused("encode with an unknown key in tone.wav.toml", ["encode", "tone.wav"], copy=["tone.wav"],
                 write={"tone.wav.toml": 'tool = "opensnes-sample"\n[encode]\nloopx = 1\n'},
                 needles=["unknown key 'loopx'"], nothing_written=["tone.brr", "tone.h"])
g.expect_refused("encode with settings for another tool", ["encode", "tone.wav"], copy=["tone.wav"],
                 write={"tone.wav.toml": 'tool = "opensnes-music"\n[encode]\n'},
                 needles=["for opensnes-music, not opensnes-sample"], nothing_written=["tone.brr"])
g.expect_refused("encode junk.wav (not a WAV)", ["encode", "junk.wav"], write={"junk.wav": b"RIFFxxxxWAVE" + bytes(40)},
                 needles=["no fmt or data chunk"], nothing_written=["junk.brr", "junk.h"])
g.expect_refused("unknown subcommand", ["bogus"], needles=["unknown subcommand 'bogus'"])
g.expect_refused("encode without an input", ["encode"], needles=["needs an input"])
sys.exit(g.report())
