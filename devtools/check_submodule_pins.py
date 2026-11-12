#!/usr/bin/env python3
"""check_submodule_pins.py — every submodule commit a pushed commit names is on that submodule's remote.

A superproject pushed before the fork it points at turns every CI job red at
checkout ("upload-pack: not our ref"), and the commit cannot be un-pushed
(2026-11-12: the compiler lot's QBE commits were pushed one minute after
OpenSNES). The pre-push hook runs this on the commit about to leave.

    python3 devtools/check_submodule_pins.py [COMMIT]     # default HEAD
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


def main():
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
