"""Golden cases of `opensnes-rom place` and of `check --objects` (L4 static frames, step S1).

Each case writes a small program as the build leaves it: a linkfile, the texts beside the objects (C texts with
the compiler's side table, assembly texts), and for the check a .sym. The C texts are written by hand in the shape
cc65816 gives them (`; Function:` header, label, body, `.ENDS`, then the `; sframe` lines), with the frames the
case needs claimed by hand (`sf=N`), since the compiler claims none in S1.

The cases: placement of a chain and of a diamond (each frame past its deepest framed ancestor) and the check
accepting that placement; the re-entries place must refuse without writing (through C in another file, through
assembly naming a C function, through a function pointer, through assembly that jumps through a pointer, through an
object without a text), with an old sframe.asm removed; and one refusal of the check per rule: a call the table
omits, a runtime helper outside the named list, an overlap of two live frames (from a placement forced by
OPENSNES_SFRAME_FAULT=overlap), a window on tcc__lf and on .snesmod_zp, a frame past $FF, a cycle through a framed
function (written despite the re-entry by OPENSNES_SFRAME_FAULT=reentry), a frame an NMI callback reaches past the
NMI page, an sf.* label without a table frame and the reverse, a frame used past its size, an access without .b,
assembly naming a frame; and a reviewed library routine trusted as it is and open again once one line changes.
"""
from __future__ import annotations

import re
import tempfile
from pathlib import Path

SYM_HEAD = """; synthetic wlalink .sym
[information]
version 3
[labels]
00:8000 main
00:a000 NmiHandler
00:a100 DefaultNmiCallback
00:a1a0 DataInitEnd
{labels}
[definitions]
00000008 _sizeof_DataInitEnd
[sections]
00000000 00:0000 8000 00000010 .text.main
00002190 00:2190 a190 00000010 .data_init
[ramsections]
00:0000 0000 00000034 .registers
00:0080 0080 00000028 .leaf_frame
00:0700 0700 000000a8 .nmi_registers
{ramsections}
"""


def fn(name, calls=(), sf=0, export=None, body=(), table_calls=None, indirect=0):
    """one C function: its text and its table lines. calls: callee names (jsl), or ('jml', x), or '*' (through a
    pointer); table_calls: what the table says when it differs from the text (a fault)."""
    if export is None:
        export = "." not in name
    text = [f"; Function: {name} (framesize=0, slots=0, alloc_slots=0, fn_leaf=0, leaf_opt=1)",
            f'.SECTION ".text.{name}" SUPERFREE', ".ACCU 16", ".INDEX 16", f"{name}:", "\trep #$20"]
    tab = []
    for c in calls:
        if c == "*":
            text += ["\tphk", "\tpea ++-1", "\tjml [tcc__r9]", "++"]
            tab.append(f"; sframe call {name} * 0")
        elif isinstance(c, tuple):
            text.append(f"\t{c[0]} {c[1]}")
            tab.append(f"; sframe call {name} {c[1]} 0")
        else:
            text.append(f"\tjsl {c}")
            if not c.startswith("__"):
                tab.append(f"; sframe call {name} {c} 0")
    text += [f"\t{b}" for b in body] + ["\trtl", ".ENDS", f"/* end function {name} */"]
    if table_calls is not None:
        tab = [f"; sframe call {name} {c} 0" for c in table_calls]
    head = (f"; sframe fn {name} export={int(export)} addrtaken=0 leaf=0 alloc=0 dynalloc=0 framesize=0 "
            f"stack={sf} params=0 lf=0 large=0 sf={sf} cand=yes")
    return "\n".join(text + [head] + tab) + "\n"


def unit(*fns, addr=(), refs=()):
    """a C text: its functions, the names whose address it takes (`; sframe addr` and a `.dl` in its data)"""
    out = ['.include "memmap.inc"', ""] + list(fns)
    if addr or refs:
        out.append('.SECTION ".rodata.1" SUPERFREE')
        out += [f"\t.dl {a}" for a in list(addr) + list(refs)]
        out.append(".ENDS")
    out += [f"; sframe addr {a}" for a in addr] + ["; sframe table 1", "", "; End of generated code", ""]
    return "\n".join(out)


def linkfile(*objs):
    return "[objects]\n" + "\n".join(objs) + "\n"


def sym(frames=None, window=None, extra=()):
    """frames: {fn: (address, size)}; window: (address, size) of .sframe"""
    labels = [f"00:{a:04x} sf.{n}.f" for n, (a, _) in (frames or {}).items()]
    rams = [f"00:{a:04x} {a:04x} {s:08x} {n}" for n, a, s in extra]
    if window:
        rams.append(f"00:{window[0]:04x} {window[0]:04x} {window[1]:08x} .sframe")
    return SYM_HEAD.format(labels="\n".join(labels), ramsections="\n".join(rams))


def offsets(asm_text):
    """{fn: offset} from the comments of a written sframe.asm"""
    return {m.group(1): int(m.group(3)) for m in re.finditer(r"^sf\.(\S+)\.f DSB \d+\s+; (\d+) bytes at \+(\d+)",
                                                             asm_text, re.M)}


CHECK = ["check", "--no-bank-reads", "--no-nmi-race", "--no-assets", "--objects", "linkfile", "game.sym"]

# a chain f -> g -> h of statics of a.c, and a diamond a -> b, a -> c, b -> d, c -> d of b.c, all framed
CHAIN = unit(fn("main", ["f.a", "dia.bb"]), fn("f.a", ["g.a"], sf=4), fn("g.a", ["h.a"], sf=2), fn("h.a", ["leaf"], sf=6),
             fn("leaf"))
DIAMOND = unit(fn("dia.bb", ["b.bb", "c.bb"], sf=2), fn("b.bb", ["d.bb"], sf=8), fn("c.bb", ["d.bb"], sf=3),
               fn("d.bb", ["leaf2"], sf=5), fn("leaf2"))
GOOD = {"a.c.wrap.asm": CHAIN, "bb.c.wrap.asm": DIAMOND, "linkfile": linkfile("a.c.o", "bb.c.o")}
GOOD_OFF = {"f.a": 0, "g.a": 4, "h.a": 6, "dia.bb": 0, "b.bb": 2, "c.bb": 2, "d.bb": 10}
SIZES = {"f.a": 4, "g.a": 2, "h.a": 6, "dia.bb": 2, "b.bb": 8, "c.bb": 3, "d.bb": 5}


def placed_sym(off, base=0xA8):
    frames = {n: (base + o, SIZES[n]) for n, o in off.items()}
    deep = max(o + SIZES[n] for n, o in off.items())
    return sym(frames, (base, deep))


def add_cases(g):
    def run(argv, files, env=None):
        import os
        import subprocess
        td = tempfile.mkdtemp(prefix="sframe_")
        work = Path(td)
        g.stage(work, (), files)
        proc = subprocess.run([str(g.tool), *argv], cwd=work, capture_output=True, text=True, timeout=g.timeout,
                              env={**os.environ, "OPENSNES_HOME": str(g.repo), **(env or {})})
        return work, proc

    def placement():
        errs = []
        work, proc = run(["place", "linkfile"], GOOD)
        if proc.returncode != 0:
            return [f"place exit {proc.returncode}: {g.said(proc).strip()[:300]}"]
        got = offsets((work / "sframe.asm").read_text())
        if got != GOOD_OFF:
            errs.append(f"offsets {got}, {GOOD_OFF} expected")
        text = (work / "sframe.asm").read_text()
        if '.RAMSECTION ".sframe" BANK 0 SLOT 1 ORGA $00A8 FORCE' not in text:
            errs.append("no .sframe section at $00A8")
        # the check accepts the placement read back from a .sym
        (work / "game.sym").write_text(placed_sym(got))
        import subprocess
        c = subprocess.run([str(g.tool), *CHECK], cwd=work, capture_output=True, text=True)
        if c.returncode != 0:
            errs.append(f"the check refuses the placement: {g.said(c).strip()[:400]}")
        return errs

    g.check("place: a chain and a diamond", "each frame past its deepest framed ancestor; the check accepts it",
            placement)

    def empty():
        work, proc = run(["place", "linkfile"], {"a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["leaf"]), fn("leaf")),
                                                 "linkfile": linkfile("a.c.o")})
        text = (work / "sframe.asm").read_text() if (work / "sframe.asm").exists() else ""
        errs = [] if proc.returncode == 0 else [f"exit {proc.returncode}"]
        if "sf." in text.replace("sf.*", "") or ".RAMSECTION" in text:
            errs.append("a section or a label in the window of a program without frames")
        return errs

    g.check("place: no frame (today's programs)", "an empty sframe.asm: no section, no sf.* label", empty)

    def reentry(label, files, needles):
        def case():
            files2 = dict(files)
            files2["sframe.asm"] = "; an old window\n"
            work, proc = run(["place", "linkfile"], files2)
            return g.refused(work, proc, needles, nothing_written=("sframe.asm",))
        g.check(f"place refuses a re-entry {label}", "refused, sframe.asm removed", case)

    # Fable's shape: f -> B.x -> A.e1 -> f, through C of another file
    reentry("through C in another file", {
        "a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["x"], sf=4), fn("e1", ["f.a"])),
        "b.c.wrap.asm": unit(fn("x", ["e1"])),
        "linkfile": linkfile("a.c.o", "b.c.o")}, ["f.a has a static frame and can be active twice", "f.a -> x -> e1 -> f.a"])
    reentry("through assembly naming a C function", {
        "a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["asmR"], sf=4), fn("e0", ["f.a"])),
        "r.wrap.asm": '.SECTION "r" SUPERFREE\nasmR:\n\tjsl e0\n\trtl\n.ENDS\n',
        "linkfile": linkfile("a.c.o", "r.o")}, ["f.a -> asmR -> e0 -> f.a"])
    reentry("through a function pointer", {
        "a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["*"], sf=4), fn("e0", ["f.a"]), addr=["e0"]),
        "linkfile": linkfile("a.c.o")}, ["f.a -> e0 -> f.a"])
    reentry("through assembly that jumps through a pointer", {
        "a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["disp"], sf=4)),
        "r.wrap.asm": '.SECTION "r" SUPERFREE\ndisp:\n\tjmp (handlers,x)\n.ENDS\n',
        "linkfile": linkfile("a.c.o", "r.o")}, ["disp [", "a jump through a pointer", "UNKNOWN -> main -> f.a"])
    reentry("through an object without a text", {
        "a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["ext"], sf=4)),
        "linkfile": linkfile("a.c.o", "ext.o")}, ["calls ext, which no linked text defines", "UNKNOWN -> main -> f.a"])

    def refused_check(label, files, needles, env=None, place_first=False, placed=None):
        def case():
            f2 = dict(files)
            if placed is not None:
                f2["game.sym"] = placed
            work, proc = run(CHECK, f2, env)
            return g.refused(work, proc, needles)
        g.check(f"check refuses {label}", "refused", case)

    # the table omits a call the text makes (what QBE_SFRAME_FAULT=noedge does on a real build)
    refused_check("a call the table omits", {
        "a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["g.a", "leaf"], table_calls=["leaf"]), fn("g.a", ["leaf"]), fn("leaf")),
        "linkfile": linkfile("a.c.o")}, ["table/text: f.a calls g.a and its table does not say so"], placed=sym())
    refused_check("a runtime helper outside the named list", {
        "a.c.wrap.asm": unit(fn("main", ["__evil", "__mul16"])), "linkfile": linkfile("a.c.o")},
        ["main calls __evil and its table does not say so"], placed=sym())

    def helper_named():
        work, proc = run(CHECK, {"a.c.wrap.asm": unit(fn("main", ["__mul16", "__sdiv16"])), "linkfile": linkfile("a.c.o"),
                                 "game.sym": sym()})
        return [] if proc.returncode == 0 else [f"refused: {g.said(proc).strip()[:300]}"]

    g.check("check accepts the named helpers", "__mul16 and __sdiv16 need no table line", helper_named)

    def overlap():
        work, proc = run(["place", "linkfile"], GOOD, {"OPENSNES_SFRAME_FAULT": "overlap"})
        if proc.returncode != 0:
            return [f"place exit {proc.returncode}"]
        got = offsets((work / "sframe.asm").read_text())
        work2, c = run(CHECK, {**GOOD, "game.sym": placed_sym(got)})
        return g.refused(work2, c, ["overlap: g.a", "can run while f.a", "their frames overlap"])

    g.check("check refuses an overlap of two live frames", "placed by OPENSNES_SFRAME_FAULT=overlap, refused from the .sym",
            overlap)
    refused_check("a window on tcc__lf", GOOD, ["window: .sframe ($0080-", "overlaps .leaf_frame", "lies on .leaf_frame"],
                  placed=placed_sym(GOOD_OFF, base=0x80))
    snes = sym({n: (0x40 + o, SIZES[n]) for n, o in GOOD_OFF.items()}, (0x40, 15), extra=[(".snesmod_zp", 0x40, 0x21)])
    refused_check("a window on .snesmod_zp", GOOD, ["overlaps .snesmod_zp", "lies on .snesmod_zp"], placed=snes)
    refused_check("a frame past $FF", GOOD, ["window: the frame of d.bb", "ends past $FF"],
                  placed=placed_sym(GOOD_OFF, base=0xF2))

    def cycle():
        files = {"a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["x"], sf=4), fn("e1", ["f.a"])),
                 "b.c.wrap.asm": unit(fn("x", ["e1"])), "linkfile": linkfile("a.c.o", "b.c.o")}
        work, proc = run(["place", "linkfile"], files, {"OPENSNES_SFRAME_FAULT": "reentry"})
        if proc.returncode != 0 or not (work / "sframe.asm").exists():
            return [f"place with the fault did not write: exit {proc.returncode}"]
        work2, c = run(CHECK, {**files, "game.sym": sym({"f.a": (0xA8, 4)}, (0xA8, 4))})
        return g.refused(work2, c, ["f.a has a static frame and can be active twice: f.a -> x -> e1 -> f.a"])

    g.check("check refuses a cycle through a framed function", "written by OPENSNES_SFRAME_FAULT=reentry, refused", cycle)
    nmi_unit = unit(fn("main", ["f.a"], body=["pea.w :cb", "pea.w cb", "jsl nmiSet"], table_calls=["f.a", "nmiSet"]), fn("f.a", ["leaf"], sf=4),
                    fn("cb", ["f.a"]), fn("leaf"), fn("nmiSet"), refs=["cb"], addr=["cb"])
    # the literal callback is a pea of the function: the text takes its address
    refused_check("a frame an NMI callback reaches past the NMI page", {"a.c.wrap.asm": nmi_unit, "linkfile": linkfile("a.c.o")},
                  ["nmi: f.a can run in the NMI", "past the NMI page (168 bytes)"], placed=sym({"f.a": (0xA8, 4)}, (0xA8, 4)))

    def nmi_inside():
        work, proc = run(CHECK, {"a.c.wrap.asm": nmi_unit, "linkfile": linkfile("a.c.o"),
                                 "game.sym": sym({"f.a": (0xA0, 4)}, (0xA0, 4), )})
        # $A0 lies on tcc__lf: refused for that, not for the NMI
        t = g.said(proc)
        return [] if "nmi:" not in t else ["an NMI refusal for a frame inside the NMI page"]

    g.check("check: an NMI-reachable frame inside the NMI page", "no NMI refusal", nmi_inside)
    one = {"a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["leaf"], sf=4), fn("leaf")), "linkfile": linkfile("a.c.o")}
    refused_check("a table frame the .sym does not define", one, ["sym: f.a has a static frame of 4 bytes and the .sym has no sf.f.a.f"],
                  placed=sym({}, (0xA8, 4)))
    refused_check("an sf.* label no table claims", one, ["sym: the .sym defines sf.leaf.f and no table gives leaf a static frame"],
                  placed=sym({"f.a": (0xA8, 4), "leaf": (0xAC, 2)}, (0xA8, 6)))
    over = {"a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["leaf"], sf=4, body=["lda.b sf.f.a.f+6"]), fn("leaf")),
            "linkfile": linkfile("a.c.o")}
    refused_check("a frame used past its size", over, ["frame: f.a uses 8 bytes of its static frame and its table declares 4"],
                  placed=sym({"f.a": (0xA8, 4)}, (0xA8, 4)))
    nob = {"a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["leaf"], sf=4, body=["lda.w sf.f.a.f+2"]), fn("leaf")),
           "linkfile": linkfile("a.c.o")}
    refused_check("an access to a frame without .b", nob, ["without .b", "lda.w sf.f.a.f+2"],
                  placed=sym({"f.a": (0xA8, 4)}, (0xA8, 4)))
    asmsf = dict(one)
    asmsf["r.wrap.asm"] = '.SECTION "r" SUPERFREE\npoke:\n\tlda.b sf.f.a.f+2\n\trtl\n.ENDS\n'
    asmsf["linkfile"] = linkfile("a.c.o", "r.o")
    refused_check("assembly naming a frame", asmsf, ["frame: assembly names a static frame", "r.wrap.asm"],
                  placed=sym({"f.a": (0xA8, 4)}, (0xA8, 4)))

    def reviewed():
        src = g.repo / "lib" / "source" / "string.asm"
        if not src.is_file():
            return [f"{src} missing"]
        text = src.read_text()
        prog = {"a.c.wrap.asm": unit(fn("main", ["f.a"]), fn("f.a", ["memcpy"], sf=4)),
                "linkfile": linkfile("a.c.o", "lib/build/lorom/string-asm.o")}
        errs = []
        for changed in (False, True):
            t = text.replace("lda #$6B                ; RTL", "lda #$6B ; RTL\n    nop", 1) if changed else text
            files = {**prog, "lib/build/lorom/string-asm.text.asm": t}
            td = tempfile.mkdtemp(prefix="sframe_")
            work = Path(td)
            (work / "lib" / "build" / "lorom").mkdir(parents=True)
            import os
            import subprocess
            for n, d in files.items():
                (work / n).write_text(d)
            p = subprocess.run([str(g.tool), "place", "linkfile"], cwd=work, capture_output=True, text=True,
                               env={**os.environ, "OPENSNES_HOME": str(g.repo)})
            if not changed and p.returncode != 0:
                errs.append(f"memcpy as reviewed: refused: {g.said(p).strip()[:300]}")
            if changed and (p.returncode == 0 or "memcpy [" not in g.said(p)):
                errs.append(f"memcpy changed by one line: not refused through memcpy: {g.said(p).strip()[:300]}")
        return errs

    g.check("place: a reviewed routine (memcpy)", "trusted as read; open again once a line changes", reviewed)
