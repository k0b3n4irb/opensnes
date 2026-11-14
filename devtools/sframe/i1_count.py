#!/usr/bin/env python3
"""I1 S0: count what inlining the C functions of the same file into their callers would take (no compiler change).

Maintainer tool (never in a user build). Result: `.claude/notes/chantiers/gap_lots/I1_s0_count.md`.

Reuses `a1_count.py` (the side table, the text, the kept IR, the ROM placement, the trace). Per program:
  sites      every direct `jsl` (and, apart, `jml`: a tail call) of a C function to a C function of the SAME unit:
             by construction a call the compiler does not inline today.
  eligible   the callee is not exported (`export=0`), its address is never taken (`addrtaken=0` and not in the
             unit's `; sframe addr` lines), it is not recursive (not in a cycle of the unit's direct calls, itself
             included), not variadic (IR), and its IR is known. Nothing else: no size limit (the ROM cost is the
             price), an alloc kept (`alloc=1`) is reported apart (today's splicer refuses it).
  gain       per executed site, lower bound, no constant propagation, priced at the median measured cost of each
             shape in the same trace: the `jsl`, the argument pushes (`pha`, `pea`, `pei`, `phx`...; the loads
             feeding them stay), the cleanup (`plx`, or `[tax] tsc clc adc tcs [txa]`), and per entry of the callee
             (its executions shared by entry count): the frame carve at its head (`tsc sec sbc tcs`), `rep #$20` at
             its head, the frame undo before each `rtl` (`[tax] tsc clc adc tcs [txa]`), the `rtl`; a `jml` (tail
             call) executed in the callee becomes a `jsl` in the copy (+ JSL - JML). The callee's reads of its
             parameters are taken at their CURRENT cost (`lda n,s` read as an access to the argument's home).
  upper      (hypothesis, not the rule) the lower bound plus every read of a parameter in the callee free (the value
             already in A, or a constant folded into its use) and the load feeding each argument push free.
  ROM        bytes: the callee's body without its head and tail pieces, copied at each eligible site, minus the call
             sequences removed, minus the standalone (gone when every reference was a site of this unit). Two
             readings: one level (each callee's body as it is today), and nested (bottom-up, the copy of a callee
             holds the copies of ITS eligible callees: what today's splicer does).
  one level  the gain of the sites whose caller is not itself an inlined callee (a floor: in a strict one-level
             inlining, the calls inside a copied body stay calls), against all sites (nested).
With `--rom --sym --trace`, executions come from the trace (a1_count's placement: every text instruction at its
ROM address, checked against the `.sym`). Without, the census only (the corpus).

usage:
  i1_count.py --casm-dir DIR [--casm-dir DIR ...] --ir-dir DIR [--rom F --sym F --trace F [--frames A-B]] [--json OUT]
  i1_count.py --corpus EXAMPLES_DIR --lib-casm DIR --ir-dir DIR [--json OUT]
"""
import argparse
import collections
import glob
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import a1_count as A  # noqa: E402

Ins = A.Ins
BRANCH = {"bcc", "bcs", "beq", "bne", "bmi", "bpl", "bvc", "bvs", "bra"}
IDX16 = {"ldx", "ldy", "cpx", "cpy"}


def ins_len(x):
    """Bytes of one text instruction (16-bit A and X/Y; the suffix decides when present)."""
    mn, op, suf = x.mn, x.op or "", x.suf
    if not op or op in ("a", "A"):
        return 1
    if mn in BRANCH:
        return 2
    if mn in ("brl", "per"):
        return 3
    if mn in ("rep", "sep", "wdm", "cop", "brk"):
        return 2
    if mn in ("mvn", "mvp"):
        return 3
    if mn == "pea":
        return 3
    if mn == "pei":
        return 2
    if op.startswith("#"):
        return 2 if suf == "b" else 3
    if mn in ("jsl", "jml") and not op.startswith(("[", "(")):
        return 4
    if mn in ("jmp", "jsr", "jml"):
        return 3
    if A.SR.match(op) or re.match(r"^\(\d+,s\),y$", op):
        return 2
    if suf == "b" or op.startswith(("tcc__", "[")) or re.match(r"^\(?tcc__", op):
        return 2
    if suf == "l":
        return 4
    return 3


def body_bytes(f):
    """Size of a function: from the ROM placement when it has one, else the estimate."""
    ins = [x for x in f["body"] if isinstance(x, Ins)]
    if ins and all(x.pc is not None for x in ins):
        # the last instruction's length from the estimate (the ROM gives the others by difference)
        return ins[-1].pc - ins[0].pc + ins_len(ins[-1])
    return sum(ins_len(x) for x in ins)


def recursive_set(edges):
    """Functions in a cycle of the direct-call graph (self loops included): Tarjan."""
    idx, low, on, st, out = {}, {}, set(), [], set()
    n = [0]
    sys.setrecursionlimit(100000)

    def sc(v):
        idx[v] = low[v] = n[0]
        n[0] += 1
        st.append(v)
        on.add(v)
        for w in edges.get(v, ()):
            if w not in idx:
                sc(w)
                low[v] = min(low[v], low[w])
            elif w in on:
                low[v] = min(low[v], idx[w])
        if low[v] == idx[v]:
            comp = []
            while True:
                w = st.pop()
                on.discard(w)
                comp.append(w)
                if w == v:
                    break
            if len(comp) > 1 or v in edges.get(v, ()):
                out.update(comp)
    for v in list(edges):
        if v not in idx:
            sc(v)
    return out


def head_tail(body):
    """Positions of the callee's head and tail pieces: rep at the head, the carve, each undo + rtl, each jml."""
    ins_pos = [p for p, x in enumerate(body) if isinstance(x, Ins)]
    ins = [body[p] for p in ins_pos]
    head = []
    k = 0
    if ins and ins[0].mn == "rep":
        head.append(ins_pos[0])
        k = 1
    if (k + 3 < len(ins) and ins[k].mn == "tsc" and ins[k + 1].mn == "sec" and ins[k + 2].mn == "sbc"
            and ins[k + 3].mn == "tcs"):
        head += ins_pos[k:k + 4]
    tail, rtls, jmls = [], [], []
    for j, x in enumerate(ins):
        if x.mn == "rtl":
            rtls.append(ins_pos[j])
            # undo straight before: [tax] tsc clc adc tcs [txa]
            q = j - 1
            if q >= 0 and ins[q].mn == "txa":
                q -= 1
                wrap = True
            else:
                wrap = False
            if (q >= 3 and ins[q].mn == "tcs" and ins[q - 1].mn == "adc" and ins[q - 2].mn == "clc"
                    and ins[q - 3].mn == "tsc" and _straight(body, ins_pos[q - 3], ins_pos[j])):
                piece = ins_pos[q - 3:q + 1]
                if wrap:
                    if q - 4 >= 0 and ins[q - 4].mn == "tax" and _straight(body, ins_pos[q - 4], ins_pos[j]):
                        piece = [ins_pos[q - 4]] + piece + [ins_pos[j - 1]]
                    else:
                        piece = None
                if piece:
                    tail += piece
        elif x.mn == "jml" and x.op and not x.op.startswith("["):
            jmls.append(ins_pos[j])
    return head, tail, rtls, jmls


def _straight(body, a, b):
    """No label between positions a and b (one straight line)."""
    return all(isinstance(body[p], Ins) for p in range(a, b + 1))


def param_reads(prog, name):
    """Positions of the callee's reads of a parameter slot (entry-relative offset 4 .. 4 + B - 1)."""
    f = prog.cfn[name]
    body = f["body"]
    carve, _ = A.carve_of(body)
    B = prog.param_bytes(name)
    out = []
    for p, x, d in A.walk_depth(body, carve):
        m = A.SR.match(x.op or "") or re.match(r"^\((\d+),s\),y$", x.op or "")
        if not m:
            continue
        e = int(m.group(1)) - d
        if 3 <= e < 3 + B and x.mn not in ("sta", "stz"):
            out.append(p)
    return out


def call_seq(prog, caller, p, callee):
    """The call sequence of one site: (push positions, the loads feeding them, cleanup positions, issue)."""
    body = prog.cfn[caller]["body"]
    B = prog.param_bytes(callee)
    pushes, k, got = [], p - 1, 0
    while k >= 0 and got < B:
        x = body[k]
        if not isinstance(x, Ins):
            return None, "label inside the argument pushes"
        if x.mn in A.PUSH_BYTES:
            pushes.append(k)
            got += A.PUSH_BYTES[x.mn]
        elif x.mn in ("jsl", "jml", "jsr", "rtl", "tcs"):
            return None, "a call or a stack move inside the pushes"
        k -= 1
    if got != B:
        return None, f"pushes {got} != {B}"
    feeds = []
    for q in pushes:
        if body[q].mn == "pha":
            if q >= 1 and isinstance(body[q - 1], Ins) and body[q - 1].mn == "lda":
                feeds.append(q - 1)
    clean = []
    if B:
        q = p + 1
        while q < len(body) and isinstance(body[q], Ins) and body[q].mn == "plx" and len(clean) * 2 < B:
            clean.append(q)
            q += 1
        if clean and len(clean) * 2 != B:
            return None, f"cleanup {len(clean) * 2} != {B}"
        if not clean:
            seq = []
            q = p + 1
            while q < len(body) and len(seq) < 6 and isinstance(body[q], Ins):
                seq.append(q)
                q += 1
            mns = [body[q].mn for q in seq]
            if mns[:5] == ["tax", "tsc", "clc", "adc", "tcs"] and len(mns) > 5 and mns[5] == "txa":
                n = A.imm(body[seq[3]].op)
                clean = seq[:6]
            elif mns[:4] == ["tsc", "clc", "adc", "tcs"]:
                n = A.imm(body[seq[2]].op)
                clean = seq[:4]
            else:
                return None, "cleanup not recognised"
            if n != B:
                return None, f"cleanup {n} != {B}"
    return dict(pushes=pushes, feeds=feeds, clean=clean, B=B), None


def analyse(prog, tr=None, label="", top=12, rom_scope=None, sel=None):
    cfn = prog.cfn
    unit_of = {n: f["unit"] for n, f in cfn.items()}
    med = tr["med"] if tr else {}
    cnt = tr["counts"] if tr else collections.Counter()
    fallback = collections.Counter()

    def P(shapes):
        return A.price(shapes, med, fallback) if tr else 0

    def c_at(name, p):
        x = cfn[name]["body"][p]
        return cnt.get(x.pc, 0) if (tr and x.pc is not None) else 0

    # direct calls of the unit (text), the graph, the address-taken set
    sites = []          # (caller, pos, callee, kind)
    edges = collections.defaultdict(set)
    for name, f in cfn.items():
        for p, x in enumerate(f["body"]):
            if isinstance(x, Ins) and x.mn in ("jsl", "jml") and x.op and not x.op.startswith(("[", "(")):
                tgt = x.op.split()[0]
                if tgt in cfn and unit_of[tgt] == f["unit"]:
                    sites.append((name, p, tgt, x.mn))
                    edges[name].add(tgt)
    rec = recursive_set(edges)
    why = {}
    for name, f in cfn.items():
        t = f["table"]
        s = prog.sig(name)
        if s is None:
            why[name] = "noir"
        elif t.get("export") == "1":
            why[name] = "exported"
        elif t.get("addrtaken") == "1" or name in prog.addr[f["unit"]]:
            why[name] = "addrtaken"
        elif name in rec:
            why[name] = "recursive"
        elif s["variadic"]:
            why[name] = "variadic"
        else:
            why[name] = "eligible"
    elig = {n for n, w in why.items() if w == "eligible"}
    called = {t for _, _, t, _ in sites}

    # per-callee pieces and per-entry prices
    pieces = {}
    for n in called:
        f = cfn[n]
        head, tail, rtls, jmls = head_tail(f["body"])
        body = f["body"]
        ins = [x for x in body if isinstance(x, Ins)]
        entries = (cnt.get(ins[0].pc, 0) if (tr and ins and ins[0].pc is not None) else 0)
        ht_m = sum(P([A.shape(body[q])]) * c_at(n, q) for q in head + tail)
        rtl_m = sum(P(["RTL imp"]) * c_at(n, q) for q in rtls)
        jml_m = sum((P(["JSL long"]) - P(["JML long"])) * c_at(n, q) for q in jmls)
        reads = param_reads(prog, n) if prog.sig(n) else []
        rd_m = sum(P([A.shape(body[q])]) * c_at(n, q) for q in reads)
        hb = sum(ins_len(body[q]) for q in head + tail) + len(rtls)   # bytes a copy does not carry
        extra_exits = max(0, len(rtls) - 1) * 3                        # each other return: a jump to the join
        pieces[n] = dict(entries=entries, ht=ht_m, rtl=rtl_m, jml=jml_m, reads=rd_m,
                         per_entry=(ht_m + rtl_m - jml_m) / entries if entries else 0.0,
                         per_entry_tail=(ht_m - jml_m) / entries if entries else 0.0,
                         reads_per_entry=rd_m / entries if entries else 0.0,
                         size=body_bytes(f), copy=body_bytes(f) - hb + extra_exits,
                         nhead=len(head), ntail=len(tail), nrtl=len(rtls), njml=len(jmls),
                         framed=any(body[q].mn == "tcs" for q in head),
                         alloc=f["table"].get("alloc") == "1", leaf=f["table"].get("leaf") == "1")

    rows, issues = [], collections.Counter()
    nsites_unit = collections.Counter(t for _, _, t, _ in sites)
    for caller, p, tgt, kind in sites:
        ex = c_at(caller, p)
        row = dict(caller=caller, callee=tgt, kind=kind, exec=ex, elig=tgt in elig, why=why[tgt])
        # variant W (not the rule): an exported or address-taken callee copied at its direct calls of the unit,
        # its standalone kept (what the other compiler does to the gapbench `call` routine, whose callee is global)
        if tgt in elig or why[tgt] in ("exported", "addrtaken"):
            cs, iss = call_seq(prog, caller, p, tgt)
            if cs is None:
                if tgt in elig:
                    issues[iss] += 1
                cs = dict(pushes=[], feeds=[], clean=[], B=prog.param_bytes(tgt))
                row["issue"] = iss
            body = cfn[caller]["body"]
            # a push goes only when the value already has a home the copy can read: a constant (`pea`), a frame
            # slot or an immediate loaded just before (`lda n,s` / `lda.b tcc__lf+N` / `lda #k`, then `pha`), a
            # direct-page word (`pei`); any other pushed value must be stored for the copy (`sta` / `stx` to a slot,
            # priced as a direct-page store), which a push of the same price replaced
            site_shapes = [A.shape(body[q]) for q in cs["clean"]]
            stores = []
            for q in cs["pushes"]:
                x = body[q]
                site_shapes.append(A.shape(x))
                if x.mn in ("pea", "pei"):
                    continue
                prv = body[q - 1] if q >= 1 and isinstance(body[q - 1], Ins) else None
                if (x.mn == "pha" and prv is not None and prv.mn == "lda"
                        and (A.SR.match(prv.op or "") or (prv.op or "").startswith(("#", "tcc__lf")))):
                    continue
                stores.append("STX dp" if x.mn == "phx" else "STY dp" if x.mn == "phy" else "STA dp")
            row["stores"] = len(stores)
            # a tail call: its jml goes, the callee's rtl stays (it is now the caller's own return)
            jsl_m = P(["JSL long"]) if kind == "jsl" else P(["JML long"])
            pc = pieces[tgt]
            g_site = (jsl_m + P(site_shapes) - P(stores)) * ex
            g_callee = (pc["per_entry"] if kind == "jsl" else pc["per_entry_tail"]) * ex
            g_low = g_site + g_callee
            g_up = g_low + pc["reads_per_entry"] * ex + P(stores) * ex + \
                sum(P([A.shape(body[q])]) for q in cs["feeds"]) * ex
            callb = sum(ins_len(body[q]) for q in cs["pushes"] + cs["clean"]) + 4
            row.update(nargs=len(prog.sig(tgt)["params"]), B=cs["B"], low=g_low, up=g_up, site=g_site,
                       callee_part=g_callee, call_bytes=callb, callee_size=pc["size"], callee_copy=pc["copy"],
                       callee_sites=nsites_unit[tgt], framed=pc["framed"], alloc=pc["alloc"])
        rows.append(row)

    # one level: drop sites whose caller is an eligible callee itself (its body would be copied with the call in it)
    inl = {r["callee"] for r in rows if r["elig"]}
    for r in rows:
        r["root_caller"] = r["caller"] not in inl

    # ROM: functions in the ROM only (rom_scope = names placed), else every function of the given units
    scope = rom_scope if rom_scope is not None else set(cfn)
    sites_of = collections.defaultdict(list)
    for r in rows:
        if r["elig"] and r["caller"] in scope:
            sites_of[r["callee"]].append(r)
    busy = tr["busy"] if tr else None

    def pct(v):
        return 100.0 * v / busy if busy else None
    whole = choose(inl, rows, pieces, scope, pct)
    one_level, nested = whole["rom_one_level"], whole["rom_nested"]

    E = [r for r in rows if r["elig"]]
    Ej = [r for r in E if r["kind"] == "jsl"]
    W = [r for r in rows if not r["elig"] and "low" in r and r["kind"] == "jsl"]
    out_w = dict(callees=len({r["callee"] for r in W}), sites=len(W), exec=sum(r["exec"] for r in W),
                 low_pct=pct(sum(r["low"] for r in W)),
                 rom=sum(r["callee_copy"] - r["call_bytes"] for r in W if r["caller"] in scope))
    out = dict(label=label, functions=len(cfn), missing_ir=prog.missing_ir,
               why_all=dict(collections.Counter(why.values())),
               why_called=dict(collections.Counter(why[n] for n in called)),
               sites_jsl=sum(1 for r in rows if r["kind"] == "jsl"),
               sites_jml=sum(1 for r in rows if r["kind"] == "jml"),
               eligible_callees=len({r["callee"] for r in E}),
               eligible_sites_jsl=len(Ej), eligible_sites_jml=len(E) - len(Ej),
               eligible_alloc_callees=sorted({r["callee"] for r in E if r["alloc"]}),
               issues=dict(issues), rom_one_level=one_level, rom_nested=nested,
               fallback_shapes=dict(fallback), variant_w=out_w)
    # callee size distribution (bytes) and sites per callee
    sz = sorted(pieces[g]["size"] for g in {r["callee"] for r in E})
    out["callee_size_quartiles"] = [sz[int(q * (len(sz) - 1))] for q in (0, .25, .5, .75, 1)] if sz else []
    out["callee_sites_dist"] = dict(collections.Counter(min(nsites_unit[g], 9) for g in {r["callee"] for r in E}))
    if tr:
        exec_same = sum(r["exec"] for r in rows if r["kind"] == "jsl")
        out.update(busy=busy, exec_same_unit_jsl=exec_same,
                   exec_same_unit_jml=sum(r["exec"] for r in rows if r["kind"] == "jml"),
                   exec_eligible_jsl=sum(r["exec"] for r in Ej),
                   exec_by_why=dict(_sum(rows, "why", lambda r: r["kind"] == "jsl")),
                   low_pct=pct(sum(r["low"] for r in Ej)), up_pct=pct(sum(r["up"] for r in Ej)),
                   low_pct_with_jml=pct(sum(r["low"] for r in E)),
                   low_pct_one_level=pct(sum(r["low"] for r in Ej if r["root_caller"])),
                   site_part_pct=pct(sum(r["site"] for r in Ej)),
                   callee_part_pct=pct(sum(r["callee_part"] for r in Ej)),
                   low_pct_no_alloc=pct(sum(r["low"] for r in Ej if not r["alloc"])),
                   mean_low_per_call=(sum(r["low"] for r in Ej) / max(1, sum(r["exec"] for r in Ej))),
                   by_nargs={k: dict(calls=v[0], pct=pct(v[1])) for k, v in _by(Ej, "nargs").items()},
                   by_framed={str(k): dict(calls=v[0], pct=pct(v[1])) for k, v in _by(Ej, "framed").items()},
                   median_shapes={k: med[k] for k in ("JSL long", "RTL imp", "JML long", "PHA imp", "PEA abs",
                                                      "PLX imp", "TSC imp", "TCS imp", "SEC imp", "CLC imp",
                                                      "SBC imm16", "ADC imm16", "LDA sr") if k in med})
        # per callee: gain, executed calls, sites, size; and the ROM it costs alone (one level)
        per = collections.defaultdict(lambda: dict(low=0.0, up=0.0, calls=0))
        for r in Ej:
            per[r["callee"]]["low"] += r["low"]
            per[r["callee"]]["up"] += r["up"]
            per[r["callee"]]["calls"] += r["exec"]
        ranked = sorted(per.items(), key=lambda kv: -kv[1]["low"])
        out["top_callees"] = [dict(fn=k, low_pct=pct(v["low"]), up_pct=pct(v["up"]), calls=v["calls"],
                                   sites=nsites_unit[k], size=pieces[k]["size"], framed=pieces[k]["framed"],
                                   rom=sum(pieces[k]["copy"] - r["call_bytes"] for r in sites_of.get(k, []))
                                   - (pieces[k]["size"] if k in scope else 0))
                              for k, v in ranked[:top]]
        # the cumulative curve: callees by gain per ROM byte, what a budget would buy
        cur, acc_g, acc_b = [], 0.0, 0
        for k, v in sorted(per.items(), key=lambda kv: -(kv[1]["low"] / max(1, pieces[kv[0]]["size"]))):
            b = sum(pieces[k]["copy"] - r["call_bytes"] for r in sites_of.get(k, [])) - \
                (pieces[k]["size"] if k in scope else 0)
            acc_g += v["low"]
            acc_b += b
            cur.append((k, pct(acc_g), acc_b))
        out["curve"] = cur
        out["executed_eligible_callees"] = sum(1 for v in per.values() if v["calls"])
        # a compiler's choice: every eligible callee up to a size, nested (bottom-up) and one level
        out["by_size"] = []
        allel = {r["callee"] for r in E}
        for lim in (16, 32, 64, 128, 256, 512, 1024, 10 ** 9):
            S = {g for g in allel if pieces[g]["size"] <= lim}
            out["by_size"].append(dict(limit=lim, callees=len(S), **choose(S, rows, pieces, scope, pct)))
        # and the profile's order (not decidable by a compiler): the smallest set that reaches 1.5 % one level
        S, best = set(), None
        for k, v in sorted(per.items(), key=lambda kv: -(kv[1]["low"] / max(1, pieces[kv[0]]["size"]))):
            S.add(k)
            c = choose(S, rows, pieces, scope, pct)
            if c["low_one_level"] >= 1.5:
                best = dict(callees=len(S), **c)
                break
        out["profile_1p5"] = best
    # a given set (a recompiled check marks these `inline`), and the project's eligible callees with their size
    if sel is not None:
        S = {g for g in sel if g in inl}
        out["set"] = dict(asked=len(sel), eligible=len(S), **choose(S, rows, pieces, scope, pct))
    out["eligible_sizes"] = {g: dict(size=pieces[g]["size"], unit=cfn[g]["unit"]) for g in inl}
    return out


def choose(S, rows, pieces, scope, pct):
    """Gain (lower) and ROM of inlining the callees of S at every eligible jsl site: nested (bottom-up: a copy holds
    the copies of its own callees in S) and one level (a site inside a callee of S stays a call)."""
    el = [r for r in rows if r["elig"] and r["kind"] == "jsl" and r["callee"] in S]
    low_n = sum(r["low"] for r in el)
    low_1 = sum(r["low"] for r in el if r["caller"] not in S)
    by_caller = collections.defaultdict(list)
    for r in rows:
        if r["elig"] and r["callee"] in S:
            by_caller[r["caller"]].append(r)
    memo = {}

    def eff(g):
        if g not in memo:
            memo[g] = 0
            v = pieces[g]["copy"]
            for r in by_caller.get(g, []):
                v += eff(r["callee"]) - r["call_bytes"]
            memo[g] = v
        return memo[g]
    rom_n = 0
    for c, rs in by_caller.items():
        if c in S or c not in scope:
            continue
        rom_n += sum(eff(r["callee"]) - r["call_bytes"] for r in rs)
    rom_n -= sum(pieces[g]["size"] for g in S if g in scope)
    rom_1, kept = 0, set()
    for c, rs in by_caller.items():
        if c not in scope:
            continue
        for r in rs:
            if c in S:
                kept.add(r["callee"])          # the copies of c call it: its standalone stays
            else:
                rom_1 += pieces[r["callee"]]["copy"] - r["call_bytes"]
    rom_1 -= sum(pieces[g]["size"] for g in S if g in scope and g not in kept)
    return dict(low_nested=pct(low_n), low_one_level=pct(low_1), rom_nested=rom_n, rom_one_level=rom_1)


def _sum(rows, key, cond):
    c = collections.Counter()
    for r in rows:
        if cond(r):
            c[r[key]] += r["exec"]
    return c


def _by(rows, key):
    d = collections.defaultdict(lambda: [0, 0.0])
    for r in rows:
        d[r[key]][0] += r["exec"]
        d[r[key]][1] += r["low"]
    return dict(d)


def corpus(a):
    out = dict(projects={}, totals=collections.Counter())
    lib = a.lib_casm
    for mk in sorted(glob.glob(os.path.join(a.corpus, "**", "Makefile"), recursive=True)):
        d = os.path.dirname(mk)
        if not glob.glob(os.path.join(d, "*.c.asm")):
            continue
        units, irmap = A.units_of([d], a.ir_dir)
        prog = A.Program(units, irmap)
        r = analyse(prog, None, os.path.relpath(d, a.corpus))
        out["projects"][r["label"]] = {k: r[k] for k in ("sites_jsl", "sites_jml", "eligible_callees",
                                                        "eligible_sites_jsl", "rom_one_level", "rom_nested",
                                                        "why_called", "issues", "missing_ir")}
        for k in ("sites_jsl", "sites_jml", "eligible_callees", "eligible_sites_jsl", "eligible_sites_jml",
                  "rom_one_level", "rom_nested"):
            out["totals"][k] += r[k]
        for k, v in r["why_called"].items():
            out["totals"]["called_" + k] += v
    units, irmap = A.units_of([lib], a.ir_dir)
    prog = A.Program(units, irmap)
    out["lib"] = analyse(prog, None, "lib")
    out["totals"] = dict(out["totals"])
    return out


def report(res):
    if "projects" in res:
        t = res["totals"]
        print(f"corpus: {len(res['projects'])} projects; same-file sites jsl {t.get('sites_jsl', 0)} jml "
              f"{t.get('sites_jml', 0)}; eligible callees {t.get('eligible_callees', 0)}, eligible jsl sites "
              f"{t.get('eligible_sites_jsl', 0)} (jml {t.get('eligible_sites_jml', 0)}); ROM one level "
              f"{t.get('rom_one_level', 0):+d} B, nested {t.get('rom_nested', 0):+d} B")
        print("  called callees by reason: " + ", ".join(f"{k[7:]} {v}" for k, v in sorted(t.items())
                                                        if k.startswith("called_")))
        L = res["lib"]
        print(f"lib alone: sites jsl {L['sites_jsl']} jml {L['sites_jml']}; eligible callees {L['eligible_callees']}"
              f" sites {L['eligible_sites_jsl']}; why called {L['why_called']}; ROM one level {L['rom_one_level']:+d}"
              f" nested {L['rom_nested']:+d}")
        return
    r = res
    print(f"{r['label']}: {r['functions']} C functions; same-file sites jsl {r['sites_jsl']} jml {r['sites_jml']}; "
          f"callees by reason {r['why_called']}")
    print(f"  eligible callees {r['eligible_callees']}, sites jsl {r['eligible_sites_jsl']} jml "
          f"{r['eligible_sites_jml']}; with an alloc {len(r['eligible_alloc_callees'])}; issues {r['issues']}")
    print(f"  callee bytes quartiles {r['callee_size_quartiles']}; sites per callee {r['callee_sites_dist']}")
    print(f"  ROM: one level {r['rom_one_level']:+,d} B, nested {r['rom_nested']:+,d} B")
    if "busy" in r:
        print(f"  busy {r['busy']:,} mclk; placed {r.get('placed')} failures {r.get('place_failures_n')}")
        print(f"  coherence: executed same-file jsl {r['exec_same_unit_jsl']:,} (M1's rule on the trace "
              f"{r.get('m1_rule'):,}); jml {r['exec_same_unit_jml']:,}; by reason {r['exec_by_why']}")
        print(f"  eligible executed jsl {r['exec_eligible_jsl']:,}; mean lower gain {r['mean_low_per_call']:.1f} mclk"
              f"/call")
        print(f"  LOWER {r['low_pct']:.3f} % (site {r['site_part_pct']:.3f} + callee {r['callee_part_pct']:.3f}); "
              f"one level {r['low_pct_one_level']:.3f} %; without alloc callees {r['low_pct_no_alloc']:.3f} %; "
              f"with the jml sites {r['low_pct_with_jml']:.3f} %; UPPER (hypothesis) {r['up_pct']:.3f} %")
        print(f"  by arity {r['by_nargs']}")
        print(f"  framed callee {r['by_framed']}")
        print(f"  medians {r['median_shapes']}")
        for c in r["top_callees"]:
            print(f"    {c['fn']:36s} {c['low_pct']:.3f} % (up {c['up_pct']:.3f})  calls {c['calls']:6d} sites "
                  f"{c['sites']:3d} size {c['size']:5d} framed {int(c['framed'])} ROM {c['rom']:+d}")
        for c in r["by_size"]:
            print(f"    callees <= {c['limit']:>10} B: {c['callees']:3d}; nested {c['low_nested']:.3f} % ROM "
                  f"{c['rom_nested']:+,d} B; one level {c['low_one_level']:.3f} % ROM {c['rom_one_level']:+,d} B")
        print(f"    profile order, smallest set at 1.5 % one level: {r['profile_1p5']}")
        print(f"    variant W (exported / address-taken callees copied, standalone kept): {r['variant_w']}")
        if "set" in r:
            print(f"    given set: {r['set']}")
    if r.get("fallback_shapes"):
        print(f"  shapes priced by the fallback model: {r['fallback_shapes']}")


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
    ap.add_argument("--label", default="")
    ap.add_argument("--top", type=int, default=12)
    ap.add_argument("--frames")
    ap.add_argument("--set", help="a file of callee names: their gain and ROM if inlined (the recompiled check)")
    ap.add_argument("--write-set", nargs=2, metavar=("BYTES", "PATH"),
                    help="write the project's eligible callees up to BYTES to PATH")
    a = ap.parse_args()
    if a.corpus:
        res = corpus(a)
    else:
        units, irmap = A.units_of(a.casm_dir, a.ir_dir)
        prog = A.Program(units, irmap)
        tr, scope, labels = None, None, None
        if a.trace:
            labels = A.read_sym(a.sym)
            rom = open(a.rom, "rb").read()
            ok, bad = A.place(prog, labels, rom)
            scope = {n for n, f in prog.cfn.items()
                     if any(isinstance(x, Ins) and x.pc is not None for x in f["body"])}
            cpcs = {x.pc for f in prog.cfn.values() for x in f["body"] if isinstance(x, Ins) and x.pc is not None}
            fr = tuple(int(v) for v in a.frames.split("-")) if a.frames else None
            tr = A.read_trace(a.trace, cpcs, fr)
        sel = set(open(a.set).read().split()) if a.set else None
        res = analyse(prog, tr, a.label, a.top, scope, sel)
        if a.write_set:
            lim, path = int(a.write_set[0]), a.write_set[1]
            names = sorted(g for g, v in res["eligible_sizes"].items()
                           if v["size"] <= lim and "/lib/build/" not in v["unit"])
            with open(path, "w") as fh:
                fh.write("\n".join(names) + "\n")
            print(f"  wrote {len(names)} names (project units, <= {lim} B) to {path}")
        if tr:
            res["placed"], res["place_failures_n"] = ok, len(bad)
            res["m1_rule"] = A.m1_rule(tr, prog, labels)
    report(res)
    if a.json:
        with open(a.json, "w") as fh:
            json.dump(res, fh, indent=1, default=str)


if __name__ == "__main__":
    main()
