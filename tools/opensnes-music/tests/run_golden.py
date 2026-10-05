#!/usr/bin/env python3
"""Golden-output tests for opensnes-music (Impulse Tracker -> SNESMOD soundbank, the 1.x successor of smconv).

The family's contract: the .asm, .h and .bnk of `bank --name soundbank
--bank 1` are compared against smconv's own goldens (its canonical
`-s -o soundbank -b 1 -n -p soundbank` run), not against copies. The rest
is this tool's: `--json` shapes, `spc`, the refusals. The fixture is
smconv's pollen8.it (PVSnesLib-derived, see ATTRIBUTION.md).

Run:  python3 tools/opensnes-music/tests/run_golden.py
"""
from __future__ import annotations

import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-music", __file__)
SMCONV = g.here.parents[1] / "smconv" / "tests" / "golden"
BANK = ["bank", "-q", "--name", "soundbank", "--bank", "1", "pollen8.it"]

g.expect_outputs("bank pollen8.it --name soundbank --bank 1 == smconv's soundbank.{asm,h,bnk}", BANK,
                 copy=["pollen8.it"], outputs=["soundbank.asm", "soundbank.h", "soundbank.bnk"], want_dir=SMCONV)
g.expect_outputs("bank soundbank.toml (composed asset: inputs from the file) == smconv's goldens", ["bank", "-q", "soundbank.toml"],
                 copy=["pollen8.it"], write={"soundbank.toml": 'tool = "opensnes-music"\n[bank]\ninputs = ["pollen8.it"]\nbank = 1\n'},
                 outputs=["soundbank.asm", "soundbank.h", "soundbank.bnk"], want_dir=SMCONV)
g.expect_outputs("bank --save writes soundbank.toml", ["bank", "-q", "--name", "soundbank", "--save", "pollen8.it"],
                 copy=["pollen8.it"], outputs=["soundbank.toml"])
g.expect_refused("bank empty.toml (no inputs)", ["bank", "empty.toml"], write={"empty.toml": 'tool = "opensnes-music"\n[bank]\nbank = 1\n'},
                 needles=["no `inputs"], nothing_written=["empty.asm"])
g.expect_stdout("bank --json (name from the module)", ["bank", "--json", "pollen8.it"], golden="bank.json", copy=["pollen8.it"])
g.expect_stdout("inspect --json", ["inspect", "--json", "pollen8.it"], golden="inspect.json", copy=["pollen8.it"])


def spc_written():
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(["spc", "-q", "pollen8.it"], work, copy=["pollen8.it"])
        if g.failure(proc):
            return [g.failure(proc)]
        spc = work / "pollen8.spc"
        if not spc.is_file():
            return ["pollen8.spc not produced"]
        data = spc.read_bytes()
        if len(data) != 66048 or not data.startswith(b"SNES-SPC700 Sound File Data"):
            return [f"pollen8.spc: {len(data)} bytes, header {data[:12]!r}"]
    return []


g.check("spc pollen8.it: a 66048-byte SPC file", "SPC700 header, 64 KB RAM + registers", spc_written)
g.expect_refused("bank junk.it (not an IT module)", ["bank", "junk.it"], write={"junk.it": b"junk" * 16},
                 needles=["opensnes-music: junk.it: 'junk.it' is not an Impulse Tracker module"], nothing_written=["junk.asm", "junk.h", "junk.bnk"])
g.expect_refused("bank --bank 0", ["bank", "--bank", "0", "pollen8.it"], copy=["pollen8.it"],
                 needles=["bank 0 holds the code"], nothing_written=["pollen8.asm"])
g.expect_refused("bank --bank x (usage)", ["bank", "--bank", "x", "pollen8.it"], copy=["pollen8.it"], needles=["needs a number"])
sys.exit(g.report())
