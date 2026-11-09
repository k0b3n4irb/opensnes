#!/usr/bin/env python3
"""Compare the ROM hash lists printed by rom_hashes.py on several platforms.

  python3 devtools/compare_rom_hashes.py linux=a.log darwin=b.log windows=c.log
  python3 devtools/compare_rom_hashes.py --baseline testing/baselines/baselines.json linux=a.log

Prints, per platform pair, the ROMs whose bytes differ (and those present on one side
only). Exit 0 when every list is identical, 1 when any differs, 2 on usage.
"""
import json
import sys
from itertools import combinations


def load(path):
    d = {}
    for l in open(path, encoding="utf-8"):
        p = l.split()
        if len(p) == 2:
            d[p[0]] = p[1]
    return d


def diff(a, b):
    both = set(a) & set(b)
    return (sorted(k for k in both if a[k] != b[k]), sorted(set(a) - set(b)), sorted(set(b) - set(a)))


def main(argv):
    args = argv[1:]
    baseline = None
    if args and args[0] == "--baseline":
        baseline, args = args[1], args[2:]
    lists = {}
    for a in args:
        if "=" not in a:
            print(__doc__, file=sys.stderr)
            return 2
        name, path = a.split("=", 1)
        lists[name] = load(path)
    if len(lists) < 1 or (len(lists) < 2 and not baseline):
        print(__doc__, file=sys.stderr)
        return 2
    bad = 0
    for (n1, a), (n2, b) in combinations(lists.items(), 2):
        changed, only1, only2 = diff(a, b)
        print(f"{n1} vs {n2}: {len(set(a) & set(b))} ROMs in common, {len(changed)} differ, {len(only1)} only in {n1}, {len(only2)} only in {n2}")
        for k in changed[:20]:
            print(f"   DIFF {k}")
        bad += len(changed) + len(only1) + len(only2)
    if baseline:
        j = json.load(open(baseline))
        ex = j.get("examples", j)
        ref = {}
        for k, v in ex.items():
            h = v.get("rom_sha256") if isinstance(v, dict) else None
            if h:
                ref[k] = h
        for n, a in lists.items():
            # baselines.json keys are example names (audio_apu_switch); match by the ROM's folder path
            def key(path):
                return "_".join(path.split("/")[1:-1])
            mine = {key(p): h for p, h in a.items()}
            comp = [k for k in ref if k in mine]
            changed = [k for k in comp if ref[k] != mine[k]]
            print(f"{n} vs baselines.json: {len(comp)} compared, {len(changed)} differ")
            for k in changed[:10]:
                print(f"   DIFF {k}")
            bad += len(changed)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
