#!/usr/bin/env python3
"""The interrupt gate: a NMI forced before EVERY instruction of the runtime's 32-bit helpers must not change a result.

`luna race-sweep` (luna develop fe0720a, in the first tag after v1.43.0) snapshots the machine at probe_begin() of
testing/fixtures/stress/race_sweep, forces an NMI before each instruction of a range, runs on to probe_end() and
compares the stored results (r_mul r_div r_rem r_sdiv) with the run that was not interrupted. The NMI lag callback
does 32-bit operations of its own, so a helper that keeps state in a shared slot gives a different result: a race.
A trial is identical, DIFFERS, NOT TAKEN (the injection was refused: never a pass) or END NOT REACHED.

  python3 testing/race_gate.py            # exit 1 on any race, refused injection or lost run
  python3 testing/race_gate.py --list     # the swept ranges

Ranges are the helpers of lib/source/mul32.asm and div32.asm, read from the .sym (a label up to the next label of
the same bank). A helper that disappears from the .sym is an error, not a skip. A luna without `race-sweep` skips
with a loud message until the pin carries it.
"""
import os
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from lib import REPO_ROOT, find_luna  # noqa: E402

FIX = REPO_ROOT / "testing" / "fixtures" / "stress" / "race_sweep"
ROM = FIX / "race_sweep.sfc"
SYM = FIX / "race_sweep.sym"
# (name, first label, label whose address ends the range, exclusive: None = up to the next top-level label)
HELPERS = [("mul32_soft", "mul32_soft", None), ("tcc_mul32", "tcc_mul32", None),
           ("div32 entries", "tcc_udivmod32", "div32_ucore"), ("div32_ucore", "div32_ucore", None)]
INVARIANT = ["r_mul", "r_div", "r_rem", "r_sdiv"]
KINDS = ["nmi"]


def symbols():
    out = []
    for line in SYM.read_text(encoding="utf-8", errors="replace").splitlines():
        m = re.match(r"^([0-9a-fA-F]{2}):([0-9a-fA-F]{4}) (\S+)$", line.strip())
        if m:
            out.append((int(m.group(1), 16), int(m.group(2), 16), m.group(3)))
    return out


def ranges(syms):
    res = []
    for name, first, stop in HELPERS:
        hit = [s for s in syms if s[2] == first]
        if not hit:
            raise SystemExit(f"race_gate: {first} is not in {SYM.name}: the gate would sweep nothing")
        bank, off, _ = hit[0]
        if stop:
            end = [s for s in syms if s[2] == stop][0][1] - 1
        else:
            later = sorted(o for b, o, n in syms if b == bank and o > off and "@" not in n)
            end = (later[0] - 1) if later else off + 0x40
        res.append((name, f"{bank:02x}:{off:04x}", f"{bank:02x}:{end:04x}"))
    return res


def has_race_sweep(luna):
    return subprocess.run([luna, "race-sweep", "--help"], capture_output=True).returncode == 0


def main():
    if not ROM.is_file():
        print(f"race_gate: build {FIX} first (make -C {FIX})", file=sys.stderr)
        return 2
    rs = ranges(symbols())
    if "--list" in sys.argv:
        for r in rs:
            print(*r)
        return 0
    luna = find_luna()
    if not has_race_sweep(luna):
        print("race_gate: SKIPPED: this luna has no `race-sweep` (the first tag after v1.43.0 has it); the gate is not armed")
        return 0
    bad = 0
    for name, a, b in rs:
        for kind in KINDS:
            cmd = [luna, "race-sweep", str(ROM), "--sym", str(SYM), "--from", a, "--to", b, "--kind", kind,
                   "--warm-until-pc", "probe_begin", "--until-pc", "probe_end", "--fail-on-race",
                   "--jobs", os.environ.get("LUNA_JOBS", "2")]
            for v in INVARIANT:
                cmd += ["--compare", v]
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=1800)
            last = [l for l in r.stdout.splitlines() if "addresses swept" in l]
            print(f"  {name:14s} {a}..{b} {kind}: {last[-1] if last else r.stdout.strip()[-120:]}")
            if r.returncode != 0:
                bad += 1
                print(r.stdout[-1500:])
    print(f"\nInterrupt gate: {len(rs) - bad}/{len(rs)} helpers clean" if not bad else f"\nInterrupt gate: {bad} helper(s) RACY")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
