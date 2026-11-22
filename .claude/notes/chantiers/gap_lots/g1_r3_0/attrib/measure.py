#!/usr/bin/env python3
"""python3 measure.py v.asm [...] : swap the .gap_pathpp section of kernels.asm for v.asm in a private copy of the bench, run.py, print path / pathpp rows."""
import json, re, shutil, subprocess, sys
from pathlib import Path
B = Path(__file__).resolve().parent.parent
base = (B / "kernels.asm.pp0").read_text()
i = base.index('.SECTION ".gap_pathpp"')
head = base[:i]
for v in sys.argv[1:]:
    v = Path(v); name = v.stem
    d = B / "attrib" / "work" / name
    shutil.rmtree(d, ignore_errors=True); d.mkdir(parents=True)
    for f in ("run.py", "Makefile", "kernels.c"):
        shutil.copy(B / f, d)
    (d / "kernels.asm").write_text(head + v.read_text())
    r = subprocess.run([sys.executable, "run.py", "--json", "out.json"], cwd=d, capture_output=True, text=True)
    if r.returncode:
        print(name, "FAILED:", (r.stdout + r.stderr)[-300:].replace("\n", " | ")); continue
    j = json.loads((d / "out.json").read_text())
    s, f = j["slow"]["pathpp"], j["fast"]["pathpp"]
    print(f"{name}\tslow {s['asm']}\tfast {f['asm']}\tbytes {s['asm_bytes']}\tsum {s['asm_sum']:#x}", flush=True)
