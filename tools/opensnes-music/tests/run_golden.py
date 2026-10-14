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


# smconv's own cases, through this tool (its successor: the build calls
# `bank`, and smconv's binary is retired). Compressed samples: a real IT 2.14
# file written by a tracker (reflection.it, modlib, MIT), pollen8 rewritten
# with every sample compressed (it214, it215: the plain golden, unchanged),
# 16-bit and multi-block twins (the uncompressed twin converted in the same
# run is the reference), and two cut-short compressed blocks refused.
SMCONV_TESTS = g.here.parents[1] / "smconv" / "tests"
sys.path.insert(0, str(SMCONV_TESTS))
import itcompress  # noqa: E402
import struct  # noqa: E402

OUTPUTS = ["soundbank.asm", "soundbank.h", "soundbank.bnk"]
STRETCH = 16   # the longest pollen8 sample spans two 8-bit blocks, three 16-bit ones


def _convert(work: Path, data: bytes, name: str = "pollen8.it"):
    return g.run(["bank", "-q", "--name", "soundbank", "--bank", "1", name], work, write={name: data})


def _same(work: Path, proc, want_dir: Path) -> list[str]:
    err = g.failure(proc)
    return [err] if err else g.compare(work, OUTPUTS, want_dir)


def _compressed(src: bytes, how: str, **kw) -> tuple[bytes, list[str]]:
    data, stats = itcompress.rewrite(src, compress_as=how, **kw)
    return data, [f"encoder never used {k}" for k, v in stats.items() if v == 0]


def smconv_cases() -> None:
    pollen8 = (SMCONV_TESTS / "fixtures" / "pollen8.it").read_bytes()
    reflection = (SMCONV_TESTS / "fixtures" / "reflection.it").read_bytes()
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)
        work = root / "reflection"
        g.record("reflection.it (IT 2.14, compressed)", "outputs match smconv's golden",
                 _same(work, _convert(work, reflection, "reflection.it"), SMCONV / "reflection"))
        for how in ("it214", "it215"):
            work = root / how
            data, errs = _compressed(pollen8, how)
            g.record(f"pollen8.it as {how}", "same soundbank as uncompressed",
                     errs + _same(work, _convert(work, data), SMCONV))
        for bits16 in (False, True):
            label = f"pollen8.it x{STRETCH} {'16' if bits16 else '8'}-bit"
            twin, _ = itcompress.rewrite(pollen8, bits16=bits16, factor=STRETCH)
            ref = root / f"twin{int(bits16)}"
            proc = _convert(ref, twin)
            if proc.returncode != 0 or not all((ref / o).is_file() for o in OUTPUTS):
                g.record(label, "", [f"uncompressed twin not converted (exit {proc.returncode})"])
                continue
            for how in ("it214", "it215"):
                work = root / f"twin{int(bits16)}_{how}"
                data, errs = _compressed(pollen8, how, bits16=bits16, factor=STRETCH)
                g.record(f"{label} as {how}", "same soundbank as its uncompressed twin",
                         errs + _same(work, _convert(work, data), ref))
        ordnum, insnum = struct.unpack_from("<HH", reflection, 0x20)
        hdr, = struct.unpack_from("<I", reflection, 0xC0 + ordnum + 4 * insnum)
        ptr = struct.unpack_from("<I", reflection, hdr + 0x48)[0]
        short = bytearray(reflection)
        struct.pack_into("<H", short, ptr, 5)
        for name, data in (("reflection_eof.it", reflection[:ptr + 12]),
                           ("reflection_short_block.it", bytes(short))):
            work = root / name
            proc = _convert(work, data, name)
            g.record(name, "refused, sample named, nothing written",
                     g.refused(work, proc, ("sample 'doodle'", "corrupt compressed data (block 1)"),
                               nothing_written=OUTPUTS))


smconv_cases()
sys.exit(g.report())
