#!/usr/bin/env python3
"""Golden tests for opensnes-save (battery save files, .srm).

No ancestor to reproduce: the reference is what luna writes. slot1.srm is
the 8 KB battery file luna's `srm_out` produced for examples/memory/save_game
after "save slot 1" (testing/manifests/sram_save.srm: 09 00 0A 00 34 12 78 56
at offset 0); `new` on a ROM that declares 8 KB then `set` of those eight
bytes must give that file. The ROM fixtures are synthetic headers (title,
map mode, cartridge type, $FFD8, $FFBD, checksum pair) in an empty image:
LoROM with 8 KB, HiROM with 2 KB, a Super FX cartridge whose save is its
64 KB of expansion RAM, a ROM without save RAM, and a file that is no ROM.

Run:  python3 tools/opensnes-save/tests/run_golden.py
"""
from __future__ import annotations

import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-save", __file__)
SLOT1 = "09 00 0A 00 34 12 78 56"


def new_then_set():
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(["new", "-q", "lorom.sfc"], work, copy=["lorom.sfc", "slot1.srm"])
        if g.failure(proc):
            return [g.failure(proc)]
        proc = g.run(["set", "-q", "lorom.srm", "--at", "0", "--hex", SLOT1], work)
        if g.failure(proc):
            return [g.failure(proc)]
        proc = g.run(["diff", "lorom.srm", "slot1.srm"], work)
        return [] if proc.returncode == 0 else [f"differs from luna's save: {g.said(proc).strip()[:120]}"]


def sizes():
    want = {"lorom": 8192, "hirom": 2048, "gsu": 65536}
    errs = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(["new", "-q", "--fill", "255", "lorom.sfc", "hirom.sfc", "gsu.sfc"], work, copy=["lorom.sfc", "hirom.sfc", "gsu.sfc"])
        if g.failure(proc):
            return [g.failure(proc)]
        for stem, n in want.items():
            data = (work / f"{stem}.srm").read_bytes()
            if len(data) != n or set(data) != {255}:
                errs.append(f"{stem}.srm: {len(data)} bytes, want {n} of $FF")
    return errs


g.check("new lorom.sfc + set slot 1 == the save luna wrote", "byte-identical", new_then_set)
g.check("new: 8 KB (LoROM $FFD8=3), 2 KB (HiROM $FFD8=1), 64 KB (Super FX $FFBD=6), --fill 255", "three sizes", sizes)
g.expect_stdout("new --json", ["new", "--json", "lorom.sfc", "gsu.sfc"], golden="new.json", copy=["lorom.sfc", "gsu.sfc"])
g.expect_stdout("inspect", ["inspect", "slot1.srm"], golden="inspect.txt", copy=["slot1.srm"])
g.expect_stdout("inspect --json --rom", ["inspect", "--json", "--rom", "lorom.sfc", "slot1.srm"], golden="inspect.json", copy=["slot1.srm", "lorom.sfc"])
g.expect_stdout("get --at 4 --count 4", ["get", "slot1.srm", "--at", "4", "--count", "4"], golden="get.txt", copy=["slot1.srm"])
g.expect_outputs("get --to FILE", ["get", "slot1.srm", "--count", "8", "--to", "slot.bin"], copy=["slot1.srm"], outputs=["slot.bin"])
g.expect_outputs("set --from FILE at an offset", ["set", "-q", "slot1.srm", "--at", "256", "--from", "slot.bin"], copy=["slot1.srm"],
                 write={"slot.bin": bytes.fromhex(SLOT1.replace(" ", ""))}, outputs=["slot1.srm"], want_dir=g.golden / "patched")
g.expect_refused("new nosave.sfc (no save RAM declared)", ["new", "nosave.sfc"], copy=["nosave.sfc"], needles=["declares no save RAM"], nothing_written=["nosave.srm"])
g.expect_refused("new notarom.sfc", ["new", "notarom.sfc"], copy=["notarom.sfc"], needles=["no SNES header"], nothing_written=["notarom.srm"])
g.expect_refused("inspect --rom gsu.sfc slot1.srm (8 KB for a 64 KB cartridge)", ["inspect", "--rom", "gsu.sfc", "slot1.srm"], copy=["gsu.sfc", "slot1.srm"],
                 needles=["declares 65536 bytes"])
g.expect_refused("set past the end", ["set", "slot1.srm", "--at", "8190", "--hex", "01 02 03"], copy=["slot1.srm"], needles=["do not fit"])
g.expect_refused("set --hex with half a byte", ["set", "slot1.srm", "--hex", "0A 1"], copy=["slot1.srm"], needles=["whole bytes"])
g.expect_refused("get outside the file", ["get", "slot1.srm", "--at", "8192"], copy=["slot1.srm"], needles=["outside the file"])
g.expect_refused("diff of two different saves", ["diff", "slot1.srm", "blank.srm"], copy=["slot1.srm"], write={"blank.srm": bytes(8192)}, needles=["6 bytes differ in 3 ranges"])
sys.exit(g.report())
