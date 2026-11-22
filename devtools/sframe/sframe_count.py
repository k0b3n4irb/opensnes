#!/usr/bin/env python3
"""L4 S0: count what static frames would take, from any build (the side table is in every .c.asm since S1).

Maintainer tool (never in a user build). Reads the `; sframe` comment lines the backend writes after each function
(`compiler/qbe/w65816/emit.c`, `sframe_dump`) and the instruction lines of the same `.c.asm`, the library's `.c.asm`
and the assembly of the library, the templates and the project. Builds the program's call graph under three edge
models, decides the candidates and the refusals, places the accepted frames (each just past its deepest accepted
ancestor), and reports the deepest chain, the NMI-reachable frames and the window fits.

Edge models (`L4_plan_decoupe.md`, contest point 1):
  coarse   a callee no project `.c.asm` defines (library C or assembly, a runtime helper) is UNKNOWN; UNKNOWN reaches
           every exported or address-taken C function of the project; a call through a pointer reaches every
           address-taken function.
  closure  the library's C is read (its calls known); assembly is read routine by routine (a global label to the
           next; falling into the next label is an edge): its jsl/jml/jsr/jmp to a named routine are edges, an
           indirect jump (but through a table of its own local labels) or a pea/per not pulled back before an rtl/rts
           (a computed return; `pea $0000 / plb / plb` is not one) makes it
           reach UNKNOWN; the runtime helpers `__*` / `tcc_*` are closed by name (no edge); a callee nobody defines
           is UNKNOWN; UNKNOWN reaches every exported or address-taken C function (project and library).
  closure_rev  closure, with the SDK routines of REVIEWED (computed jumps read by hand, into their own code) closed.
  m2       M2's approximation: direct calls between C functions only (project and library), no edge for an
           unknown callee or a call through a pointer; refused when a candidate calls a C function of another file
           that reaches its own file again, or reaches itself.

Candidate (all models): the compiler's `cand=yes` (static, address never taken, not a leaf, no alloc, not a large
frame) and on no cycle of its own file's call graph. Refused (coarse, closure): the candidate is its own ancestor.

usage:
  sframe_count.py --project DIR [--project DIR ...] [--lib-casm DIR] [--asm PATH ...] [--sym FILE] [--json OUT]
  sframe_count.py --corpus EXAMPLES_DIR [--lib-casm DIR] [--asm PATH ...] [--json OUT]
Env SFRAME_COUNT_FAULT=no_unknown drops every UNKNOWN edge (the faulted run of the fixtures).
"""
import argparse
import collections
import glob
import json
import os
import re
import subprocess
import sys

WINDOWS = (88, 131, 164)
FAULT = os.environ.get("SFRAME_COUNT_FAULT", "")
UNK = "<unknown>"
LABEL = re.compile(r"^([A-Za-z_][A-Za-z0-9_.]*):")
INS = re.compile(r"^\s+(?:[+-]+\s+)?([A-Za-z]{2,4})(?:\.[bwlBWL])?(?:\s+(.*?))?\s*$")
IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_.]*")
CALLS = ("jsl", "jsr", "jml", "jmp")
ENDS_FLOW = ("rts", "rtl", "rti", "jmp", "jml", "bra", "brl", "stp")
HELPER = re.compile(r"^(__|tcc_)")
FNLINE = re.compile(r"^; sframe fn (\S+) (.*)$")
CALLLINE = re.compile(r"^; sframe call (\S+) (\S+) (\d+)$")
ADDRLINE = re.compile(r"^; sframe addr (\S+)$")
NOT_SYMBOLS = {"a", "x", "y", "s", "A", "X", "Y", "S"}
# closure_rev: SDK routines whose computed jump was read by hand on 2026-11-13 and goes only into their own code; the
# strict closure model counts them UNKNOWN. A count variant, not a rule: S1 decides whether such a list (or an
# annotation in the assembly) is delivered.
REVIEWED = {
    "memcpy": "string.asm: builds MVN + RTL on the stack and returns into it; `pea @back-1` brings it back",
    "memmove": "string.asm: memcpy's stub for the forward case, `jml memcpy` otherwise",
    "lzssDecodeRam": "lzss.asm: `jmp (tcc__r3h)` where tcc__r3h is one of #@cpslow / #@cp7e / #@cp7f, its own labels",
    "sprite_oamset.asm:_ohr_big": "sprite_oamset.asm: `pea _ohr_lo1-1` + `adc #_ohr_run-1` / rts, into its own store run",
    "sprite_oamset.asm:_ohr_lo1": "sprite_oamset.asm: `pea _ohr_lo2-1` + `adc #_ohr_run-1` / rts, into its own store run",
}


def strip_comment(line):
    out, q = [], None
    for ch in line:
        if q:
            out.append(ch)
            if ch == q:
                q = None
        elif ch in "\"'":
            q = ch
            out.append(ch)
        elif ch == ";":
            break
        else:
            out.append(ch)
    return "".join(out).rstrip()


class Fn:
    __slots__ = ("name", "file", "kind", "info", "ircalls", "body", "indirect", "tail", "argb", "tcd")

    def __init__(self, name, file, kind):
        self.name, self.file, self.kind = name, file, kind  # kind: proj | lib | asm
        self.info = {}
        self.ircalls = []          # (callee or '*', argbytes)
        self.body = set()          # call targets read from the instruction lines
        self.indirect = 0          # indirect calls / jumps read from the instruction lines
        self.tail = set()          # jml / jmp targets (tail calls)
        self.argb = 0
        self.tcd = False           # assembly that moves the direct page (H5)


def read_casm(path, kind, fns, addr):
    cur = None
    in_body = False
    expect = None
    for raw in open(path, errors="replace"):
        line = raw.rstrip("\n")
        if line.startswith("; Function: "):
            expect = line.split()[2]
            continue
        m = FNLINE.match(line)
        if m:
            f = fns.get(m.group(1))
            if f is not None:
                for kv in m.group(2).split():
                    k, _, v = kv.partition("=")
                    f.info[k] = int(v) if v.lstrip("-").isdigit() else v
            continue
        m = CALLLINE.match(line)
        if m:
            f = fns.get(m.group(1))
            if f is not None:
                f.ircalls.append((m.group(2), int(m.group(3))))
            continue
        m = ADDRLINE.match(line)
        if m:
            addr.add(m.group(1))
            continue
        if line.startswith(".ENDS"):
            in_body = False
            continue
        m = LABEL.match(line)
        if m and line[m.end():].strip() == "" and m.group(1) == expect:
            name = m.group(1)
            cur = fns.setdefault(name, Fn(name, path, kind))
            in_body = True
            expect = None
            continue
        if not in_body or cur is None:
            continue
        code = strip_comment(line)
        m = INS.match(code)
        if not m:
            continue
        mn, op = m.group(1).lower(), (m.group(2) or "").strip()
        if mn in CALLS:
            if op.startswith(("[", "(")):
                if not op.startswith("(@"):
                    cur.indirect += 1
            else:
                t = op.split()[0] if op else ""
                if t and not t.startswith(("@", "+", "-", ":")) and IDENT.fullmatch(t):
                    cur.body.add(t)
                    if mn in ("jml", "jmp"):
                        cur.tail.add(t)


def read_asm(path, routines, asm_refs, writes_nmi):
    """routine-level reading of hand-written assembly"""
    base = os.path.basename(path)
    cur = None
    last_mn = None
    recent = []
    in_code = True
    for raw in open(path, errors="replace"):
        code = strip_comment(raw.rstrip("\n"))
        if not code.strip():
            continue
        low = code.strip().lower()
        if low.startswith((".ramsection", ".struct", ".enum")):
            in_code = False
        elif low.startswith((".ends", ".ende")):
            in_code = True
        if low.startswith((".dw", ".dl", ".db", ".dsw", ".dsl", ".word", ".long", ".addr", ".faraddr")):
            for t in IDENT.findall(code.split(None, 1)[1] if len(code.split(None, 1)) > 1 else ""):
                asm_refs.add(t)
            continue
        m = LABEL.match(code)
        if m:
            name = m.group(1)
            rest = code[m.end():].strip()
            if name.startswith("_"):
                name = base + ":" + name
            if in_code:
                nxt = routines.setdefault(name, Fn(name, path, "asm"))
                if cur is not None and last_mn is not None and last_mn not in ENDS_FLOW:
                    cur.body.add(name)                        # falls into the next label
                cur = nxt
                recent = []
                last_mn = None
            if not rest:
                continue
            code = "\t" + rest
        if cur is None or not in_code:
            continue
        m = INS.match(code)
        if not m:
            continue
        mn, op = m.group(1).lower(), (m.group(2) or "").strip()
        if mn.startswith("."):
            continue
        last_mn = mn
        if mn == "tcd" or mn == "pld":
            cur.tcd = True
        if mn in CALLS:
            if op.startswith(("[", "(")):
                if not op.startswith("(@"):
                    cur.indirect += 1
            else:
                t = op.split()[0] if op else ""
                if t.startswith("_"):
                    t = base + ":" + t
                if t and not t.startswith(("@", "+", "-")) and (IDENT.fullmatch(t) or ":" in t):
                    cur.body.add(t)
        else:
            for t in IDENT.findall(op):
                if t not in NOT_SYMBOLS:
                    asm_refs.add(t)
            if mn in ("sta", "stx", "sty", "stz") and re.search(r"\bnmi_(lag_)?callback\b", op) \
                    and not path.endswith("templates/crt0.asm"):
                writes_nmi.append(f"{path}: {code.strip()}")
        if mn in ("pea", "per"):
            recent.append(2)                                  # bytes pushed, not yet pulled
        elif mn in ("plb",) and recent:
            recent[-1] -= 1
        elif mn in ("pla", "plx", "ply", "pld") and recent:
            recent[-1] -= 2
        recent = [b for b in recent if b > 0]
        if mn in ("rtl", "rts") and recent:
            cur.indirect += 1                                 # pea/per then return: a computed jump
            recent = []
        if mn in ("jsl", "jsr", "rtl", "rts"):
            recent = []


def asm_files(paths):
    out = []
    for p in paths:
        if os.path.isdir(p):
            for f in sorted(glob.glob(os.path.join(p, "*.asm"))):
                out.append(f)
        elif os.path.exists(p):
            out.append(p)
    return [f for f in out if not f.endswith((".c.asm", ".wrap.asm", ".spc700.asm"))]


def reach(succ, roots):
    seen, st = set(), list(roots)
    while st:
        x = st.pop()
        if x in seen:
            continue
        seen.add(x)
        st.extend(succ.get(x, ()))
    return seen


def path_back(succ, n):
    """a shortest call path from n back to n (the refusal's reason)"""
    par, st, seen = {}, collections.deque(), set()
    for c in succ.get(n, ()):
        if c not in seen:
            seen.add(c); par[c] = None; st.append(c)
    while st:
        x = st.popleft()
        if x == n:
            p, out = x, []
            while p is not None:
                out.append(p); p = par[p]
            return [n] + out[::-1]
        for c in succ.get(x, ()):
            if c not in seen:
                seen.add(c); par[c] = x; st.append(c)
    return []


def scc_cycles(nodes, succ):
    """nodes on a cycle of the graph restricted to `nodes` (Tarjan, iterative)"""
    index, low, onst, st, res, cnt = {}, {}, set(), [], set(), [0]
    for v0 in nodes:
        if v0 in index:
            continue
        work = [(v0, iter([w for w in succ.get(v0, ()) if w in nodes]))]
        index[v0] = low[v0] = cnt[0]; cnt[0] += 1; st.append(v0); onst.add(v0)
        while work:
            v, it = work[-1]
            adv = False
            for w in it:
                if w not in index:
                    index[w] = low[w] = cnt[0]; cnt[0] += 1; st.append(w); onst.add(w)
                    work.append((w, iter([u for u in succ.get(w, ()) if u in nodes])))
                    adv = True
                    break
                elif w in onst:
                    low[v] = min(low[v], index[w])
            if adv:
                continue
            work.pop()
            if work:
                low[work[-1][0]] = min(low[work[-1][0]], low[v])
            if low[v] == index[v]:
                comp = []
                while True:
                    w = st.pop(); onst.discard(w); comp.append(w)
                    if w == v:
                        break
                if len(comp) > 1 or v in succ.get(v, ()):
                    res.update(comp)
    return res


def analyse(proj_dirs, lib_dir, asm_paths, sym=None, label=""):
    fns, addr_c = {}, set()
    for d in proj_dirs:
        for f in sorted(glob.glob(os.path.join(d, "**", "*.c.asm"), recursive=True)):
            read_casm(f, "proj", fns, addr_c)
    proj = {n for n, f in fns.items() if f.kind == "proj"}
    lib = {}
    addr_lib = set()
    if lib_dir:
        for f in sorted(glob.glob(os.path.join(lib_dir, "*.c.asm"))):
            read_casm(f, "lib", lib, addr_lib)
    routines, asm_refs, writes_nmi = {}, set(), []
    for f in asm_files(asm_paths):
        read_asm(f, routines, asm_refs, writes_nmi)
    for n in list(routines):
        if n in fns or n in lib:
            del routines[n]                                  # a C function wins its name
    # project C functions that carry no dump line: the build was not made with QBE_SFRAME_DUMP=1
    nodump = sorted(n for n in proj if not fns[n].info)
    allc = dict(fns)
    allc.update({n: f for n, f in lib.items() if n not in fns})

    # consistency of the two sources inside a function: IR calls against instruction lines
    mism = []
    for n in proj:
        f = fns[n]
        ir = {c for c, _ in f.ircalls if c != "*"}
        body = {c for c in f.body if not HELPER.match(c)}
        nind = sum(1 for c, _ in f.ircalls if c == "*")
        if ir != body or (nind > 0) != (f.indirect > 0):
            mism.append((n, sorted(ir - body), sorted(body - ir), nind, f.indirect))

    # address-taken functions of the program
    funcnames = set(allc) | set(routines)
    addrtaken = (addr_c | addr_lib | asm_refs) & funcnames
    exported_proj = {n for n in proj if fns[n].info.get("export") == 1}
    exported_lib = {n for n, f in lib.items() if f.info.get("export") == 1 or f.info == {}}

    def calls_of(f):
        out = set(c for c, _ in f.ircalls if c != "*") | set(f.body)
        return out, (f.indirect > 0 or any(c == "*" for c, _ in f.ircalls))

    models = {}
    for model in ("coarse", "closure", "closure_rev", "m2"):
        succ = collections.defaultdict(set)
        if model == "coarse":
            nodes = set(proj)
            for n in proj:
                cs, ind = calls_of(fns[n])
                for c in cs:
                    succ[n].add(c if c in proj else UNK)
                if ind:
                    succ[n].update(addrtaken & proj)
            succ[UNK] = (exported_proj | (addrtaken & proj))
        elif model in ("closure", "closure_rev"):
            nodes = set(allc) | set(routines)
            for n, f in list(allc.items()) + list(routines.items()):
                if HELPER.match(n):
                    continue
                cs, ind = calls_of(f)
                for c in cs:
                    if HELPER.match(c):
                        continue
                    succ[n].add(c if c in nodes else UNK)
                if f.kind == "asm" and f.indirect:
                    if not (model == "closure_rev" and n in REVIEWED):
                        succ[n].add(UNK)
                elif ind:
                    succ[n].update(addrtaken)
            succ[UNK] = exported_proj | exported_lib | addrtaken
        else:
            nodes = set(allc)
            for n, f in allc.items():
                cs, _ = calls_of(f)
                succ[n].update(c for c in cs if c in nodes)
        if FAULT == "no_unknown":
            for k in list(succ):
                succ[k].discard(UNK)
            succ[UNK] = set()
        models[model] = succ

    # candidates: the compiler's yes, and on no cycle of the file's graph (direct project edges in one file)
    byfile = collections.defaultdict(set)
    for n in proj:
        byfile[fns[n].file].add(n)
    filesucc = {n: {c for c in calls_of(fns[n])[0] if c in proj and fns[c].file == fns[n].file} for n in proj}
    oncycle = set()
    for fl, ns in byfile.items():
        oncycle |= scc_cycles(ns, filesucc)
    reasons = collections.Counter()
    cand = []
    for n in sorted(proj):
        why = fns[n].info.get("cand", "nodump")
        if why == "yes" and n in oncycle:
            why = "cycle"
        reasons[why] += 1
        if why == "yes":
            cand.append(n)
    alloc_share = sum(1 for n in proj if fns[n].info.get("cand") in ("alloc", "dynalloc"))
    statics_nonleaf = sum(1 for n in proj if fns[n].info.get("export") == 0 and fns[n].info.get("addrtaken") == 0
                          and fns[n].info.get("leaf") == 0)

    # frame sizes: temps; temps + parameters (the callee's own count, or the largest argument block a caller sends)
    argmax = collections.Counter()
    for n, f in allc.items():
        for c, b in f.ircalls:
            argmax[c] = max(argmax[c], b)
    size_t = {n: fns[n].info.get("stack", 0) for n in cand}
    size_p = {n: fns[n].info.get("stack", 0) + max(fns[n].info.get("params", 0), argmax.get(n, 0)) for n in cand}
    tailcalls = {n: sorted(fns[n].tail) for n in cand if fns[n].tail}

    # NMI roots: literal arguments of nmiSet / nmiSetLag: the two instructions right before the jsl push the callback
    # (`pea.w :fn` / `pea.w fn`, the pattern check_nmi_race reads); `pea.w 0` twice is NULL (no callback); anything
    # else (a pointer computed and pushed by pha) is not literal and falls back to every address-taken and exported fn
    nmi_roots, nmi_nonlit = set(), []
    for path in sorted({f.file for f in allc.values()}):
        cur, last = None, []
        for raw in open(path, errors="replace"):
            line = strip_comment(raw.rstrip("\n"))
            m = LABEL.match(line)
            if m and line[m.end():].strip() == "":
                cur, last = m.group(1), []
                continue
            m = INS.match(line)
            if not m or cur is None:
                continue
            mn, op = m.group(1).lower(), (m.group(2) or "").strip()
            if mn in ("jsl", "jml") and op.split()[0:1] and op.split()[0] in ("nmiSet", "nmiSetLag"):
                ops = [o for (mm, o) in last[-2:] if mm == "pea"]
                names = {o.split()[0].lstrip(":") for o in ops}
                if len(ops) == 2 and len(names) == 1 and next(iter(names)) in funcnames:
                    nmi_roots.add(next(iter(names)))
                elif len(ops) == 2 and all(re.fullmatch(r"\$?0+", o.split()[0]) for o in ops):
                    pass                                      # NULL: no callback
                else:
                    nmi_nonlit.append(cur)
            last = (last + [(mn, op)])[-2:]
    irq_c = sorted(n for n in proj if "irqSet" in calls_of(fns[n])[0])     # refused at link (plan point 5)
    # H5: assembly that moves D and calls C (by name, or through a pointer that may hold a C function)
    h5 = sorted(f"{n} ({os.path.basename(r.file)}): " + ",".join(sorted(c for c in r.body if c in allc) +
                                                            (["<indirect>"] if r.indirect else []))
                for n, r in routines.items() if r.tcd and (r.indirect or any(c in allc for c in r.body)))
    nmi_fallback = bool(nmi_nonlit or writes_nmi)
    exported_all = exported_proj | exported_lib

    out = dict(label=label, functions=len(proj), lib_functions=len(lib), asm_routines=len(routines),
               nodump=nodump[:10], nodump_n=len(nodump), mismatch_n=len(mism), mismatch=mism[:10],
               reasons=dict(reasons), alloc_share=alloc_share, statics_nonleaf=statics_nonleaf,
               candidates=len(cand), cand_with_tailcall=len(tailcalls),
               nmi_roots=sorted(nmi_roots), nmi_nonliteral=nmi_nonlit, nmi_asm_writes=writes_nmi,
               nmi_fallback=nmi_fallback, irq_files=irq_c, h5_dp_moving_asm_calling_c=h5, models={})
    for model, succ in models.items():
        refused = {}
        for n in cand:
            r = reach(succ, succ.get(n, ()))
            if n in r:
                refused[n] = "re-entry" + (" via unknown" if UNK in r else "")
            elif model == "m2":
                for c in succ.get(n, ()):
                    if c in allc and allc[c].file != fns[n].file and \
                            any(x in allc and allc[x].file == fns[n].file for x in reach(succ, [c])):
                        refused[n] = "m2 out-and-back"
                        break
        acc = [n for n in cand if n not in refused]
        # ancestors among accepted: a reaches n
        anc = {n: set() for n in acc}
        reach_of = {a: reach(succ, succ.get(a, ())) for a in acc}
        for a in acc:
            for n in acc:
                if n != a and n in reach_of[a]:
                    anc[n].add(a)
        res = dict(refused=len(refused), refused_unknown=sum(1 for v in refused.values() if "unknown" in v),
                   refused_names=sorted(refused), accepted=acc, place={},
                   refused_paths={n: path_back(succ, n) for n in sorted(refused)})
        for tag, size in (("temps", size_t), ("params", size_p)):
            off = place(acc, anc, size)
            deep = max([off[n] + size[n] for n in acc] or [0])
            r = dict(deepest=deep, offsets=off, sizes={n: size[n] for n in acc})
            # NMI: accepted candidates an NMI root reaches (fallback: every address-taken and every exported function)
            roots = nmi_roots if not nmi_fallback else (addrtaken | exported_all)
            nmi_reach = reach(succ, roots) if roots else set()
            nmi_c = [n for n in acc if n in nmi_reach]
            r["nmi_candidates"] = len(nmi_c)
            r["nmi_extent"] = max([off[n] + size[n] for n in nmi_c] or [0])
            r["fits"] = {}
            for w in WINDOWS:
                placed = fit(acc, anc, size, w)
                r["fits"][w] = sorted(placed)
            res["place"][tag] = r
        out["models"][model] = res
    if sym:
        out["sym"] = sym_facts(sym)
    return out


def place(acc, anc, size):
    off, todo = {}, set(acc)
    while todo:
        ready = [n for n in todo if all(a in off for a in anc[n])]
        if not ready:
            raise SystemExit("internal: the accepted graph has a cycle")
        for n in ready:
            off[n] = max([off[a] + size[a] for a in anc[n]] or [0])
            todo.discard(n)
    return off


def fit(acc, anc, size, w):
    """drop the frames that end past the window, place again, until stable"""
    keep = set(acc)
    while True:
        sub = {n: anc[n] & keep for n in keep}
        off = place(sorted(keep), sub, size)
        bad = {n for n in keep if off[n] + size[n] > w}
        if not bad:
            return keep
        keep -= bad


def sym_facts(sym):
    snes = False
    for line in open(sym, errors="replace"):
        p = line.split()
        if len(p) == 2 and p[1] == "spc_ptr" and p[0].startswith("00:00"):
            snes = True
    facts = dict(snesmod=snes)
    here = os.path.dirname(os.path.abspath(__file__))
    try:
        r = subprocess.run([sys.executable, os.path.join(here, "..", "symmap", "symmap.py"), "--check-ram-budget", sym],
                           capture_output=True, text=True)
        m = re.search(r"C RAM band \$0000-\$1FFF: (\d+) bytes free", r.stdout + r.stderr)
        facts["plain_free"] = int(m.group(1)) if m else None
    except OSError:
        facts["plain_free"] = None
    return facts


def summary(o):
    print(f"[{o['label']}] C functions {o['functions']} (+ library {o['lib_functions']}, assembly routines "
          f"{o['asm_routines']}); not dumped {o['nodump_n']}; IR/text call mismatches {o['mismatch_n']}")
    print(f"  reasons: " + ", ".join(f"{k} {v}" for k, v in sorted(o["reasons"].items())))
    print(f"  candidates {o['candidates']} (static non-leaf {o['statics_nonleaf']}, of which with an alloc "
          f"{o['alloc_share']}); candidates with a tail call {o['cand_with_tailcall']}")
    print(f"  NMI roots {o['nmi_roots']} fallback={o['nmi_fallback']} non-literal {o['nmi_nonliteral']} "
          f"asm writes {len(o['nmi_asm_writes'])}; irqSet from C in {o['irq_files']}")
    if o["h5_dp_moving_asm_calling_c"]:
        print("  H5, assembly that moves D and calls C or jumps through a pointer: " + "; ".join(o["h5_dp_moving_asm_calling_c"]))
    for m, r in o["models"].items():
        pt, pp = r["place"]["temps"], r["place"]["params"]
        print(f"  {m:8s} refused {r['refused']} of {o['candidates']} (through UNKNOWN {r['refused_unknown']}); "
              f"deepest chain temps {pt['deepest']} B, temps+params {pp['deepest']} B; NMI-reachable "
              f"{pp['nmi_candidates']} ending at {pp['nmi_extent']} B; placed (params) in "
              + " / ".join(f"{w}: {len(pp['fits'][w])}" for w in WINDOWS))
    if "sym" in o:
        print(f"  SNESMOD linked: {o['sym']['snesmod']}; plain band free: {o['sym']['plain_free']} B")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--project", action="append", default=[])
    ap.add_argument("--corpus")
    ap.add_argument("--lib-casm")
    ap.add_argument("--asm", action="append", default=[])
    ap.add_argument("--sym")
    ap.add_argument("--json")
    ap.add_argument("--label", default="")
    ap.add_argument("--no-sdk-asm", action="store_true",
                    help="do not read the SDK's assembly (lib/source, lib/contrib, templates/crt0.asm)")
    a = ap.parse_args()
    if not a.no_sdk_asm:
        root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
        a.asm += [os.path.join(root, "lib", "source"), os.path.join(root, "lib", "contrib"),
                  os.path.join(root, "templates", "crt0.asm")]
    res = []
    if a.corpus:
        for mk in sorted(glob.glob(os.path.join(a.corpus, "**", "Makefile"), recursive=True)):
            d = os.path.dirname(mk)
            if not glob.glob(os.path.join(d, "*.c.asm")):
                continue
            syms = glob.glob(os.path.join(d, "*.sym"))
            o = analyse([d], a.lib_casm, a.asm + [d], syms[0] if syms else None, os.path.relpath(d, a.corpus))
            res.append(o)
            summary(o)
        tot = collections.Counter()
        for o in res:
            tot["functions"] += o["functions"]; tot["candidates"] += o["candidates"]
            tot["statics_nonleaf"] += o["statics_nonleaf"]; tot["alloc"] += o["alloc_share"]
            for m, r in o["models"].items():
                tot["refused_" + m] += r["refused"]
                tot["deep_" + m] = max(tot["deep_" + m], r["place"]["params"]["deepest"])
        print(f"[corpus] {len(res)} projects: " + ", ".join(f"{k} {v}" for k, v in tot.items()))
    else:
        o = analyse(a.project, a.lib_casm, a.asm + a.project, a.sym, a.label or ",".join(a.project))
        res.append(o)
        summary(o)
    if a.json:
        json.dump(res, open(a.json, "w"), indent=1, default=sorted)


if __name__ == "__main__":
    main()
