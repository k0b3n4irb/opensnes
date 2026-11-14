#!/usr/bin/env python3
"""check_submodule_pins.py — every submodule commit a pushed commit names is on that submodule's remote.

A superproject pushed before the fork it points at turns every CI job red at
checkout ("upload-pack: not our ref"), and the commit cannot be un-pushed
(2026-11-12: the compiler lot's QBE commits were pushed one minute after
OpenSNES). The pre-push hook runs this on the commit about to leave.

    python3 devtools/check_submodule_pins.py [COMMIT]     # default HEAD
    python3 devtools/check_submodule_pins.py --coherent RANGE

--coherent: in every commit of RANGE, the submodule commit the tree names is the one
compiler/PINS.md writes in the same commit. A series whose code commit moves the
submodule and whose later commit updates PINS.md leaves a commit that
`make verify-toolchain` refuses: a bisect or a clone built at it fails (2026-11-14,
three commits of the X/Y lot).
"""
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent


def git(*args, cwd=REPO):
    return subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True)


def pins(commit):
    out = git("ls-tree", "-r", commit).stdout
    return [(line.split()[2], line.split("\t", 1)[1]) for line in out.splitlines()
            if line.startswith("160000 ")]


def on_remote(path, sha):
    sub = REPO / path
    if not (sub / ".git").exists():
        return None                      # not checked out here: nothing to ask
    tips = git("ls-remote", "origin", cwd=sub)
    if tips.returncode:
        return None                      # no network: the push itself will fail
    if any(line.split()[0] == sha for line in tips.stdout.splitlines()):
        return True
    git("fetch", "-q", "origin", cwd=sub)
    return bool(git("branch", "-r", "--contains", sha, cwd=sub).stdout.strip())


def coherent(rng):
    import re
    bad = []
    for c in git("rev-list", "--reverse", rng).stdout.split():
        table = git("show", f"{c}:compiler/PINS.md").stdout
        if not table:
            continue
        for sha, path in pins(c):
            m = re.search(r"^\|\s*" + re.escape(path) + r"\s*\|\s*([0-9a-f]{12,40})\s*\|", table, re.M)
            if m and not sha.startswith(m.group(1)[:12]):
                bad.append((c, path, sha, m.group(1)))
    for c, path, sha, pinned in bad:
        print(f"check_submodule_pins: commit {c[:12]} names {path} at {sha[:12]} but compiler/PINS.md "
              f"says {pinned[:12]}: move the pin and PINS.md in the same commit", file=sys.stderr)
    if not bad:
        print(f"check_submodule_pins: PINS.md agrees with the submodule pins in every commit of {rng}")
    return 1 if bad else 0


def main():
    if len(sys.argv) > 2 and sys.argv[1] == "--coherent":
        return coherent(sys.argv[2])
    commit = sys.argv[1] if len(sys.argv) > 1 else "HEAD"
    bad = [(path, sha) for sha, path in pins(commit) if on_remote(path, sha) is False]
    for path, sha in bad:
        print(f"check_submodule_pins: {path} is pinned at {sha[:12]}, which is on no branch of its "
              f"remote: push the fork first (git -C {path} push origin {sha[:12]}:refs/heads/<pinned branch>)",
              file=sys.stderr)
    if not bad:
        print(f"check_submodule_pins: {len(pins(commit))} submodule pin(s) of {commit[:12]} are on their remotes")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
