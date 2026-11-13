#!/usr/bin/env python3
"""Golden tests for opensnes-rom: check, inspect, budget.

Synthetic .sym and .c.asm fixtures, one per failure the check must catch:
the bank $00 ratchet, a RAM section across $2000, a data-init sentinel that
is not last, a bank-blind read of bank $01+ data, a WRAM-port write
reachable from an NMI callback; and the healthy case, whose --json is the
golden. The verdicts and figures against the five Python scripts were
compared on the 99 built ROMs of the repository on 2026-10-05 (identical).

`inspect` and `budget` read synthetic ROMs whose checksums were computed
by the script that made them (independently of the tool): a healthy LoROM
(good.sfc, the ROM of good.sym), a HiROM FastROM, a 96 KB image (not a power
of two: the last 32 KB count twice), the healthy ROM with one byte edited
after the link, the same behind a 512-byte copier header, and 32 KB of zeros.

Run:  python3 tools/opensnes-rom/tests/run_golden.py
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes-rom", __file__)
g.expect_stdout("check good.sym --json (healthy)", ["check", "--json", "good.sym"], golden="good.json", copy=["good.sym", "good.c.asm"])
g.expect_refused("check good.sym --bank0-fail 30000 (ratchet)", ["check", "--bank0-fail", "30000", "good.sym"],
                 copy=["good.sym", "good.c.asm"], needles=["under the threshold asked for"])
g.expect_refused("check good.sym --ram-fail 8000 (ratchet)", ["check", "--ram-fail", "8000", "good.sym"],
                 copy=["good.sym", "good.c.asm"], needles=["C RAM band nearly full"])
g.expect_refused("check ram_crosser.sym (a section across $2000)", ["check", "--no-nmi-race", "ram_crosser.sym"],
                 copy=["ram_crosser.sym"], needles=["crosses $2000"])
g.expect_refused("check datainit_late.sym (sentinel not last)", ["check", "--no-nmi-race", "datainit_late.sym"],
                 copy=["datainit_late.sym"], needles=["data-init sentinel is not last"])
g.expect_refused("check bankblind.sym (bank-blind read)", ["check", "bankblind.sym"],
                 copy=["bankblind.sym", "bankblind.c.asm"], needles=["read bank-blind in bankblind.c.asm", "mapdata"])
g.expect_refused("check sa1_bank1.sym (SA1Start out of bank $00)", ["check", "--no-nmi-race", "sa1_bank1.sym"],
                 copy=["sa1_bank1.sym"], needles=["SA1Start is at $01:8000", "16-bit", "BANK 0"])
g.expect_refused("check nmirace.sym (WRAM port from an NMI callback)", ["check", "nmirace.sym"],
                 copy=["nmirace.sym", "nmirace.c.asm"], needles=["NMI / WRAM port race", "fill", "sta.w $2180"])


def nmirace_lag():
    """The same race from a callback given to nmiSetLag (it runs in the NMI, on lag frames)."""
    import tempfile
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(["check", "nmirace.sym"], work, ["nmirace.sym", "nmirace.c.asm"])
        asm = (work / "nmirace.c.asm").read_text().replace("jsl nmiSet", "jsl nmiSetLag")
        (work / "nmirace.c.asm").write_text(asm)
        proc = g.run(["check", "nmirace.sym"], work)
        if proc.returncode == 0 or "NMI / WRAM port race" not in proc.stdout + proc.stderr:
            return ["a WRAM-port write from an nmiSetLag callback was not refused"]
        return []


g.check("check nmirace.sym with nmiSetLag", "refused like nmiSet", nmirace_lag)


def nmirace_static():
    """The write in a static C function (`fill.main`), placed in the file before any function the callback
    reaches: until 2026-11-14 a name with a dot was not read as a function, its body went to the function above
    it (here one nothing reaches) and the race passed."""
    import tempfile
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        asm = ("other:\n\trts\nfill.main:\n\tsta.w $2180\n\trts\nmain:\n\tpea.w my_cb.main\n\tjsl nmiSet\n\trts\n"
               "my_cb.main:\n\tjsr fill.main\n\trts\n")
        proc = g.run(["check", "nmirace.sym"], work, ["nmirace.sym"], {"nmirace.c.asm": asm})
        text = proc.stdout + proc.stderr
        if proc.returncode == 0 or "fill.main" not in text or "NMI / WRAM port race" not in text:
            return ["a WRAM-port write in a static function reached from a static callback was not refused"]
        return []


g.check("check: the race in a static function (fill.main)", "refused, the static function named", nmirace_static)
g.expect_stdout("inspect good.sfc hirom.sfc oddsize.sfc (header, checksum, digests)", ["inspect", "good.sfc", "hirom.sfc", "oddsize.sfc"],
                golden="inspect.txt", copy=["good.sfc", "hirom.sfc", "oddsize.sfc"])
g.expect_stdout("inspect --json good.sfc hirom.sfc", ["inspect", "--json", "good.sfc", "hirom.sfc"],
                golden="inspect.json", copy=["good.sfc", "hirom.sfc"])
g.expect_refused("inspect edited.sfc (a byte changed after the link)", ["inspect", "edited.sfc"], copy=["edited.sfc"],
                 needles=["checksum in the header does not match", "relink"])
g.expect_refused("inspect notarom.sfc (no header)", ["inspect", "notarom.sfc"], copy=["notarom.sfc"], needles=["no cartridge header"])


def copier():
    """A copier header is skipped and said: same title, same digests as good.sfc, exit 0."""
    import tempfile
    with tempfile.TemporaryDirectory() as td:
        proc = g.run(["inspect", "copier.sfc"], Path(td), ["copier.sfc"])
        errs = []
        if proc.returncode != 0:
            errs.append(f"exit {proc.returncode}")
        if "copier header" not in proc.stderr:
            errs.append("the copier header is not said")
        if "crc32 24d69500" not in proc.stdout or "checksum $0612 ok" not in proc.stdout:
            errs.append("not the digests of good.sfc")
        return errs


g.check("inspect copier.sfc (512 bytes in front)", "warned, same image", copier)


def scenes():
    """One folder per level under res/: the figure is the heaviest level plus
    what res/ shares, not the sum. res/shared.pic 500 bytes and 16 colours;
    res/lvl1 1000 bytes, res/lvl2 3000 bytes and 32 colours: the heaviest is
    lvl2 at 3500 bytes and 48 colours (all of them), the total 4500.
    Without the folders,
    the total says it is everything at once."""
    import json
    import tempfile
    errs = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        (work / "res" / "lvl1").mkdir(parents=True)
        (work / "res" / "lvl2").mkdir()
        (work / "res" / "shared.pic").write_bytes(bytes(500))
        (work / "res" / "shared.pal").write_bytes(bytes(32))
        (work / "res" / "lvl1" / "a.pic").write_bytes(bytes(1000))
        (work / "res" / "lvl2" / "b.pic").write_bytes(bytes(3000))
        (work / "res" / "lvl2" / "b.pal").write_bytes(bytes(64))
        proc = g.run(["check", "--json", "good.sym"], work, ["good.sym", "good.c.asm"])
        a = json.loads(proc.stdout).get("assets", {}) if proc.returncode == 0 else {}
        want = {"vram_bytes": 4500, "colours": 48, "scenes": 2, "heaviest_scene": "res/lvl2",
                "heaviest_scene_vram_bytes": 3500, "heaviest_scene_colours": 48}
        for k, v in want.items():
            if a.get(k) != v:
                errs.append(f"assets.{k} = {a.get(k)!r}, {v!r} expected")
        text = g.run(["check", "good.sym"], work, ["good.sym", "good.c.asm"]).stdout
        if "heaviest scene res/lvl2: VRAM 3.4K/64K" not in text or "2 scenes" not in text:
            errs.append("the text report does not name the heaviest scene: " + text.strip().splitlines()[-1])
        for f in ("lvl1/a.pic", "lvl2/b.pic", "lvl2/b.pal"):
            (work / "res" / f).unlink()
        (work / "res" / "lvl1").rmdir()
        (work / "res" / "lvl2").rmdir()
        text = g.run(["check", "good.sym"], work, ["good.sym", "good.c.asm"]).stdout
        if "every asset together, not necessarily loaded at once" not in text:
            errs.append("without scenes, the total does not say what it is")
    return errs


g.check("check with res/lvl1 and res/lvl2 (one scene per folder)", "the heaviest scene, not the sum", scenes)


def declared_vram():
    """An asset's settings may say what it holds in VRAM at once (`vram = N`
    before its table): its files then count N once instead of their size.
    res/sheet.pic 42 000 bytes declared 8192 (a cache), res/lvl1/obj.pic
    48 000 declared 0 (streamed through it), res/lvl1/walls.pic 30 000
    undeclared: lvl1 is 8192 + 30 000 = 38 192."""
    import json
    import tempfile
    errs = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        (work / "res" / "lvl1").mkdir(parents=True)
        (work / "res" / "sheet.pic").write_bytes(bytes(42000))
        (work / "res" / "sheet.png.toml").write_text('tool = "opensnes-sprite"\nvram = 8192\n\n[sheet]\n')
        (work / "res" / "lvl1" / "obj.pic").write_bytes(bytes(48000))
        (work / "res" / "lvl1" / "obj.png.toml").write_text('tool = "opensnes-sprite"\nvram = 0   # streamed\n\n[sheet]\n')
        (work / "res" / "lvl1" / "walls.pic").write_bytes(bytes(30000))
        (work / "res" / "lvl1" / "walls.png.toml").write_text('tool = "opensnes-tileset"\n\n[convert]\n')
        proc = g.run(["check", "--json", "good.sym"], work, ["good.sym", "good.c.asm"])
        a = json.loads(proc.stdout).get("assets", {}) if proc.returncode == 0 else {}
        for k, v in {"scenes": 1, "heaviest_scene": "res/lvl1", "heaviest_scene_vram_bytes": 38192, "vram_bytes": 38192}.items():
            if a.get(k) != v:
                errs.append(f"assets.{k} = {a.get(k)!r}, {v!r} expected")
    return errs


g.check("check with vram = N in settings files (a cache, a streamed sheet)", "the declared bytes, once", declared_vram)
g.expect_stdout("budget good.sfc (ROM by bank, RAM, video)", ["budget", "good.sfc"], golden="budget.txt", copy=["good.sfc", "good.sym", "good.c.asm"])
g.expect_stdout("budget --json good.sfc", ["budget", "--json", "good.sfc"], golden="budget.json", copy=["good.sfc", "good.sym", "good.c.asm"])
g.expect_stdout("budget good.sfc + a module (ARAM from opensnes-music inspect)", ["budget", "good.sfc"], golden="budget_aram.txt",
                copy=["good.sfc", "good.sym", "good.c.asm", "theme.it"])
g.expect_stdout("budget --json good.sfc + a module", ["budget", "--json", "good.sfc"], golden="budget_aram.json",
                copy=["good.sfc", "good.sym", "good.c.asm", "theme.it"])


def ram_markers():
    """wlalink writes its RAM-usage markers as bank-$00 labels whatever their
    bank (RAM_USAGE_SLOT_2_BANK_126_START at $7E:2000 comes out 00:2000), and a
    profiler folding PCs onto the nearest label credits code to them. The
    check removes those rows from the .sym once it has read it, and only
    them: the rest of the file is byte for byte what it was. --keep-ram-markers
    leaves the file alone. A .sym without them is not rewritten."""
    import tempfile
    errs = []
    good = (g.fixtures / "good.sym").read_text()
    markers = ("00:0000 RAM_USAGE_SLOT_1_BANK_0_START\n00:051f RAM_USAGE_SLOT_1_BANK_0_END\n"
               "00:2000 RAM_USAGE_SLOT_2_BANK_126_START\n00:2fff RAM_USAGE_SLOT_2_BANK_126_END\n")
    marked = good.replace("[definitions]", markers + "[definitions]")
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        proc = g.run(["check", "--no-nmi-race", "marked.sym"], work, ["good.c.asm"], write={"marked.sym": marked})
        if proc.returncode != 0:
            errs.append(f"check exited {proc.returncode} on the marked .sym")
        after = (work / "marked.sym").read_text()
        if after != good:
            errs.append("the .sym is not good.sym once the four marker rows are removed")
        if (work / "marked.sym.tmp").exists():
            errs.append("the temporary file was left behind")
        proc = g.run(["check", "--no-nmi-race", "--keep-ram-markers", "kept.sym"], work, write={"kept.sym": marked})
        if (work / "kept.sym").read_text() != marked:
            errs.append("--keep-ram-markers did not keep the rows")
        before = (work / "good.sym").write_text(good) and None
        st = (work / "good.sym").stat().st_mtime_ns
        g.run(["check", "--no-nmi-race", "good.sym"], work)
        if (work / "good.sym").stat().st_mtime_ns != st:
            errs.append("a .sym without markers was rewritten")
    return errs


g.check("check marked.sym (wlalink's RAM_USAGE_SLOT_* rows)", "removed from the .sym, nothing else touched", ram_markers)
g.expect_refused("check missing.sym", ["check", "missing.sym"], needles=["cannot open"])
def big_rom():
    """A FastROM LoROM image of 128 banks (4 MB): linker bank 64 sits at $C0,
    which the $80/$C0 fold alone took for bank 0 (a game of 80 banks showed
    bank $00 at 200 % and a false "32768 bytes of asset data in bank $00").
    Sections: code in bank 0, 32 KB of assets in banks 64 and 100."""
    import json
    import tempfile
    errs = []
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        img = bytearray(128 * 0x8000)
        img[:0x10000] = (g.fixtures / "good.sfc").read_bytes()[:0x10000]
        img[0x7FD7] = 0x0C                              # ROM size 2^12 KB
        img[0x7FDC:0x7FE0] = bytes([0, 0, 0xFF, 0xFF])  # checksum fields out of the sum
        cs = sum(img) & 0xFFFF
        img[0x7FDC:0x7FE0] = bytes([(cs ^ 0xFFFF) & 0xFF, (cs ^ 0xFFFF) >> 8, cs & 0xFF, cs >> 8])
        (work / "big.sfc").write_bytes(img)
        sym = (g.fixtures / "good.sym").read_text().split("[sections]")[0].replace("02:8000 mapdata", "40:8000 mapdata")
        sym += ("[definitions]\n[sections]\n"
                "00000000 00:0000 8000 00000010 .text.main\n"
                "00200000 c0:0000 8000 00008000 .data.mapdata\n"
                "00320000 e4:0000 8000 00008000 .data.more\n")
        (work / "big.sym").write_text(sym)
        proc = g.run(["budget", "--json", "big.sfc"], work, [])
        if proc.returncode != 0:
            return [f"exit {proc.returncode}: {proc.stderr.strip()[:200]}"]
        banks = {b["bank"]: b["used"] for b in json.loads(proc.stdout)["rom"]["banks"]}
        for b, u in {0: 16, 64: 32768, 100: 32768}.items():
            if banks.get(b) != u:
                errs.append(f"bank {b}: {banks.get(b)!r} bytes, {u} expected")
        if "asset data are in bank $00" in proc.stderr:
            errs.append("a false asset warning for bank $00")
    return errs


g.check("budget --json on a 128-bank FastROM image (banks past 64)", "each section in its own bank", big_rom)


# the static frames (L4 S1): place and check --objects
sys.path.insert(0, str(Path(__file__).resolve().parent))
from sframe_golden import add_cases  # noqa: E402
add_cases(g)

sys.exit(g.report())
