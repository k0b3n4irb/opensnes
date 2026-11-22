#!/usr/bin/env python3
"""I1 S2: calibrate the size rule of the inliner before it exists (no compiler change; reads a trace and a text).

Maintainer tool (never in a user build). Plan: `.claude/notes/chantiers/gap_lots/I1_inline_plan.md` (S2); result:
`I1_s2_result.md` beside it. Reuses `a1_count.py` (the text reader) and `i1_count.py` (the byte estimate).

Three subcommands:

  compile  --qbe BIN --ir-dir DIR --out DIR [--short]
           runs `qbe -t w65816` on every kept IR file (`CC65816_KEEP_IR`) with `CC_TRACE_INLINE=1`, writing
           `UNIT.c.asm` and `UNIT.trace` side by side: the trace and the text come from the same compile. `--short`
           names a unit by its file stem (one program); otherwise by the whole kept name (the corpus, where stems
           repeat).

  report   --dir DIR [--dir DIR ...] [--bpi F] [--callseq B] [--write-set N PATH] [--json OUT]
           per function: own IR size (`record ins=`, before its own inlining), final IR size (`final ins=`, after),
           sites, static, address taken, section, `inline`, absorbed, and its bytes in the text (the estimate of
           `i1_count.py`, jump tables counted: two bytes a `.dw`). Gives:
             - the decline reasons of auto_inline_ok / splice_fn (`[inline] decline` lines), one-site callees apart,
               and the temporaries counted against the temporaries used in each declining caller;
             - bytes per IR instruction over the functions that absorbed nothing (own = final), the median and the
               quartiles, and the separator table `own <= N` against `bytes <= 256`;
             - the same on the size rule's candidates (static, address never taken, no section of their own, two
               sites or more, no `inline`, not recursive, every site declined for its site count alone), with
               their FINAL size: `own <= N` and `final <= N` against `final bytes <= 256` (hypothesis H5);
             - free copies (own <= 6) against a call sequence, and the callees holding a jump table;
             - the planning estimate of the plan (1.3) for the candidates with own <= N_SIZE: nested final size,
               cost per callee, growth per unit and per caller, against the budgets (32 768 / 8 192 / 16 384).
           `--write-set N PATH` writes the candidates with own <= N (one name a line), for `i1_count.py --set`;
           `--write-final-set N PATH` those with final <= N (hypothesis H5).

  growth   --before-sym F --before-dir D --after-sym F --after-dir D [--json OUT]
           code bytes per unit and per function from the `.text.*` sections of two linked `.sym` files (a section
           is mapped to its unit by the `.SECTION ".text.NAME"` line of that unit's `.c.asm` or `.c.wrap.asm`):
           growth per unit, the largest growths per function, the largest section after.
"""
import argparse
import collections
import glob
import json
import os
import re
import statistics
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import a1_count as A  # noqa: E402
import i1_count as I  # noqa: E402

REC = re.compile(r"^\[inline\] record (\S+): hint=(\d) blk=\d+ ins=(\d+)")
FIN = re.compile(r"^\[inline\] final (\S+): ins=(\d+) ntmp=(\d+) used=(\d+) own=(-?\d+) sites=(-?\d+) static=(\d) "
                 r"addrtaken=(\d) sec=(\d) hint=(\d) absorbed=(\d)")
DEC = re.compile(r"^\[inline\] decline (\S+) -> (\S+): ([a-z ]+) \(pass=(\d) sites=(\d+) hint=(\d) own=(\d+) "
                 r"caller_sec=(\d) caller_ntmp=(\d+) caller_used=(\d+) callee_ntmp=(\d+) callee_used=(\d+) "
                 r"callee_ins=(\d+) room=(\d+) rest=([a-z -]+)\)")
ABS = re.compile(r"^\[inline\] (\S+) absorbed (\S+) \(")
TMP0 = 64           # QBE's first non-register temporary: counted in every fn->ntmp
SIZES = (40, 50, 60, 64, 70, 72, 80, 90, 100)


# ------------------------------------------------------------------------------------------------- compile
def compile_dir(a):
    os.makedirs(a.out, exist_ok=True)
    env = dict(os.environ, CC_TRACE_INLINE="1")
    n = bad = 0
    for p in sorted(glob.glob(os.path.join(a.ir_dir, "*.ssa"))):
        base = os.path.basename(p)[:-4]
        unit = base.split("__")[-1] if a.short else base
        out = os.path.join(a.out, unit)
        with open(out + ".trace", "w") as tr:
            r = subprocess.run([a.qbe, "-t", "w65816", "-o", out + ".c.asm", p], stderr=tr, env=env)
        n += 1
        if r.returncode:
            bad += 1
            print(f"FAIL {p}", file=sys.stderr)
    print(f"compiled {n} units, {bad} failed, into {a.out}")


# ------------------------------------------------------------------------------------------------- reading
def read_unit(asm, trace):
    fns, calls, addr = {}, [], collections.defaultdict(set)
    if os.path.exists(asm):
        A.read_casm(asm, asm, fns, calls, addr)
    info = {}
    decl, absd = [], []
    for line in open(trace, errors="replace"):
        m = REC.match(line)
        if m:
            info.setdefault(m.group(1), {})["own"] = int(m.group(3))
            continue
        m = FIN.match(line)
        if m:
            d = info.setdefault(m.group(1), {})
            d.update(final=int(m.group(2)), ntmp=int(m.group(3)), used=int(m.group(4)), sites=int(m.group(6)),
                     static=m.group(7) == "1", addrtaken=m.group(8) == "1", sec=m.group(9) == "1",
                     hint=m.group(10) == "1", absorbed=m.group(11) == "1")
            continue
        m = DEC.match(line)
        if m:
            g = m.groups()
            decl.append(dict(caller=g[0], callee=g[1], why=g[2], pass_=int(g[3]), sites=int(g[4]), hint=g[5] == "1",
                             own=int(g[6]), caller_sec=g[7] == "1", caller_ntmp=int(g[8]), caller_used=int(g[9]),
                             callee_ntmp=int(g[10]), callee_used=int(g[11]), callee_ins=int(g[12]),
                             room=int(g[13]), rest=g[14]))
            continue
        m = ABS.match(line)
        if m:
            absd.append((m.group(1), m.group(2)))
    absorbers = {c for c, _ in absd}
    out = {}
    for name, d in info.items():
        if "final" not in d:
            continue
        f = fns.get(name)
        d["emitted"] = f is not None
        if f is not None:
            d["bytes"] = I.body_bytes(f)
            tb = sum(2 * len(x[1].split(None, 1)[1].split(",")) for x in f["body"]
                     if isinstance(x, tuple) and x[0] == "D" and x[1].lower().startswith(".dw"))
            d["table_bytes"] = tb
            d["bytes"] += tb
            # what a copy carries: the body without its head and tail pieces and its rtl (i1_count's reading),
            # plus a 3-byte jump per extra return
            head, tail, rtls, _ = I.head_tail(f["body"])
            d["copy"] = d["bytes"] - sum(I.ins_len(f["body"][q]) for q in head + tail) - len(rtls) \
                + max(0, len(rtls) - 1) * 3
        d["absorbed_any"] = name in absorbers
        d["flat"] = d.get("own") == d["final"] and name not in absorbers
        out[name] = d
    edges = collections.defaultdict(set)
    for _, c, t, _ in calls:
        edges[c].add(t)
    for x in decl:
        edges[x["caller"]].add(x["callee"])
    rec = I.recursive_set(edges)
    for name in out:
        out[name]["recursive"] = name in rec
    return out, decl, absd


def read_dirs(dirs):
    units = {}
    for d in dirs:
        for tr in sorted(glob.glob(os.path.join(d, "*.trace"))):
            u = tr[:-len(".trace")]
            units[u] = read_unit(u + ".c.asm", tr)
    return units


# ------------------------------------------------------------------------------------------------- report
def candidates(fns, decl):
    """The size rule's candidates of one unit (plan 1.1, conditions 1-3 and 5): name -> list of its sites."""
    by = collections.defaultdict(list)
    for x in decl:
        by[x["callee"]].append(x)
    out = {}
    for name, d in fns.items():
        if not d.get("static") or d.get("addrtaken") or d.get("sec") or d.get("hint") or d.get("recursive"):
            continue
        if d.get("sites", 0) < 2:
            continue
        sites = by.get(name, [])
        if not sites or any(s["why"] != "sites" for s in sites):
            continue
        if any(s["rest"] not in ("ok", "tmp room") for s in sites):
            continue
        out[name] = sites
    return out


def sep_table(pop, key_ir, key_b, lim=256):
    rows = []
    for n in SIZES:
        tk = sum(1 for d in pop if d[key_ir] <= n and d[key_b] <= lim)
        wr = sum(1 for d in pop if d[key_ir] <= n and d[key_b] > lim)
        ms = sum(1 for d in pop if d[key_ir] > n and d[key_b] <= lim)
        rows.append(dict(N=n, taken=tk, wrong=wr, missed=ms))
    return rows


def quart(v):
    v = sorted(v)
    if not v:
        return []
    return [round(v[int(q * (len(v) - 1))], 3) for q in (0, .25, .5, .75, 1)]


def plan_estimate(fns, cand, nsize, bpi, callseq):
    """Plan 1.3 without budgets: admit every candidate with own <= nsize; nested final, cost, growth."""
    adm = {g for g in cand if fns[g]["own"] <= nsize}
    # sites of h in g's final IR: decline lines with g as caller (a clone's calls are walked in its absorber)
    sites_in = collections.defaultdict(collections.Counter)
    for g, ss in cand.items():
        for s in ss:
            sites_in[s["caller"]][g] += 1
    memo = {}

    def final(g, depth=0):
        if g in memo:
            return memo[g]
        memo[g] = bpi * fns[g]["final"]
        if depth > 50:
            return memo[g]
        v = bpi * fns[g]["final"]
        for h, k in sites_in.get(g, {}).items():
            if h in adm and h != g:
                v += k * (final(h, depth + 1) - callseq)
        memo[g] = v
        return v
    cost = {}
    for g in adm:
        nsit = sum(k for c, cnt in sites_in.items() for h, k in cnt.items() if h == g
                   and fns.get(c, {}).get("emitted"))
        cost[g] = (nsit - 1) * final(g) - nsit * callseq if nsit else 0
    growth_caller = collections.Counter()
    for c, cnt in sites_in.items():
        if not fns.get(c, {}).get("emitted") or c in adm:
            continue
        for h, k in cnt.items():
            if h in adm:
                growth_caller[c] += k * (final(h) - callseq)
    unit = sum(growth_caller.values()) - sum(bpi * fns[g]["final"] for g in adm if fns[g].get("emitted"))
    big = max(((c, v, v + fns[c].get("bytes", 0)) for c, v in growth_caller.items()), key=lambda t: t[1],
              default=("", 0, 0))
    bigfinal = max(((c, v + fns[c].get("bytes", 0)) for c, v in growth_caller.items()), key=lambda t: t[1],
                   default=("", 0))
    return dict(admitted=len(adm), unit_growth=round(unit), sum_cost=round(sum(cost.values())),
                caller_max_growth=(big[0], round(big[1])), caller_max_final=(bigfinal[0], round(bigfinal[1])),
                callers_over_8k=sorted(c for c, v in growth_caller.items() if v > 8192),
                callers_final_over_16k=sorted(c for c, v in growth_caller.items()
                                              if v + fns[c].get("bytes", 0) > 16384))


def report(a):
    units = read_dirs(a.dir)
    allf = []
    reasons = collections.Counter()
    one_site = collections.Counter()
    room = []
    per_caller = {}
    cand_all = {}
    plans = {}
    for u, (fns, decl, absd) in units.items():
        for n, d in fns.items():
            allf.append(dict(d, name=n, unit=u))
        for x in decl:
            callee = fns.get(x["callee"], {})
            elig = callee.get("static") and not callee.get("addrtaken")
            key = "one" if x["sites"] == 1 else "multi"
            reasons[(key, x["why"], x["rest"])] += 1
            if x["sites"] == 1 and elig and x["why"] not in ("exported", "addrtaken", "self", "busy"):
                one_site[x["why"]] += 1
            if x["why"] == "tmp room":
                room.append(x)
                pc = per_caller.setdefault(u + ":" + x["caller"], dict(n=0, ntmp=0, used=0))
                pc["n"] += 1
                pc["ntmp"] = max(pc["ntmp"], x["caller_ntmp"])
                pc["used"] = max(pc["used"], x["caller_used"])
        cand = candidates(fns, decl)
        for g, ss in cand.items():
            cand_all[(u, g)] = dict(fns[g], name=g, unit=u, nsites=len(ss),
                                    room_sites=sum(1 for s in ss if s["rest"] == "tmp room"))
        if cand:
            plans[u] = plan_estimate(fns, cand, a.nsize, a.bpi, a.callseq)

    res = dict(units=len(units), functions=len(allf))
    res["decline_reasons"] = {" / ".join(k): v for k, v in sorted(reasons.items(), key=lambda kv: -kv[1])}
    res["one_site_declines"] = dict(one_site)
    res["room"] = dict(
        declines=len(room),
        caller_used_over_counted=quart([x["caller_used"] / max(1, x["caller_ntmp"] - TMP0) for x in room]),
        callee_used_over_counted=quart([x["callee_used"] / max(1, x["callee_ntmp"] - TMP0) for x in room]),
        fit_if_both_compacted=sum(1 for x in room if x["caller_used"] + x["callee_used"] + TMP0 < x["room"]),
        fit_if_tmp0_counted_once=sum(1 for x in room if x["caller_ntmp"] + x["callee_ntmp"] - TMP0 < x["room"]),
        callers={k: v for k, v in sorted(per_caller.items(), key=lambda kv: -kv[1]["n"])})
    flat = [d for d in allf if d.get("flat") and d.get("emitted") and d.get("own", 0) > 0]
    bpi = [d["bytes"] / d["own"] for d in flat]
    res["flat"] = dict(n=len(flat), bpi_quartiles=quart(bpi), separator=sep_table(flat, "own", "bytes"))
    tab = [d for d in flat if d.get("table_bytes")]
    med = statistics.median(bpi) if bpi else 0
    res["jump_table"] = dict(n=len(tab), bpi_quartiles=quart([d["bytes"] / d["own"] for d in tab]),
                             bpi_without_table=quart([(d["bytes"] - d["table_bytes"]) / d["own"] for d in tab]),
                             median_all=round(med, 3))
    free = [d for d in flat if d["own"] <= 6]
    res["free_copies"] = dict(n=len(free), bytes=quart([d["bytes"] for d in free]),
                              copy=quart([d["copy"] for d in free]),
                              copy_at_most_callseq=sum(1 for d in free if d["copy"] <= a.callseq + 2),
                              callseq=a.callseq + 2,
                              by_own={k: quart([d["copy"] for d in free if d["own"] == k]) for k in range(7)})
    C = [d for d in cand_all.values() if d.get("emitted")]
    res["candidates"] = dict(
        n=len(cand_all), emitted=len(C), absorbed_any=sum(1 for d in C if d["absorbed_any"]),
        with_room_sites=sum(1 for d in C if d["room_sites"]),
        own_vs_final_bytes=sep_table(C, "own", "bytes"), final_vs_final_bytes=sep_table(C, "final", "bytes"),
        own_le_N_final_gt_N=sum(1 for d in C if d["own"] <= a.nsize < d["final"]),
        own_le_N_bytes_gt_256=sorted(d["name"] for d in C if d["own"] <= a.nsize and d["bytes"] > 256),
        bpi_quartiles=quart([d["bytes"] / d["final"] for d in C if d["final"]]))
    res["plan"] = plans
    res["plan_max_unit"] = max(((u, p["unit_growth"]) for u, p in plans.items()), key=lambda t: t[1],
                               default=("", 0))
    if a.write_set:
        lim, path = int(a.write_set[0]), a.write_set[1]
        names = sorted({d["name"] for d in cand_all.values() if d["own"] <= lim})
        with open(path, "w") as fh:
            fh.write("\n".join(names) + "\n")
        res["written_set"] = dict(path=path, n=len(names), limit=lim)
    if a.write_final_set:
        lim, path = int(a.write_final_set[0]), a.write_final_set[1]
        names = sorted({d["name"] for d in cand_all.values() if d["final"] <= lim})
        with open(path, "w") as fh:
            fh.write("\n".join(names) + "\n")
        res["written_final_set"] = dict(path=path, n=len(names), limit=lim)
    return res


def show(res):
    print(f"{res['units']} units, {res['functions']} functions")
    print("decline reasons (one|multi site / reason / reason without the site count):")
    for k, v in res["decline_reasons"].items():
        print(f"  {v:6d}  {k}")
    print(f"one-site declines of static, address-not-taken callees: {res['one_site_declines']}")
    r = res["room"]
    print(f"tmp room: {r['declines']} declines; caller used/(ntmp-{TMP0}) {r['caller_used_over_counted']}; callee "
          f"{r['callee_used_over_counted']}; would fit with both compacted {r['fit_if_both_compacted']}, with Tmp0 "
          f"counted once {r['fit_if_tmp0_counted_once']}")
    for k, v in list(r["callers"].items())[:8]:
        print(f"    {k}: {v}")
    f = res["flat"]
    print(f"functions that absorbed nothing: {f['n']}; bytes per IR instruction {f['bpi_quartiles']}")
    for row in f["separator"]:
        print(f"    own <= {row['N']:3d}: <=256 B taken {row['taken']:4d}  >256 B taken {row['wrong']:3d}  "
              f"<=256 B missed {row['missed']:3d}")
    print(f"jump tables: {res['jump_table']}")
    print(f"free copies (own <= 6): {res['free_copies']}")
    c = res["candidates"]
    print(f"size-rule candidates: {c['n']} ({c['emitted']} emitted, {c['absorbed_any']} absorbed something, "
          f"{c['with_room_sites']} with a site the room would decline); final bytes per IR {c['bpi_quartiles']}")
    for o, fi in zip(c["own_vs_final_bytes"], c["final_vs_final_bytes"]):
        print(f"    N {o['N']:3d}: own taken {o['taken']:4d} wrong {o['wrong']:3d} missed {o['missed']:3d} | "
              f"final taken {fi['taken']:4d} wrong {fi['wrong']:3d} missed {fi['missed']:3d}")
    print(f"    own <= N_SIZE < final: {c['own_le_N_final_gt_N']}; own <= N_SIZE and > 256 B: "
          f"{len(c['own_le_N_bytes_gt_256'])}")
    print(f"plan estimate per unit (largest {res['plan_max_unit']}):")
    for u, p in sorted(res["plan"].items(), key=lambda kv: -kv[1]["unit_growth"])[:10]:
        print(f"    {os.path.basename(u)[-60:]}: {p}")
    for k in ("written_set", "written_final_set"):
        if k in res:
            print(f"wrote {res[k]}")


# ------------------------------------------------------------------------------------------------- growth
SEC = re.compile(r'^\.SECTION "\.text\.([^"]+)"')


def text_sections(sym):
    out = {}
    on = False
    for line in open(sym, errors="replace"):
        if line.startswith("[sections]"):
            on = True
            continue
        if line.startswith("["):
            on = False
        if on:
            p = line.split()
            if len(p) >= 5 and p[4].startswith(".text."):
                out[p[4][6:]] = out.get(p[4][6:], 0) + int(p[3], 16)
    return out


def unit_map(d):
    m = {}
    for p in glob.glob(os.path.join(d, "*.c.asm")) + glob.glob(os.path.join(d, "*.c.wrap.asm")):
        u = os.path.basename(p).split(".c.")[0]
        for line in open(p, errors="replace"):
            s = SEC.match(line)
            if s:
                m.setdefault(s.group(1), u)
    return m


def growth(a):
    b, af = text_sections(a.before_sym), text_sections(a.after_sym)
    um = unit_map(a.before_dir)
    um.update({k: v for k, v in unit_map(a.after_dir).items() if k not in um})
    pu = collections.defaultdict(lambda: [0, 0])
    for k, v in b.items():
        pu[um.get(k, "?")][0] += v
    for k, v in af.items():
        pu[um.get(k, "?")][1] += v
    per_fn = sorted(((k, af.get(k, 0) - b.get(k, 0), af.get(k, 0)) for k in set(b) | set(af)),
                    key=lambda t: -t[1])
    res = dict(units={u: dict(before=x[0], after=x[1], growth=x[1] - x[0]) for u, x in
                      sorted(pu.items(), key=lambda kv: -(kv[1][1] - kv[1][0]))},
               total=dict(before=sum(b.values()), after=sum(af.values()),
                          growth=sum(af.values()) - sum(b.values())),
               top_fn_growth=[dict(fn=k, growth=g, after=s) for k, g, s in per_fn[:15]],
               largest_after=sorted(({"fn": k, "bytes": v} for k, v in af.items()), key=lambda t: -t["bytes"])[:5],
               fn_growth_over_8k=sum(1 for _, g, _ in per_fn if g > 8192),
               sections_over_16k=sum(1 for v in af.values() if v > 16384))
    print(f"code (.text.*): {res['total']}")
    for u, x in res["units"].items():
        print(f"    {u:24s} {x['before']:8d} -> {x['after']:8d}  {x['growth']:+8d}")
    print(f"  functions grown over 8 KB: {res['fn_growth_over_8k']}; sections over 16 KB after: "
          f"{res['sections_over_16k']}; largest after {res['largest_after'][:3]}")
    for t in res["top_fn_growth"][:8]:
        print(f"    {t}")
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    c = sub.add_parser("compile")
    c.add_argument("--qbe", required=True)
    c.add_argument("--ir-dir", required=True)
    c.add_argument("--out", required=True)
    c.add_argument("--short", action="store_true")
    r = sub.add_parser("report")
    r.add_argument("--dir", action="append", required=True)
    r.add_argument("--bpi", type=float, default=3.24, help="estimated bytes per IR instruction (plan: 3.24)")
    r.add_argument("--callseq", type=int, default=10, help="bytes of a call sequence a copy removes")
    r.add_argument("--nsize", type=int, default=72)
    r.add_argument("--write-set", nargs=2, metavar=("N", "PATH"))
    r.add_argument("--write-final-set", nargs=2, metavar=("N", "PATH"),
                   help="the same with the final IR size (after the candidate's own inlining): hypothesis H5")
    r.add_argument("--json")
    g = sub.add_parser("growth")
    g.add_argument("--before-sym", required=True)
    g.add_argument("--before-dir", required=True)
    g.add_argument("--after-sym", required=True)
    g.add_argument("--after-dir", required=True)
    g.add_argument("--json")
    a = ap.parse_args()
    if a.cmd == "compile":
        compile_dir(a)
        return
    res = report(a) if a.cmd == "report" else growth(a)
    if a.cmd == "report":
        show(res)
    if a.json:
        with open(a.json, "w") as fh:
            json.dump(res, fh, indent=1, default=str)


if __name__ == "__main__":
    main()
