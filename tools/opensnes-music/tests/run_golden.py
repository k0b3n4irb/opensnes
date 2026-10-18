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
SMCONV = g.here.parents[1] / "lib" / "smconv" / "tests" / "golden"
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
        # the module where the driver reads it: `luna spc-dump` of
        # snesmod_music (the same module, played by the game) shows these
        # bytes at $1A00; the file's ARAM starts at offset $100
        at = data[0x100 + 0x1A00:0x100 + 0x1A04]
        if at != bytes.fromhex("806e0324"):
            return [f"pollen8.spc: ARAM $1A00 holds {at.hex()}, not the module header 806e0324"]
    return []


def unwritable(blocked):
    """An output that cannot be opened (a folder of that name is in the way):
    the tool says so and exits 3, where it used to write through NULL (.asm,
    .h) or skip the .bnk in silence and exit 0."""
    def run():
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            (work / blocked).mkdir()
            proc = g.run(["bank", "-q", "--name", "soundbank", "pollen8.it"], work, copy=["pollen8.it"])
            errs = []
            if proc.returncode != 3:
                errs.append(f"exit {proc.returncode}, want 3 (I/O)")
            if "cannot write the soundbank" not in g.said(proc):
                errs.append(f"without 'cannot write the soundbank': {g.said(proc).strip()[-120:]}")
            return errs
    return run


for blocked in ("soundbank.bnk", "soundbank.asm", "soundbank.h"):
    g.check(f"bank with {blocked} unwritable", "exit 3, said", unwritable(blocked))


g.check("spc pollen8.it: a 66048-byte SPC file", "SPC700 header, 64 KB RAM + registers, module at $1A00", spc_written)
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
SMCONV_TESTS = g.here.parents[1] / "lib" / "smconv" / "tests"
sys.path.insert(0, str(SMCONV_TESTS))
import itcompress  # noqa: E402
import struct  # noqa: E402

OUTPUTS = ["soundbank.asm", "soundbank.h", "soundbank.bnk"]
STRETCH = 16   # the longest pollen8 sample spans two 8-bit blocks, three 16-bit ones
LONGEST = 10   # that sample (2122 frames): stretched alone, so the module still fits the driver's ARAM


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
    # its echo (delay 14, 28 KB) leaves 29952 bytes from $1A00; the stretched twin needs more
    short_echo = pollen8.replace(b"edl 14", b"edl 02")
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
            twin, _ = itcompress.rewrite(short_echo, bits16=bits16, factor=STRETCH, only=LONGEST)
            ref = root / f"twin{int(bits16)}"
            proc = _convert(ref, twin)
            if proc.returncode != 0 or not all((ref / o).is_file() for o in OUTPUTS):
                g.record(label, "", [f"uncompressed twin not converted (exit {proc.returncode})"])
                continue
            for how in ("it214", "it215"):
                work = root / f"twin{int(bits16)}_{how}"
                data, errs = _compressed(short_echo, how, bits16=bits16, factor=STRETCH, only=LONGEST)
                g.record(f"{label} as {how}", "same soundbank as its uncompressed twin",
                         errs + _same(work, _convert(work, data), ref))
        # every sample stretched: 84734 bytes, where the driver has 29952
        # from $1A00 under this module's echo — refused, nothing written
        big, _ = itcompress.rewrite(pollen8, factor=STRETCH)
        g.expect_refused(f"bank pollen8.it x{STRETCH} (too big for the ARAM)", BANK, write={"pollen8.it": big},
                         needles=["needs 84734 bytes of SPC RAM, more than the 29952"], nothing_written=OUTPUTS)
        g.expect_refused(f"spc pollen8.it x{STRETCH} (too big for the ARAM)", ["spc", "pollen8.it"], write={"pollen8.it": big},
                         needles=["more than the 29952"], nothing_written=["pollen8.spc"])
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
