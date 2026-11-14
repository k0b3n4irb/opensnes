#!/usr/bin/env python3
"""A1 S0: count what passing the first integer argument in A would take, from any build (no compiler change).

Maintainer tool (never in a user build). Plan: `.claude/notes/chantiers/gap_lots/A1_first_arg_in_A_plan.md`;
result: `A1_s0_count.md` beside it.

Reads, per program (a project's `.c.asm` and the library's):
  - the L4 side table every `.c.asm` carries (`; sframe fn NAME export= addrtaken= ... framesize= lf=`);
  - the instruction lines of the same `.c.asm` (the TEXT the compiler emits today);
  - each unit's QBE IR as cproc wrote it (`CC65816_KEEP_IR=<dir>`): the class of each parameter (`w` or `l`), the
    variadic mark, and every direct call with the class of each argument.
Decides, per function, whether it is A1 (plan section 1, conditions 1-5: not exported, address never taken, first
parameter Kw, every direct call of the unit with the same number of arguments and a Kw first argument, not variadic)
and prices, from the text alone, what each call site and each callee would gain:
  caller  the push of argument 1 (`pha`, `pea`) goes; a `pea #k` becomes `lda.w #k`; with 2+ arguments argument 1 is
          loaded after the other pushes (its `lda` moves when its operand is still valid at the call, else it is
          reloaded from where it was stored, else stored and reloaded); the cleanup shrinks by 2 bytes under the
          emitter's own rule (plx up to 2 words with a result, 1 without, else the `tsa/clc/adc/tas` arithmetic);
  callee  the reads of the parameter's stack slot (`op N,s`, the slot found by entry-relative offset with the
          pushes and the frame carve tracked): a load at the head before A is written goes; otherwise the parameter
          is stored at entry, in `tcc__lf` (`sta.b`, before the carve) when it crosses no call and the leaf frame has
          a free word, its reads `.b`; else on the stack (`tax / txa / sta N,s` around the carve), reads unchanged.
  cost test (condition 6): the callee's entry cost in cycles (0, 4, 9, more when a frame must be created) at most
          the caller-side saving at its arity (9 for one argument, 4 for two, 0 for three or more; plan 2.2).
Three variants priced apart, not in the rule: A1-L (a Kl first parameter in A + X), A1-W (exported functions, a
second entry `lda N,s` for calls from other units), A1-T (the tail calls `jml` between a stack function and an A1
function: kept, lost).

With `--rom --sym --trace` (a luna trace in the gauge's format, `[Disassembly]|A|X|Y|SP|P|frame|line|hclock`), every
text instruction of every C function is placed at its ROM address (decoded from the ROM bytes, checked mnemonic by
mnemonic and at every local label against the `.sym`), and the trace gives only how many times each ran. Each
removed or added instruction is priced at the median measured cost of its shape (mnemonic + addressing mode) in the
same trace. The busy time is the trace's, as the private gauge tooling computes it.

usage:
  a1_count.py --casm-dir DIR [--casm-dir DIR ...] --ir-dir DIR [--rom F --sym F --trace F] [--json OUT] [--top N]
  a1_count.py --corpus EXAMPLES_DIR --lib-casm DIR --ir-dir DIR [--json OUT]
"""
import argparse
import bisect
import collections
import glob
import json
import os
import re
import statistics
import sys

FNLINE = re.compile(r"^; sframe fn (\S+) (.*)$")
CALLLINE = re.compile(r"^; sframe call (\S+) (\S+) (\d+)$")
ADDRLINE = re.compile(r"^; sframe addr (\S+)$")
SECTION = re.compile(r'^\.SECTION "\.text\.([^"]+)"')
LABEL = re.compile(r"^([A-Za-z_][A-Za-z0-9_.]*):\s*$")
LOCAL = re.compile(r"^(@[A-Za-z0-9_.]+):\s*$")
INS = re.compile(r"^(?:([+-]+))?\t([A-Za-z]{2,4})(?:\.([bwlBWL]))?(?:\s+(.*?))?\s*(?:;.*)?$")
ANON = re.compile(r"^([+-]+)\s*$")
SR = re.compile(r"^(\d+),s$")
ALIAS = {"tsa": "tsc", "tas": "tcs", "tad": "tcd", "tda": "tdc", "swa": "xba", "ina": "inc", "dea": "dec"}
PUSH_BYTES = {"pha": 2, "pea": 2, "phx": 2, "phy": 2, "pei": 2, "phd": 2, "phk": 1, "phb": 1, "php": 1}
# instructions that leave A alone (16-bit A); anything else writes A or ends the walk
A_KEEPS = {"sta", "stx", "sty", "stz", "ldx", "ldy", "inx", "iny", "dex", "dey", "cpx", "cpy", "cmp", "bit", "pea",
           "pha", "phx", "phy", "pei", "plx", "ply", "clc", "sec", "tax", "tay", "txy", "tyx", "nop", "phd", "phb",
           "phk", "php"}
X_WRITERS = {"ldx", "tax", "tsx", "tyx", "plx", "inx", "dex", "mvn", "mvp", "jsl", "jsr", "jml", "rep", "sep", "plp"}
FLOW = {"bcc", "bcs", "beq", "bne", "bmi", "bpl", "bvc", "bvs", "bra", "brl", "jmp", "jml", "jsl", "jsr", "rtl",
        "rts", "rti", "stp"}
# static cycle model of 16-bit A (plan 2.2), for the cost test only; the gain itself is priced from the trace
CYC = {"tax": 2, "txa": 2, "sta_dp": 4, "sta_sr": 5, "frame": 18}
CALLER_SAVING = {1: 9, 2: 4}


# ---------------------------------------------------------------------------------------------- 65816 decoding
def _optable():
    rows = """
brk s8|ora dpx_ind|cop s8|ora sr|tsb dp|ora dp|asl dp|ora dp_lind|php imp|ora imm_m|asl imp|phd imp|tsb abs|ora abs|asl abs|ora long
bpl rel|ora dp_indy|ora dp_ind|ora sr_indy|trb dp|ora dpx|asl dpx|ora dp_lindy|clc imp|ora absy|inc imp|tcs imp|trb abs|ora absx|asl absx|ora longx
jsr abs|and dpx_ind|jsl long|and sr|bit dp|and dp|rol dp|and dp_lind|plp imp|and imm_m|rol imp|pld imp|bit abs|and abs|rol abs|and long
bmi rel|and dp_indy|and dp_ind|and sr_indy|bit dpx|and dpx|rol dpx|and dp_lindy|sec imp|and absy|dec imp|tsc imp|bit absx|and absx|rol absx|and longx
rti imp|eor dpx_ind|wdm s8|eor sr|mvp blk|eor dp|lsr dp|eor dp_lind|pha imp|eor imm_m|lsr imp|phk imp|jmp abs|eor abs|lsr abs|eor long
bvc rel|eor dp_indy|eor dp_ind|eor sr_indy|mvn blk|eor dpx|lsr dpx|eor dp_lindy|cli imp|eor absy|phy imp|tcd imp|jml long|eor absx|lsr absx|eor longx
rts imp|adc dpx_ind|per rel16|adc sr|stz dp|adc dp|ror dp|adc dp_lind|pla imp|adc imm_m|ror imp|rtl imp|jmp abs_ind|adc abs|ror abs|adc long
bvs rel|adc dp_indy|adc dp_ind|adc sr_indy|stz dpx|adc dpx|ror dpx|adc dp_lindy|sei imp|adc absy|ply imp|tdc imp|jmp absx_ind|adc absx|ror absx|adc longx
bra rel|sta dpx_ind|brl rel16|sta sr|sty dp|sta dp|stx dp|sta dp_lind|dey imp|bit imm_m|txa imp|phb imp|sty abs|sta abs|stx abs|sta long
bcc rel|sta dp_indy|sta dp_ind|sta sr_indy|sty dpx|sta dpx|stx dpy|sta dp_lindy|tya imp|sta absy|txs imp|txy imp|stz abs|sta absx|stz absx|sta longx
ldy imm_x|lda dpx_ind|ldx imm_x|lda sr|ldy dp|lda dp|ldx dp|lda dp_lind|tay imp|lda imm_m|tax imp|plb imp|ldy abs|lda abs|ldx abs|lda long
bcs rel|lda dp_indy|lda dp_ind|lda sr_indy|ldy dpx|lda dpx|ldx dpy|lda dp_lindy|clv imp|lda absy|tsx imp|tyx imp|ldy absx|lda absx|ldx absy|lda longx
cpy imm_x|cmp dpx_ind|rep s8|cmp sr|cpy dp|cmp dp|dec dp|cmp dp_lind|iny imp|cmp imm_m|dex imp|wai imp|cpy abs|cmp abs|dec abs|cmp long
bne rel|cmp dp_indy|cmp dp_ind|cmp sr_indy|pei dp|cmp dpx|dec dpx|cmp dp_lindy|cld imp|cmp absy|phx imp|stp imp|jml abs_lind|cmp absx|dec absx|cmp longx
cpx imm_x|sbc dpx_ind|sep s8|sbc sr|cpx dp|sbc dp|inc dp|sbc dp_lind|inx imp|sbc imm_m|nop imp|xba imp|cpx abs|sbc abs|inc abs|sbc long
beq rel|sbc dp_indy|sbc dp_ind|sbc sr_indy|pea abs|sbc dpx|inc dpx|sbc dp_lindy|sed imp|sbc absy|plx imp|xce imp|jsr absx_ind|sbc absx|inc absx|sbc longx
"""
    t = []
    for row in rows.strip().splitlines():
        for cell in row.split("|"):
            mn, mode = cell.split()
            t.append((mn, mode))
    assert len(t) == 256
    return t


OPS = _optable()
LEN = {"imp": 1, "s8": 2, "rel": 2, "rel16": 3, "blk": 3, "long": 4, "longx": 4}


def oplen(mode, m16, x16):
    if mode in LEN:
        return LEN[mode]
    if mode == "imm_m":
        return 3 if m16 else 2
    if mode == "imm_x":
        return 3 if x16 else 2
    if mode.startswith(("dp", "sr")):
        return 2
    return 3   # abs family, abs_ind, absx_ind, abs_lind


# ---------------------------------------------------------------------------------------------- text
class Ins:
    __slots__ = ("mn", "suf", "op", "label", "idx", "pc", "count")

    def __init__(self, mn, suf, op, label=None):
        self.mn, self.suf, self.op, self.label = mn, suf, op, label
        self.pc, self.count = None, 0

    def __repr__(self):
        return f"{self.mn}{'.' + self.suf if self.suf else ''} {self.op or ''}".strip()


def norm_mn(mn):
    mn = mn.lower()
    return ALIAS.get(mn, mn)


def read_casm(path, unit, fns, calls, addr):
    """Functions of one .c.asm: name -> dict(unit, table, body=[Ins | ('L', label)])."""
    cur = None
    for raw in open(path, errors="replace"):
        line = raw.rstrip("\n")
        m = FNLINE.match(line)
        if m:
            d = dict(kv.split("=", 1) for kv in m.group(2).split())
            if m.group(1) in fns:
                fns[m.group(1)]["table"] = d
            continue
        m = CALLLINE.match(line)
        if m:
            calls.append((unit, m.group(1), m.group(2), int(m.group(3))))
            continue
        m = ADDRLINE.match(line)
        if m:
            addr[unit].add(m.group(1))
            continue
        m = SECTION.match(line)
        if m:
            cur = None
            continue
        if line.startswith(".ENDS"):
            cur = None
            continue
        m = LABEL.match(line)
        if m and not line.startswith(("\t", ";")):
            name = m.group(1)
            cur = dict(unit=unit, path=path, body=[], table=None, name=name)
            fns[name] = cur
            continue
        if cur is None:
            continue
        m = LOCAL.match(line)
        if m:
            cur["body"].append(("L", m.group(1)))
            continue
        m = ANON.match(line)
        if m:
            cur["body"].append(("A", m.group(1)))
            continue
        if line.lstrip().startswith((".dw", ".db", ".dl", ".DW", ".DB")):
            cur["body"].append(("D", line.strip()))
            continue
        m = INS.match(line)
        if m:
            if m.group(1):
                cur["body"].append(("A", m.group(1)))
            mn = m.group(2).lower()
            if mn in (".accu", ".index"):
                continue
            cur["body"].append(Ins(norm_mn(mn), (m.group(3) or "").lower(), (m.group(4) or "").strip()))


IRFN = re.compile(r"^(export\s+)?(?:inline\s+)?function\s+(?:\S+\s+)?\$([^\s(]+)\((.*)\)\s*\{")
IRCALL = re.compile(r"call \$([^\s(]+)\((.*)\)")
IRREF = re.compile(r"\$([A-Za-z_][A-Za-z0-9_.]*)")


def split_args(s):
    out = []
    for a in s.split(","):
        a = a.strip()
        if a:
            out.append(a)
    return out


def read_ir(path):
    """name -> dict(params=[cls], variadic, export); calls: [(caller, callee, [cls])]; refs: names used as values."""
    fns, calls, refs = {}, [], set()
    cur = None
    export_next = False
    for line in open(path, errors="replace"):
        s = line.strip()
        if s == "export":
            export_next = True
            continue
        m = IRFN.match(s)
        if m:
            args = split_args(m.group(3))
            var = any(a == "..." for a in args)
            params = [a.split()[0] for a in args if a != "..." and not a.startswith("env ")]
            cur = m.group(2)
            fns[cur] = dict(params=params, variadic=var, export=bool(m.group(1)) or export_next)
            export_next = False
            continue
        export_next = False if not s.startswith(("data", "type", "section")) else export_next
        if cur is None:
            continue
        if s == "}":
            cur = None
            continue
        m = IRCALL.search(s)
        rest = s
        if m:
            args = [a for a in split_args(m.group(2)) if a != "..."]
            calls.append((cur, m.group(1), [a.split()[0] for a in args], "..." in m.group(2)))
            rest = s[:m.start()] + s[m.start() + 5 + len(m.group(1)) + 1:]
        for r in IRREF.findall(rest):
            refs.add(r)
    return fns, calls, refs


# ---------------------------------------------------------------------------------------------- program model
class Program:
    def __init__(self, casm_units, ir_for_unit):
        """casm_units: [(unit, path)]; ir_for_unit: unit -> IR path or None."""
        self.fns, self.tcalls, self.addr = {}, [], collections.defaultdict(set)
        self.ir = {}
        self.ircalls = collections.defaultdict(list)   # unit -> [(caller, callee, classes, variadic)]
        self.missing_ir = []
        for unit, path in casm_units:
            read_casm(path, unit, self.fns, self.tcalls, self.addr)
            irp = ir_for_unit.get(unit)
            if irp and os.path.exists(irp):
                f, c, _ = read_ir(irp)
                self.ir[unit] = f
                self.ircalls[unit] = c
            else:
                self.missing_ir.append(unit)
        # C functions: a table line says it was emitted
        self.cfn = {n: f for n, f in self.fns.items() if f["table"] is not None}
        self.decide()

    def sig(self, name):
        f = self.cfn[name]
        return self.ir.get(f["unit"], {}).get(name)

    def decide(self):
        """Per C function: kind (A1 / A1-L / A1-W / no) and why, before the cost test."""
        self.why = {}
        for name, f in self.cfn.items():
            t = f["table"]
            s = self.sig(name)
            exported = t.get("export") == "1"
            if s is None:
                self.why[name] = ("noir", None)
                continue
            if not s["params"]:
                self.why[name] = ("noparams", None)
                continue
            if s["variadic"]:
                self.why[name] = ("variadic", None)
                continue
            first = s["params"][0]
            n = len(s["params"])
            calls = [c for c in self.ircalls[f["unit"]] if c[1] == name]
            agree = all(len(c[2]) == n and (c[2][0] == "l") == (first == "l") and not c[3] for c in calls)
            if not agree:
                self.why[name] = ("disagree", first)
                continue
            if exported:
                self.why[name] = ("exported", first)
            elif t.get("addrtaken") == "1":
                self.why[name] = ("addrtaken", first)
            elif first == "l":
                self.why[name] = ("kl_first", first)
            else:
                self.why[name] = ("A1", first)

    def param_bytes(self, name):
        s = self.sig(name)
        return sum(4 if p == "l" else 2 for p in s["params"])


def instrs(body):
    return [x for x in body if isinstance(x, Ins)]


def carve_of(body):
    """Frame carved at entry: `tsc / sec / sbc.w #F / tcs` among the first instructions (rep #$20 may precede)."""
    ins = instrs(body)
    for k in range(min(3, len(ins))):
        if (k + 3 < len(ins) and ins[k].mn == "tsc" and ins[k + 1].mn == "sec" and ins[k + 2].mn == "sbc"
                and ins[k + 3].mn == "tcs"):
            v = ins[k + 2].op.lstrip("#")
            return int(v[1:], 16) if v.startswith("$") else int(v), k + 4
    return 0, (1 if ins and ins[0].mn == "rep" else 0)


def imm(op):
    v = op.lstrip("#")
    try:
        return int(v[1:], 16) if v.startswith("$") else int(v)
    except ValueError:
        return None


def walk_depth(body, carve):
    """Yield (position in body, Ins, depth): bytes pushed since entry (carve included) before the instruction."""
    depth = 0
    seq = list(body)
    for p, x in enumerate(seq):
        if not isinstance(x, Ins):
            if x[0] == "L":
                depth = carve
            continue
        yield p, x, depth
        mn = x.mn
        if mn in PUSH_BYTES:
            depth += PUSH_BYTES[mn]
        elif mn in ("pla", "plx", "ply"):
            depth -= 2
        elif mn in ("plb", "plp"):
            depth -= 1
        elif mn == "pld":
            depth -= 2
        elif mn == "jml" and x.op.startswith("["):
            depth -= 3
        elif mn == "tcs":
            # tsc ; clc|sec ; adc|sbc #N ; tcs
            k = p - 1
            prev = []
            while k >= 0 and len(prev) < 3:
                if isinstance(seq[k], Ins):
                    prev.append(seq[k])
                k -= 1
            if len(prev) == 3 and prev[2].mn == "tsc" and prev[0].mn in ("adc", "sbc"):
                v = imm(prev[0].op)
                if v is not None:
                    depth += v if prev[0].mn == "sbc" else -v


# ---------------------------------------------------------------------------------------------- per callee
def callee_model(prog, name, kl=False):
    """Text facts of the callee: reads of the first parameter, head read, crossing, entry case."""
    f = prog.cfn[name]
    body = f["body"]
    t = f["table"]
    carve, start = carve_of(body)
    B = prog.param_bytes(name)
    if kl:
        offs = {B: "lo", B + 2: "hi"}      # entry-relative: low word at 0+B, high at B+2 (F = 0)
    else:
        offs = {2 + B: "w"}
    reads, writes, odd = [], [], []
    calls_pos = []
    targets = collections.Counter()
    pos_of = {}
    for x in body:
        if isinstance(x, Ins) and x.mn in FLOW and x.op:
            for lab in re.findall(r"@[A-Za-z0-9_.]+", x.op):
                targets[lab] += 1
    labels_at = {}
    for p, x in enumerate(body):
        if not isinstance(x, Ins) and x[0] == "L":
            labels_at[x[1]] = p
    for p, x, d in walk_depth(body, carve):
        pos_of[id(x)] = p
        if x.mn in ("jsl", "jsr") or (x.mn == "jml" and x.op.startswith("[")):
            calls_pos.append(p)
        m = SR.match(x.op or "")
        if not m:
            m2 = re.match(r"^\((\d+),s\),y$", x.op or "")
            if m2 and int(m2.group(1)) - d in offs:
                odd.append((p, repr(x)))
            continue
        e = int(m.group(1)) - d
        if e in offs:
            if x.mn in ("sta", "stz"):
                writes.append((p, x))
            else:
                reads.append((p, x, offs[e]))
        elif e - 1 in offs:
            odd.append((p, repr(x)))
    # head read: first instruction of the body after the carve, straight line, A untouched
    head = None
    ins_pos = [p for p, x in enumerate(body) if isinstance(x, Ins)]
    first_ins = ins_pos[start] if start < len(ins_pos) else None
    if first_ins is not None:
        p = first_ins
        while p < len(body):
            x = body[p]
            if not isinstance(x, Ins):
                if x[0] == "L" and targets[x[1]]:
                    break
                if x[0] == "A":
                    break
                p += 1
                continue
            hit = [r for r in reads if r[0] == p]
            if hit:
                if x.mn == "lda" and (not kl or hit[0][2] == "lo"):
                    head = p
                break
            if x.mn not in A_KEEPS:
                break
            p += 1
    # crossing: a read after a call in the text, or a read inside a loop (a backward branch) that contains a call
    branch_back = []
    for p, x in enumerate(body):
        if isinstance(x, Ins) and x.mn in FLOW and x.op:
            for lab in re.findall(r"@[A-Za-z0-9_.]+", x.op):
                lp = labels_at.get(lab)
                if lp is not None and lp < p:
                    branch_back.append((lp, p))
    cross = False
    for rp, x, _ in reads:
        if any(c < rp for c in calls_pos):
            cross = True
        for lp, bp in branch_back:
            if lp <= rp and any(rp < c < bp for c in calls_pos):
                cross = True
    leaf = t.get("leaf") == "1"
    lf = int(t.get("lf", "0"))
    # a leaf's all-in direct-page frame is not in `lf=` (its slots are not DPSLOT ones): read it from the text
    for x in body:
        if isinstance(x, Ins) and x.op:
            for mm in re.finditer(r"tcc__lf\+(\d+)", x.op):
                lf = max(lf, int(mm.group(1)) + 2)
            if re.search(r"tcc__lf(?![+\d])", x.op):
                lf = max(lf, 2)
    nonhead = [r for r in reads if r[0] != head]
    if not reads:
        case, entry = "unused", []
    elif not nonhead:
        case, entry = ("head_framed", ["TAX imp", "TXA imp"]) if carve else ("head", [])
    elif not cross and lf + (4 if kl else 2) <= 40:
        case, entry = "dp", ["STA dp"] * (2 if kl else 1)
        if kl:
            entry = ["STA dp", "STX dp"]
    elif carve:
        case, entry = "stack", (["TAX imp", "TXA imp", "STA sr"] if not kl else ["PHX imp", "PHA imp"])
    else:
        case = "stack_newframe" if not leaf else "leaf_full"
        entry = ["STA sr", "TSC imp", "SEC imp", "SBC imm16", "TCS imp", "TSC imp", "CLC imp", "ADC imm16",
                 "TCS imp"]
    cyc = {"unused": 0, "head": 0, "head_framed": 4, "dp": 4 * (2 if kl else 1), "stack": 9,
           "stack_newframe": 5 + CYC["frame"], "leaf_full": 5 + CYC["frame"]}[case]
    return dict(reads=reads, head=head, cross=cross, case=case, entry=entry, entry_cycles=cyc, carve=carve,
                writes=writes, odd=odd, leaf=leaf, lf=lf, B=B)


def read_change(x, part, case, is_head, kl=False):
    """(removed shape, added shape) of one read of the parameter."""
    old = f"{x.mn.upper()} sr"
    if is_head:
        if kl and part == "hi":
            return old, "TXA imp"
        return old, None
    if case == "dp":
        return old, f"{x.mn.upper()} dp"
    return None, None


# ---------------------------------------------------------------------------------------------- per call site
def site_model(prog, caller, p, callee, kl=False):
    """Text facts of one call site: the push of argument 1 and its replacement, the cleanup before and after."""
    body = prog.cfn[caller]["body"]
    B = prog.param_bytes(callee)
    n = len(prog.sig(callee)["params"])
    pushes, k, got = [], p - 1, 0
    while k >= 0 and got < B:
        x = body[k]
        if isinstance(x, Ins) and x.mn in PUSH_BYTES:
            pushes.append(k)
            got += PUSH_BYTES[x.mn]
        k -= 1
    issues = []
    if got != B:
        issues.append(f"pushes {got} != {B}")
        return None, issues
    pushes.reverse()
    arg1 = pushes[:2] if kl else pushes[:1]
    removed, added = [], []
    note = ""
    for q in arg1:
        x = body[q]
        removed.append(shape(x))
        if x.mn == "pea":
            added.append("LDX imm16" if (kl and q == arg1[0]) else "LDA imm16")
            note = "const"
        elif x.mn == "pha":
            # what A held: the instruction before the push
            prv = prev_ins(body, q)
            later_pushes = len(pushes) - pushes.index(q) - 1
            if kl and q == arg1[0]:
                # high half: into X instead of a push
                if prv is not None and prv.mn == "lda" and prv.op and not SR.match(prv.op) and "," not in prv.op:
                    removed.append(shape(prv))
                    added.append(shape(prv).replace("LDA", "LDX"))
                else:
                    added.append("TAX imp")
                    if later_pushes - 1 > 0:
                        added += ["STX dp", "LDX dp"]
                note = note or "kl"
                continue
            if later_pushes == 0:
                note = note or "pha_last"
                continue
            # argument 1 pushed first, others after: argument 1 must be in A at the call
            if prv is not None and prv.mn == "lda" and stable(body, prv, q, p) and not reads_a_after(body, q):
                note = note or "moved"
            elif prv is not None and prv.mn == "lda" and stable(body, prv, q, p):
                added.append(shape(prv))
                note = note or "reload"
            else:
                home = stored_home(body, q)
                if home is not None and stable(body, home, q, p):
                    added.append(shape(home).replace("STA", "LDA"))
                    note = note or "reload_home"
                else:
                    added += ["STA dp", "LDA dp"]
                    note = note or "spill"
        else:
            issues.append(f"arg1 push {x!r}")
            return None, issues
    # cleanup
    q = p + 1
    clean = []
    # at most B/2 of them: a frame popped by plx in the epilogue may follow at once (it stays)
    while q < len(body) and isinstance(body[q], Ins) and body[q].mn == "plx" and len(clean) * 2 < B:
        clean.append(q)
        q += 1
    old_clean, has_ret = [], None
    if clean:
        old_clean = ["PLX imp"] * len(clean)
        nplx = len(clean)
        if nplx * 2 != B:
            issues.append(f"cleanup {nplx * 2} != {B}")
        has_ret = True if nplx == 2 else None
    else:
        seq = []
        qq = p + 1
        while qq < len(body) and len(seq) < 6 and isinstance(body[qq], Ins):
            seq.append(body[qq])
            qq += 1
        mns = [x.mn for x in seq]
        if mns[:5] == ["tax", "tsc", "clc", "adc", "tcs"] and len(mns) > 5 and mns[5] == "txa":
            has_ret = True
            old_clean = ["TAX imp", "TSC imp", "CLC imp", "ADC imm16", "TCS imp", "TXA imp"]
            N = imm(seq[3].op)
        elif mns[:4] == ["tsc", "clc", "adc", "tcs"]:
            has_ret = False
            old_clean = ["TSC imp", "CLC imp", "ADC imm16", "TCS imp"]
            N = imm(seq[2].op)
        else:
            N = None
            issues.append("cleanup not recognised")
        if N is not None and N != B:
            issues.append(f"cleanup {N} != {B}")
    newb = B - (4 if kl else 2)
    new_clean = []
    if newb > 0:
        nplx = newb // 2
        if has_ret is None:
            has_ret_eff = False     # 1 plx today: void or result, the smaller limit is the safe reading
        else:
            has_ret_eff = has_ret
        limit = 2 if has_ret_eff else 1
        if nplx <= limit:
            new_clean = ["PLX imp"] * nplx
        else:
            new_clean = (["TAX imp"] if has_ret_eff else []) + ["TSC imp", "CLC imp", "ADC imm16", "TCS imp"] + \
                        (["TXA imp"] if has_ret_eff else [])
    return dict(n=n, B=B, push_removed=removed, push_added=added, clean_old=old_clean, clean_new=new_clean,
                note=note, pushes=pushes), issues


def prev_ins(body, q):
    k = q - 1
    while k >= 0:
        x = body[k]
        if isinstance(x, Ins):
            return x
        return None      # a label between: not the same straight line
    return None


def reads_a_after(body, q):
    """Does an instruction after the push read A before writing it (the A-cache kept argument 1)?"""
    k = q + 1
    while k < len(body):
        x = body[k]
        if not isinstance(x, Ins):
            return True
        if x.mn in ("pha", "sta", "adc", "sbc", "and", "ora", "eor", "cmp", "tax", "tay", "asl", "lsr", "rol",
                    "ror", "inc", "dec", "xba", "bit") and not (x.mn in ("inc", "dec", "asl", "lsr", "rol", "ror")
                                                               and x.op not in ("", "a", "A")):
            return True
        if x.mn in ("lda", "pla", "txa", "tya", "tdc", "tsc") or x.mn in ("jsl", "jml"):
            return False
        k += 1
    return False


def stored_home(body, q):
    """Walk back from the push while A is unchanged: the last `sta X` of that value (its slot), if any."""
    k = q - 1
    while k >= 0:
        x = body[k]
        if not isinstance(x, Ins):
            return None
        if x.mn == "sta" and x.op and "," not in x.op.replace(",s", ""):
            return x
        if x.mn not in A_KEEPS:
            return None
        k -= 1
    return None


def stable(body, ld, q, p):
    """Is the operand of `ld` (a load or a store) unchanged from the push q to the call p?"""
    op = ld.op or ""
    if op.startswith("#"):
        return True
    m = SR.match(op)
    target = None
    if m:
        target = ("sr", int(m.group(1)))
    elif "," in op or op.startswith(("(", "[")):
        if op.endswith(",x") or op.endswith(",y"):
            for k in range(q + 1, p):
                x = body[k]
                if isinstance(x, Ins) and (x.mn in X_WRITERS or x.mn in ("ldy", "tay", "iny", "dey", "ply")):
                    return False
            target = ("sym", op.split(",")[0])
        else:
            return False
    else:
        target = ("sym", op)
    depth = 0
    for k in range(q, p):
        x = body[k]
        if not isinstance(x, Ins):
            return False
        if x.mn in PUSH_BYTES:
            depth += PUSH_BYTES[x.mn]
        if x.mn in ("sta", "stz", "stx", "sty", "inc", "dec", "asl", "lsr", "rol", "ror", "tsb", "trb") and x.op:
            if target[0] == "sr":
                m2 = SR.match(x.op)
                if m2 and abs(int(m2.group(1)) - depth - target[1]) <= 1:
                    return False
                if x.op.startswith(("(", "[")) or ",x" in x.op:
                    return False
            else:
                base = re.sub(r"[+-]\d+$", "", target[1])
                if base in x.op or x.op.startswith(("(", "[")):
                    return False
        if x.mn in ("mvn", "mvp"):
            return False
    return True


def shape(x):
    mn = x.mn.upper()
    op = x.op or ""
    if not op or op in ("a", "A"):
        return f"{mn} imp"
    if op.startswith("#"):
        if mn in ("REP", "SEP"):
            return f"{mn} imm8"
        return f"{mn} imm{'8' if x.suf == 'b' else '16'}"
    if SR.match(op):
        return f"{mn} sr"
    if re.match(r"^\(\d+,s\),y$", op):
        return f"{mn} sr_indy"
    if mn == "PEA":
        return "PEA abs"
    if x.suf == "b" or op.startswith("tcc__"):
        if op.startswith("["):
            return f"{mn} dp_lind" + ("y" if op.endswith(",y") else "")
        return f"{mn} dp" + (",x" if op.endswith(",x") else "")
    if x.suf == "l":
        return f"{mn} long" + (",x" if op.endswith(",x") else "")
    return f"{mn} abs" + (",x" if op.endswith(",x") else ",y" if op.endswith(",y") else "")


# ---------------------------------------------------------------------------------------------- ROM placement
def norm_pc(pc):
    bank, off = pc >> 16, pc & 0xFFFF
    if off >= 0x8000 and (bank & 0x7F) not in (0x7E, 0x7F):
        return ((bank & 0x7F) << 16) | off
    return pc


def read_sym(path):
    labels = {}
    sec = None
    for line in open(path, errors="replace"):
        line = line.strip()
        if line.startswith("["):
            sec = line
            continue
        if sec != "[labels]" or not line or line.startswith(";"):
            continue
        ad, nm = line.split(None, 1)
        b, o = ad.split(":")
        labels[nm] = norm_pc((int(b, 16) << 16) | int(o, 16))
    return labels


def rom_offset(pc):
    bank, off = (pc >> 16) & 0x7F, pc & 0xFFFF
    return bank * 0x8000 + (off - 0x8000)


def place(prog, labels, rom):
    """Give every instruction of every C function its address; returns (placed functions, failures)."""
    ok, bad = 0, []
    for name, f in prog.cfn.items():
        if name not in labels:
            bad.append((name, "no label"))
            continue
        pc = labels[name]
        if (pc & 0xFFFF) < 0x8000:
            bad.append((name, "not in ROM"))
            continue
        m16, x16 = True, True
        err = None
        for x in f["body"]:
            if not isinstance(x, Ins):
                if x[0] == "L":
                    want = labels.get(f"{name}{x[1]}")
                    if want is not None and want != pc:
                        err = f"label {x[1]} at {pc:06X} != {want:06X}"
                        break
                if x[0] == "D":
                    break
                continue
            o = rom_offset(pc)
            mn, mode = OPS[rom[o]]
            if mn != x.mn:
                err = f"{x!r} at {pc:06X}: ROM has {mn}"
                break
            if mode in ("imm_m", "imm_x") and x.suf in ("w", "b"):
                ln = 3 if x.suf == "w" else 2
            else:
                ln = oplen(mode, m16, x16)
            x.pc = pc
            if mn in ("rep", "sep"):
                v = rom[o + 1]
                if v & 0x20:
                    m16 = mn == "rep"
                if v & 0x10:
                    x16 = mn == "rep"
            pc += ln
        if err:
            bad.append((name, err))
            for x in f["body"]:
                if isinstance(x, Ins):
                    x.pc = None
        else:
            ok += 1
    return ok, bad


TRACE_OP = [
    (re.compile(r"^#\$[0-9A-F]{4}$"), "imm16"), (re.compile(r"^#\$[0-9A-F]{2}$"), "imm8"),
    (re.compile(r"^\$[0-9A-F]{2},S$"), "sr"), (re.compile(r"^\(\$[0-9A-F]{2},S\),Y$"), "sr_indy"),
    (re.compile(r"^\$[0-9A-F]{2}$"), "dp"), (re.compile(r"^\$[0-9A-F]{2},X$"), "dp,x"),
    (re.compile(r"^\[\$[0-9A-F]{2}\]$"), "dp_lind"), (re.compile(r"^\[\$[0-9A-F]{2}\],Y$"), "dp_lindy"),
    (re.compile(r"^\$[0-9A-F]{4}$"), "abs"), (re.compile(r"^\$[0-9A-F]{4},X$"), "abs,x"),
    (re.compile(r"^\$[0-9A-F]{4},Y$"), "abs,y"), (re.compile(r"^\$[0-9A-F]{6}$"), "long"),
    (re.compile(r"^\$[0-9A-F]{6},X$"), "long,x"),
]


def trace_shape(mn, op):
    if not op or op == "A":
        return f"{mn} imp"
    for r, k in TRACE_OP:
        if r.match(op):
            if mn == "PEA":
                return "PEA abs"
            return f"{mn} {k}"
    return f"{mn} ?"


def read_trace(path, cpcs, frames=None):
    """Counts per normalized PC; busy master cycles (as the gauge's private tooling); median cost per shape in C."""
    rows = []
    for line in open(path):
        try:
            pcs, rest = line.rstrip("\n").split("  ", 1)
            dis, A, X, Y, SP, P, fr, ln, hc = rest.split("|")
        except ValueError:
            continue
        mn, _, op = dis.strip().partition(" ")
        fr = int(fr)
        if frames and not (frames[0] <= fr < frames[1]):
            continue
        rows.append((int(pcs, 16), mn.upper(), op.strip(), (fr * 262 + int(ln)) * 1364 + int(hc), fr))
    N = len(rows)
    cost = [0] * N
    typ = {}
    for i in range(N - 1):
        d = rows[i + 1][3] - rows[i][3]
        if d <= 0 or d > 400000 or rows[i + 1][4] - rows[i][4] > 1:
            d = typ.get(rows[i][0], 18)
        elif rows[i][1] != "WAI":
            typ[rows[i][0]] = d
        cost[i] = d
    cost[-1] = typ.get(rows[-1][0], 18)
    busy = sum(cost[i] for i in range(N) if rows[i][1] != "WAI")
    counts = collections.Counter()
    pcsum = collections.Counter()
    shapes = collections.defaultdict(list)
    for i, r in enumerate(rows):
        pc = norm_pc(r[0])
        counts[pc] += 1
        pcsum[pc] += cost[i]
        if pc in cpcs:
            shapes[trace_shape(r[1], r[2])].append(cost[i])
    med = {k: statistics.median(v) for k, v in shapes.items() if v}
    # M1's rule (L4 M1, the coherence check): executed jsl whose next row is in another C function of the same file
    return dict(rows=rows, cost=cost, busy=busy, counts=counts, pcsum=pcsum, med=med, N=N)


# ---------------------------------------------------------------------------------------------- the count
def price(shapes, med, fallback):
    s = 0
    for k in shapes:
        if k in med:
            s += med[k]
        else:
            fallback[k] += 1
            s += 6 * CYCLE_GUESS.get(k.split()[1], 3)
    return s


CYCLE_GUESS = {"imp": 2, "imm16": 3, "imm8": 2, "dp": 4, "sr": 5, "abs": 5, "long": 6}


def analyse(prog, tr=None, top=10, label=""):
    out = dict(label=label, missing_ir=prog.missing_ir)
    cfn = prog.cfn
    unit_of = {n: f["unit"] for n, f in cfn.items()}
    why = prog.why
    out["functions"] = len(cfn)
    out["why"] = dict(collections.Counter(w for w, _ in why.values()))
    # call sites from the text: every jsl / jml (direct) to a C function of the same unit
    sites = []      # (caller, pos, callee, kind jsl|jml)
    for name, f in cfn.items():
        for p, x in enumerate(f["body"]):
            if isinstance(x, Ins) and x.mn in ("jsl", "jml") and x.op and not x.op.startswith("["):
                tgt = x.op.split()[0]
                if tgt in cfn:
                    sites.append((name, p, tgt, x.mn, unit_of[tgt] == f["unit"]))
    med = tr["med"] if tr else {}
    cnt = tr["counts"] if tr else collections.Counter()
    fallback = collections.Counter()

    def c_of(name, p):
        x = cfn[name]["body"][p]
        return cnt.get(x.pc, 0) if (tr and x.pc is not None) else 0

    # coherence: executed jsl to a C function of the same unit (M1's part 2 count)
    same = [(c, p, t) for c, p, t, k, s in sites if k == "jsl" and s]
    out["sites_same_unit_jsl"] = len(same)
    out["exec_same_unit_jsl"] = sum(c_of(c, p) for c, p, t in same) if tr else None
    out["sites_other_unit_jsl"] = sum(1 for c, p, t, k, s in sites if k == "jsl" and not s)

    variants = {}
    issues = collections.Counter()
    issue_ex = {}
    for var in ("A1", "A1-L", "A1-W"):
        kl = var == "A1-L"
        elig = set()
        for name, (w, first) in why.items():
            if var == "A1" and w == "A1":
                elig.add(name)
            elif var == "A1-L" and w == "kl_first":
                elig.add(name)
            elif var == "A1-W" and w == "exported" and first != "l":
                elig.add(name)
        models = {n: callee_model(prog, n, kl=kl) for n in elig}
        site_rows = []
        per_fn = collections.defaultdict(lambda: dict(caller=0.0, callee=0.0, calls=0, ext=0, entries=0))
        for caller, p, tgt, kind, su in sites:
            if tgt not in elig or kind != "jsl" or not su:
                continue
            sm, iss = site_model(prog, caller, p, tgt, kl=kl)
            for i in iss:
                issues[(var, i.split()[0] + " " + i.split()[1] if " " in i else i)] += 1
                issue_ex.setdefault((var, i), f"{caller} -> {tgt}")
            if sm is None:
                continue
            ex = c_of(caller, p)
            g_push = price(sm["push_removed"], med, fallback) - price(sm["push_added"], med, fallback)
            g_clean = price(sm["clean_old"], med, fallback) - price(sm["clean_new"], med, fallback)
            site_rows.append(dict(caller=caller, callee=tgt, n=sm["n"], note=sm["note"], exec=ex,
                                  push=g_push * ex, clean=g_clean * ex,
                                  cyc_static=None))
            per_fn[tgt]["caller"] += (g_push + g_clean) * ex
            per_fn[tgt]["calls"] += ex
        # entries of each callee: every executed direct call (jsl + jml) to it; A1-W also from other units
        entries = collections.Counter()
        ext_entries = collections.Counter()
        for caller, p, tgt, kind, su in sites:
            if tgt in elig and su:
                entries[tgt] += c_of(caller, p)
        if tr:
            for n in elig:
                pc = cfn[n]["body"] and next((x.pc for x in cfn[n]["body"] if isinstance(x, Ins)), None)
                tot = cnt.get(pc, 0) if pc is not None else 0
                ext_entries[n] = max(0, tot - entries[n])
        dist = collections.Counter()
        accepted = set()
        for n in elig:
            mdl = models[n]
            nargs = len(prog.sig(n)["params"])
            dist[mdl["case"]] += 1
            s_static = CALLER_SAVING.get(nargs, 0) if not kl else CALLER_SAVING.get(nargs, 0) + 4
            if mdl["entry_cycles"] <= s_static:
                accepted.add(n)
            # callee side, per entry (all entries take the A body; an A1-W external entry adds its lda)
            g = 0.0
            for rp, x, part in mdl["reads"]:
                rem, add = read_change(x, part, mdl["case"], rp == mdl["head"], kl=kl)
                ex = cnt.get(x.pc, 0) if (tr and x.pc is not None) else 0
                if rem:
                    g += price([rem], med, fallback) * ex
                if add:
                    g -= price([add], med, fallback) * ex
            E = price(mdl["entry"], med, fallback)
            allent = entries[n] + ext_entries[n]
            per_fn[n]["reads"] = g
            per_fn[n]["entry"] = E * allent
            g -= E * allent
            if var == "A1-W":
                g -= price(["LDA sr"], med, fallback) * ext_entries[n]
            per_fn[n]["callee"] = g
            per_fn[n]["entries"] = entries[n]
            per_fn[n]["ext"] = ext_entries[n]
        busy = tr["busy"] if tr else None

        def pct(v):
            return 100.0 * v / busy if busy else None
        tot_all = sum(v["caller"] + v["callee"] for v in per_fn.values())
        tot_acc = sum(v["caller"] + v["callee"] for k, v in per_fn.items() if k in accepted)
        parts = dict(push=sum(r["push"] for r in site_rows if r["callee"] in accepted),
                     clean=sum(r["clean"] for r in site_rows if r["callee"] in accepted),
                     callee=sum(per_fn[k]["callee"] for k in accepted),
                     callee_reads=sum(per_fn[k].get("reads", 0) for k in accepted),
                     callee_entry=-sum(per_fn[k].get("entry", 0) for k in accepted))
        pos = sum(max(0.0, v["caller"] + v["callee"]) for v in per_fn.values())
        site_rows.sort(key=lambda r: -(r["push"] + r["clean"]))
        fnrank = sorted(((v["caller"] + v["callee"], k) for k, v in per_fn.items()), reverse=True)
        variants[var] = dict(
            eligible=len(elig), accepted=len(accepted),
            sites=len(site_rows), sites_executed=sum(1 for r in site_rows if r["exec"]),
            calls_executed=sum(r["exec"] for r in site_rows),
            entry_cases=dict(dist),
            accepted_entry_cases=dict(collections.Counter(models[n]["case"] for n in accepted)),
            pct_by_case=({c: pct(sum(per_fn[k]["caller"] + per_fn[k]["callee"] for k in accepted
                                     if models[k]["case"] == c))
                          for c in set(models[k]["case"] for k in accepted)} if busy else None),
            mclk_all=tot_all, mclk_accepted=tot_acc, mclk_positive_only=pos,
            pct_all=pct(tot_all), pct_accepted=pct(tot_acc), pct_positive_only=pct(pos),
            parts_accepted_pct={k: pct(v) for k, v in parts.items()},
            leaf_lf_full=sum(1 for n in elig if models[n]["case"] == "leaf_full"),
            top_sites=[dict(r, push_pct=pct(r["push"]), clean_pct=pct(r["clean"])) for r in site_rows[:top]],
            top_callees=[dict(fn=k, pct=pct(v), case=models[k]["case"], entries=per_fn[k]["entries"],
                              ext=per_fn[k]["ext"], reads=len(models[k]["reads"]),
                              caller_pct=pct(per_fn[k]["caller"]), callee_pct=pct(per_fn[k]["callee"]),
                              accepted=k in accepted)
                         for v, k in fnrank[:top]],
            notes=dict(collections.Counter(r["note"] for r in site_rows)),
            notes_exec=dict(collections.Counter({r["note"]: 0 for r in site_rows}) +
                            collections.Counter({k: v for k, v in _sum_by(site_rows, "note").items()})),
            odd=[(n, models[n]["odd"]) for n in elig if models[n]["odd"]][:10],
            writes=[n for n in elig if models[n]["writes"]],
            eligible_names=sorted(elig),
            accepted_names=sorted(accepted),
            sites_list=[(r["caller"], r["callee"], r["n"], r["note"], r["exec"]) for r in site_rows],
        )
    # A1-T: tail calls between a stack function and an A1 function (scope A1, accepted set)
    acc = set(variants["A1"]["accepted_names"])
    tq = collections.Counter()
    tq_exec = collections.Counter()
    for caller, p, tgt, kind, su in sites:
        if kind != "jml" or not su:
            continue
        q = ("A1" if caller in acc else "stack") + "->" + ("A1" if tgt in acc else "stack")
        tq[q] += 1
        tq_exec[q] += c_of(caller, p)
    out["tail"] = dict(sites=dict(tq), exec=dict(tq_exec))
    out["variants"] = variants
    out["issues"] = {f"{a}|{b}": v for (a, b), v in issues.items()}
    out["issue_examples"] = {f"{a}|{b}": v for (a, b), v in issue_ex.items()}
    out["fallback_shapes"] = dict(fallback)
    if tr:
        out["busy"] = tr["busy"]
        out["median_shapes"] = {k: tr["med"][k] for k in ("PHA imp", "PEA abs", "PLX imp", "LDA sr", "LDA dp",
                                                           "STA dp", "STA sr", "LDA imm16", "TAX imp", "TXA imp",
                                                           "ADC imm16", "TSC imp", "TCS imp", "CLC imp")
                                if k in tr["med"]}
    return out


def _sum_by(rows, key):
    c = collections.Counter()
    for r in rows:
        c[r[key]] += r["exec"]
    return c


def m1_rule(tr, prog, labels):
    """L4 M1's part-2 count on the same trace: an executed jsl whose next row lies in a C function of the caller's
    unit (function of a PC = the last label at or below it, local labels folded)."""
    addrs = sorted((a, n.split("@")[0]) for n, a in labels.items() if (a & 0xFFFF) >= 0x8000 or (a >> 16) >= 0x7E)
    A = [a for a, _ in addrs]
    Nm = [n for _, n in addrs]
    cache = {}

    def func(pc):
        pc = norm_pc(pc)
        r = cache.get(pc)
        if r is None:
            i = bisect.bisect_right(A, pc) - 1
            r = Nm[i] if i >= 0 else "?"
            cache[pc] = r
        return r
    unit = {n: f["unit"] for n, f in prog.fns.items()}
    rows = tr["rows"]
    n = 0
    for i in range(len(rows) - 1):
        if rows[i][1] != "JSL":
            continue
        f, g = func(rows[i][0]), func(rows[i + 1][0])
        if f in unit and g in unit and g in prog.fns and unit[f] == unit[g]:
            n += 1
    return n


# ---------------------------------------------------------------------------------------------- inputs
def ir_index(ir_dir):
    """IR files by their source path (cc65816 names them after the cwd and input path, separators as __)."""
    idx = {}
    for p in glob.glob(os.path.join(ir_dir, "*.ssa")):
        idx[os.path.basename(p)[:-4]] = p
    return idx


def ir_for(idx, src_hint):
    """The IR file whose flattened name ends with the source's last path parts."""
    parts = src_hint.strip("/").split("/")
    for k in range(len(parts), 0, -1):
        tail = "__".join(parts[-k:])
        hits = [v for key, v in idx.items() if key == tail or key.endswith("__" + tail)]
        if len(hits) == 1:
            return hits[0]
    return None


def units_of(casm_dirs, ir_dirs):
    units, irmap = [], {}
    idx = {}
    for d in ir_dirs:
        idx.update(ir_index(d))
    for d in casm_dirs:
        for p in sorted(glob.glob(os.path.join(d, "*.c.asm"))):
            base = os.path.basename(p)[:-len(".c.asm")]
            unit = os.path.join(d, base)
            units.append((unit, p))
            hint = os.path.join(os.path.abspath(d), base)
            if "/lib/build/" in hint:
                hint = hint.split("/lib/build/")[0] + "/lib/source/" + base
            irmap[unit] = ir_for(idx, hint.replace(os.sep, "/"))
    return units, irmap


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--casm-dir", action="append", default=[])
    ap.add_argument("--ir-dir", action="append", default=[])
    ap.add_argument("--corpus")
    ap.add_argument("--lib-casm")
    ap.add_argument("--rom")
    ap.add_argument("--sym")
    ap.add_argument("--trace")
    ap.add_argument("--json")
    ap.add_argument("--top", type=int, default=10)
    ap.add_argument("--label", default="")
    ap.add_argument("--frames", help="A-B: only the trace rows of frames A to B-1 (one gauge window)")
    a = ap.parse_args()
    if a.corpus:
        res = corpus(a)
    else:
        units, irmap = units_of(a.casm_dir, a.ir_dir)
        prog = Program(units, irmap)
        tr = None
        if a.trace:
            labels = read_sym(a.sym)
            rom = open(a.rom, "rb").read()
            ok, bad = place(prog, labels, rom)
            cpcs = set()
            for f in prog.cfn.values():
                for x in f["body"]:
                    if isinstance(x, Ins) and x.pc is not None:
                        cpcs.add(x.pc)
            fr = tuple(int(v) for v in a.frames.split("-")) if a.frames else None
            tr = read_trace(a.trace, cpcs, fr)
            for f in prog.cfn.values():
                for x in f["body"]:
                    if isinstance(x, Ins) and x.pc is not None:
                        x.count = tr["counts"].get(x.pc, 0)
        res = analyse(prog, tr, a.top, a.label)
        if tr:
            res["placed"] = ok
            res["place_failures"] = bad[:20]
            res["place_failures_n"] = len(bad)
            res["m1_rule_exec_same_unit_jsl"] = m1_rule(tr, prog, labels)
            placed_c = sum(1 for f in prog.cfn.values() for x in f["body"]
                           if isinstance(x, Ins) and x.pc is not None and x.count)
            res["c_instructions_executed_placed"] = placed_c
    report(res)
    if a.json:
        with open(a.json, "w") as fh:
            json.dump(res, fh, indent=1, default=str)


def corpus(a):
    out = dict(projects={}, totals=collections.Counter())
    lib = a.lib_casm
    for mk in sorted(glob.glob(os.path.join(a.corpus, "**", "Makefile"), recursive=True)):
        d = os.path.dirname(mk)
        if not glob.glob(os.path.join(d, "*.c.asm")):
            continue
        units, irmap = units_of([d, lib], a.ir_dir)
        prog = Program(units, irmap)
        r = analyse(prog, None, a.top, os.path.relpath(d, a.corpus))
        v = r["variants"]
        proj_units = {u for u, _ in units if not u.startswith(lib)}
        own = lambda names: [n for n in names if prog.cfn[n]["unit"] in proj_units]
        row = dict(eligible=own(v["A1"]["eligible_names"]), accepted=own(v["A1"]["accepted_names"]),
                   sites=[s for s in v["A1"]["sites_list"] if prog.cfn[s[0]]["unit"] in proj_units],
                   kl_first=own(v["A1-L"]["eligible_names"]), exported_kw=own(v["A1-W"]["eligible_names"]),
                   why={k: n for k, n in collections.Counter(
                       w for nm, (w, _) in prog.why.items() if prog.cfn[nm]["unit"] in proj_units).items()},
                   cases={k: n for k, n in collections.Counter(
                       callee_model(prog, nm)["case"] for nm in own(v["A1"]["eligible_names"])).items()},
                   missing_ir=[u for u in r["missing_ir"] if u in proj_units],
                   issues=r["issues"])
        out["projects"][r["label"]] = row
        for k in ("eligible", "accepted", "sites", "kl_first", "exported_kw"):
            out["totals"][k] += len(row[k])
        for k, n in row["why"].items():
            out["totals"]["why_" + k] += n
        for k, n in row["cases"].items():
            out["totals"]["case_" + k] += n
    # the library alone, once
    units, irmap = units_of([lib], a.ir_dir)
    prog = Program(units, irmap)
    r = analyse(prog, None, a.top, "lib")
    v = r["variants"]
    out["lib"] = dict(eligible=v["A1"]["eligible_names"], accepted=v["A1"]["accepted_names"],
                      sites=v["A1"]["sites_list"], why=r["why"], cases=v["A1"]["entry_cases"],
                      kl_first=v["A1-L"]["eligible_names"], missing_ir=r["missing_ir"], issues=r["issues"])
    out["totals"] = dict(out["totals"])
    return out


def report(res):
    if "projects" in res:
        t = res["totals"]
        print(f"corpus: {len(res['projects'])} projects; A1 eligible {t.get('eligible', 0)}, accepted by the cost "
              f"test {t.get('accepted', 0)}, call sites {t.get('sites', 0)}; Kl first {t.get('kl_first', 0)}; "
              f"exported Kw-first {t.get('exported_kw', 0)}")
        print("  why: " + ", ".join(f"{k[4:]} {v}" for k, v in sorted(t.items()) if k.startswith("why_")))
        print("  entry cases: " + ", ".join(f"{k[5:]} {v}" for k, v in sorted(t.items()) if k.startswith("case_")))
        L = res["lib"]
        print(f"lib: A1 eligible {len(L['eligible'])} accepted {len(L['accepted'])} sites {len(L['sites'])}; "
              f"why {L['why']}; cases {L['cases']}")
        return
    print(f"{res['label']}: {res['functions']} C functions; why {res['why']}")
    if res.get("missing_ir"):
        print(f"  units without IR: {len(res['missing_ir'])}: {res['missing_ir'][:5]}")
    if "busy" in res:
        print(f"  busy {res['busy']:,} mclk; placed {res['placed']} functions, failures {res['place_failures_n']}"
              f" {res['place_failures'][:3]}")
        print(f"  coherence: executed jsl to the same unit, text sites {res['exec_same_unit_jsl']:,}, M1's rule on "
              f"the trace {res['m1_rule_exec_same_unit_jsl']:,} ({res['sites_same_unit_jsl']} sites)")
    for var, v in res["variants"].items():
        p = v["pct_accepted"]
        print(f"  {var}: eligible {v['eligible']}, accepted {v['accepted']}, sites {v['sites']} "
              f"({v['sites_executed']} executed, {v['calls_executed']:,} calls); cases {v['entry_cases']}")
        if p is not None:
            q = v['parts_accepted_pct']
            print(f"      gain accepted {p:.3f} %  (push {q['push']:.3f}, cleanup {q['clean']:.3f}, callee "
                  f"{q['callee']:.3f} = reads {q['callee_reads']:.3f} + entry stores {q['callee_entry']:.3f}); "
                  f"all eligible {v['pct_all']:.3f} %; positive callees only {v['pct_positive_only']:.3f} %")
            print(f"      notes (exec) {v['notes_exec']}")
            for r in v["top_callees"][:5]:
                print(f"      {r['fn']:40s} {r['pct']:.3f} % case {r['case']} entries {r['entries']} ext {r['ext']}"
                      f" reads {r['reads']} {'acc' if r['accepted'] else 'refused'}")
    print(f"  tail calls same unit: {res['tail']}")
    if res["issues"]:
        print(f"  issues: {res['issues']}")
    if res.get("fallback_shapes"):
        print(f"  shapes priced by the fallback model: {res['fallback_shapes']}")


if __name__ == "__main__":
    main()
