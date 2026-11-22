#!/usr/bin/env python3
"""L4 S1: opensnes-rom place and check --objects on the built corpus, and the compiler's provoked fault.

1. Every built example: `opensnes-rom place` writes an empty window (no frame is claimed in S1) and
   `opensnes-rom check --objects linkfile` passes (each C table equals its text, no sf.* label, nothing refused).
2. The compiler's fault: one example copied and rebuilt with QBE_SFRAME_FAULT=noedge (the table leaves out the
   first call of every function) links to the same ROM, and the check refuses it, naming a call.
3. With --oracle: `place --what-if` (every compiler candidate of the project given its S0 frame) against the S0
   count (`sframe_count.py`, models closure_rev and, with OPENSNES_SFRAME_FAULT=noreview, closure): the same
   candidates, the same refused functions, the same deepest chain, on every example. Two implementations, one
   answer: the script is the tool's oracle (L4 plan, S1).

usage: python3 devtools/sframe/test_sframe_link.py [--oracle]     (after `make`; `make test-sframe`)
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ROM = ROOT / "bin" / "opensnes-rom"
TMP = Path(os.environ.get("OPENSNES_TMP", "/tmp/opensnes-tests"))
FAULT_EXAMPLE = "text/print_string"


def examples():
    for lf in sorted((ROOT / "examples").rglob("linkfile")):
        sfc = sorted(lf.parent.glob("*.sfc"))
        if sfc:
            yield lf.parent, sfc[0]


def corpus():
    errs, n = [], 0
    with tempfile.TemporaryDirectory(dir=TMP) as td:
        for d, sfc in examples():
            n += 1
            label = d.relative_to(ROOT / "examples")
            out = Path(td) / "sframe.asm"
            p = subprocess.run([str(ROM), "place", "--out", str(out), "linkfile"], cwd=d, capture_output=True, text=True)
            if p.returncode != 0:
                errs.append(f"{label}: place exit {p.returncode}: {(p.stdout + p.stderr).strip()[:300]}")
            elif ".RAMSECTION" in out.read_text() or "0 static frames" not in p.stdout:
                errs.append(f"{label}: place wrote a frame: {p.stdout.strip()}")
            c = subprocess.run([str(ROM), "check", "--objects", "linkfile", "--no-assets", "--no-bank-reads",
                                "--no-nmi-race", "--keep-ram-markers", sfc.name], cwd=d, capture_output=True, text=True)
            if c.returncode != 0 or "OK: static frames: 0 framed" not in c.stdout:
                errs.append(f"{label}: check exit {c.returncode}: {(c.stdout + c.stderr).strip()[-600:]}")
    return n, errs


def noedge():
    errs = []
    src = ROOT / "examples" / FAULT_EXAMPLE
    with tempfile.TemporaryDirectory(dir=TMP) as td:
        work = Path(td) / "fx"
        shutil.copytree(src, work, ignore=shutil.ignore_patterns("*.o", "*.sfc", "*.sym", "*.c.asm", "*.wrap.asm",
                                                                   "*.text.asm", "linkfile", ".opensnes_config"))
        env = {**os.environ, "QBE_SFRAME_FAULT": "noedge"}
        b = subprocess.run(["make", f"OPENSNES={ROOT}"], cwd=work, capture_output=True, text=True, env=env)
        if b.returncode != 0:
            return [f"the faulted build failed: {(b.stdout + b.stderr).strip()[-400:]}"]
        rom = sorted(work.glob("*.sfc"))[0]
        if rom.read_bytes() != (src / rom.name).read_bytes():
            errs.append("the faulted build's ROM differs from the corpus one (the fault must touch comments only)")
        c = subprocess.run([str(ROM), "check", "--objects", "linkfile", "--no-assets", rom.name], cwd=work,
                           capture_output=True, text=True)
        if c.returncode == 0 or "table/text:" not in c.stderr or "its table does not say so" not in c.stderr:
            errs.append(f"QBE_SFRAME_FAULT=noedge was not refused: exit {c.returncode}: {(c.stdout + c.stderr).strip()[-400:]}")
    return errs


def oracle():
    errs = []
    with tempfile.TemporaryDirectory(dir=TMP) as td:
        js = Path(td) / "s0.json"
        r = subprocess.run([sys.executable, str(ROOT / "devtools" / "sframe" / "sframe_count.py"), "--corpus",
                            str(ROOT / "examples"), "--lib-casm", str(ROOT / "lib" / "build" / "lorom"), "--json",
                            str(js)], capture_output=True, text=True)
        if r.returncode != 0:
            return 0, [f"sframe_count.py failed: {r.stderr.strip()[-300:]}"]
        s0 = {o["label"]: o for o in json.load(open(js))}
    n = 0
    for model, fault in (("closure_rev", ""), ("closure", "noreview")):
        for label, o in sorted(s0.items()):
            d = ROOT / "examples" / label
            p = subprocess.run([str(ROM), "place", "--what-if", "--json", "linkfile"], cwd=d, capture_output=True,
                               text=True, env={**os.environ, "OPENSNES_SFRAME_FAULT": fault})
            if p.returncode != 0:
                errs.append(f"{label}: place --what-if exit {p.returncode}")
                continue
            t = json.loads(p.stdout)
            m = o["models"][model]
            want = (o["candidates"], sorted(m["refused_names"]), m["place"]["params"]["deepest"])
            got = (t["candidates"], sorted(t["refused"]), t["deepest"])
            n += 1
            if want != got:
                errs.append(f"{label} ({model}): script {want}, tool {got}")
    return n, errs


def main():
    TMP.mkdir(parents=True, exist_ok=True)
    if not ROM.is_file():
        sys.exit(f"{ROM} missing: run make first")
    bad = 0
    n, errs = corpus()
    for e in errs:
        print(f"  FAIL {e}")
    print(f"sframe corpus: {n - len({e.split(':')[0] for e in errs})}/{n} examples: empty window, check passes")
    bad += bool(errs)
    errs = noedge()
    for e in errs:
        print(f"  FAIL {e}")
    print(f"sframe fault QBE_SFRAME_FAULT=noedge on {FAULT_EXAMPLE}: " + ("refused by the check, same ROM" if not errs else "FAIL"))
    bad += bool(errs)
    if "--oracle" in sys.argv:
        n, errs = oracle()
        for e in errs:
            print(f"  FAIL {e}")
        print(f"sframe oracle: {n - len(errs)}/{n} (example, model) pairs agree with sframe_count.py")
        bad += bool(errs)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
