#!/usr/bin/env python3
"""Golden tests for opensnes (the project CLI; a shell script until 2026-10-06).

The contract of a port: the same bytes. golden/blank and golden/game hold
what the shell script's `init` wrote (main.c, the two luna manifests, the
Makefile with its SDK path replaced by @SDK@), and golden/upgrade.txt what
its `upgrade` printed on a source using all 46 removed names and the five
calls that changed meaning — itself identical to the Python checker it had
replaced the day before. `init` runs against a fake SDK (a folder holding
make/common.mk) named by OPENSNES_HOME; `upgrade` and `doctor` find the real
one from the binary's place, bin/. `build`, `test` and `run` drive make and
luna on a real project: `make test-project` and `make release-smoke` are
their tests (the latter on the three OSes, through the zip).

Run:  python3 tools/opensnes/tests/run_golden.py
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from golden import Golden  # noqa: E402

g = Golden("opensnes", __file__)


def cli(argv, cwd, home=None):
    env = {k: v for k, v in os.environ.items() if k != "OPENSNES_HOME"}
    if home is not None:
        env["OPENSNES_HOME"] = str(home)
    return subprocess.run([str(g.tool), *argv], cwd=cwd, env=env, capture_output=True, text=True, timeout=60)


def fake_sdk(work: Path) -> Path:
    sdk = work / "sdk"
    (sdk / "make").mkdir(parents=True)
    (sdk / "make" / "common.mk").write_text("# a fake SDK for the init tests\n")
    return sdk


def init_case(template, name, files):
    def run():
        with tempfile.TemporaryDirectory() as td:
            work = Path(td).resolve()
            sdk = fake_sdk(work)
            argv = ["init", name] + (["--template", template] if template != "blank" else [])
            proc = cli(argv, work, sdk)
            if proc.returncode != 0:
                return [f"exit {proc.returncode}: {proc.stderr.strip()[:160]}"]
            errs = []
            leaf = Path(name).name
            for rel in files:
                got = work / name / rel
                want = (g.golden / template / Path(rel).name).read_text()
                want = want.replace("@SDK@", sdk.as_posix()).replace("my_game-2", leaf).replace("blank-one", leaf)
                if rel == "Makefile":   # the title: 21 characters, upper case, - and _ as spaces
                    title = leaf.upper().replace("_", " ").replace("-", " ")[:21].ljust(21)
                    want = "\n".join(f"ROM_NAME := {title}" if l.startswith("ROM_NAME := ") else l for l in want.split("\n"))
                if not got.is_file():
                    errs.append(f"{rel}: not written")
                elif got.read_text() != want:
                    errs.append(f"{rel}: differs from what the shell CLI wrote")
            if not (work / name / "res").is_dir():
                errs.append("res/: not created")
            if f"Created project: {name}/" not in proc.stdout or "opensnes build" not in proc.stdout:
                errs.append("the next steps are not printed")
            return errs
    return run


g.check("init blank-one == the shell CLI's files", "main.c, Makefile, res/", init_case("blank", "blank-one", ["main.c", "Makefile"]))
g.check("init my_game-2 --template game == the shell CLI's files", "main.c, Makefile, two manifests",
        init_case("game", "my_game-2", ["main.c", "Makefile", "test/boot.toml", "test/walk_right.toml"]))
g.check("init sub/dir/deep_game --template game (a path: the leaf names the ROM)", "nested folders made",
        init_case("game", "sub/dir/deep_game", ["main.c", "Makefile", "test/boot.toml", "test/walk_right.toml"]))


def refused(label, argv, needle, home="fake", pre=None, nothing=None):
    def run():
        with tempfile.TemporaryDirectory() as td:
            work = Path(td).resolve()
            sdk = fake_sdk(work) if home == "fake" else home
            if pre:
                pre(work)
            proc = cli(argv, work, sdk)
            errs = []
            if proc.returncode == 0:
                errs.append("accepted (exit 0)")
            if needle not in proc.stderr + proc.stdout:
                errs.append(f"without {needle!r}: {(proc.stderr + proc.stdout).strip()[-140:]}")
            if nothing and (work / nothing).exists():
                errs.append(f"wrote {nothing}")
            return errs
    g.check(label, "refused", run)


refused("init of an existing folder", ["init", "there"], "already exists", pre=lambda w: (w / "there").mkdir())
refused("init --template nope", ["init", "x", "--template", "nope"], "blank or game", nothing="x")
refused("init 'my game' (make cannot hold a space)", ["init", "my game"], "letters, digits", nothing="my game")
refused("build outside a project", ["build"], "no OpenSNES project here")
refused("test outside a project", ["test"], "no OpenSNES project here")



def upgrade_case(argv, golden, want_exit):
    def run():
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            g.stage(work, "*")
            proc = cli(argv, work)
            errs = []
            if proc.returncode != want_exit:
                errs.append(f"exit {proc.returncode}, want {want_exit}: {proc.stderr.strip()[:120]}")
            if proc.stdout != (g.golden / golden).read_text():
                errs.append(f"stdout differs from {golden}")
            return errs
    return run


g.check("upgrade old.c two.h data.asm clean.c == the shell CLI (55 hits, exit 1)", "same lines",
        upgrade_case(["upgrade", "old.c", "two.h", "data.asm", "clean.c"], "upgrade.txt", 1))
g.check("upgrade -q --removed-only old.c (what a failed compile prints: 46 names, no current API)", "same lines",
        upgrade_case(["upgrade", "-q", "--removed-only", "old.c"], "upgrade_removed_only.txt", 1))


def upgrade_folder():
    """A folder is walked for .c .h .asm .s .inc, in path order; a .txt is not read; a clean tree exits 0."""
    with tempfile.TemporaryDirectory() as td:
        work = Path(td)
        (work / "src" / "sub").mkdir(parents=True)
        for name, where in (("old.c", "src"), ("two.h", "src/sub"), ("data.asm", "src/sub"), ("clean.c", "src/sub")):
            (work / where / name).write_bytes((g.fixtures / name).read_bytes())
        (work / "src" / "sub" / "note.txt").write_text("padRaw(0)\n")
        proc = cli(["upgrade", "-q", "src"], work)
        files = [l.split(":")[0] for l in proc.stdout.splitlines()]
        order = sorted(set(files), key=files.index)
        errs = []
        if order != ["src/old.c", "src/sub/data.asm", "src/sub/two.h"]:
            errs.append(f"files reported: {order}")
        if len(files) != 55 or proc.returncode != 1:
            errs.append(f"{len(files)} hits, exit {proc.returncode}")
        clean = cli(["upgrade", "src/sub/clean.c"], work)
        if clean.returncode != 0 or "0 hit(s) in 1 file(s)" not in clean.stdout:
            errs.append(f"a clean file: exit {clean.returncode}")
        return errs


g.check("upgrade src/ (a folder: sources only, path order; a clean file exits 0)", "55 hits in 3 files", upgrade_folder)


def doctor_json():
    """doctor --json: one object, a list of checks each ok / warn / fail, and the SDK is this repository."""
    proc = cli(["doctor", "--json"], g.repo)
    try:
        d = json.loads(proc.stdout)
    except ValueError as e:
        return [f"not JSON: {e}"]
    checks = {c["check"]: c for c in d.get("checks", [])}
    errs = []
    if checks.get("SDK", {}).get("detail") != g.repo.as_posix():
        errs.append(f"SDK: {checks.get('SDK')}")
    for name in ("host cc", "make", "library"):
        if name not in checks:
            errs.append(f"no '{name}' check")
    if any(c["status"] not in ("ok", "warn", "fail") for c in checks.values()):
        errs.append("a status that is not ok / warn / fail")
    if (d.get("failures", 0) > 0) != (proc.returncode != 0):
        errs.append(f"{d.get('failures')} failures but exit {proc.returncode}")
    return errs


def doctor_empty_sdk():
    with tempfile.TemporaryDirectory() as td:
        work = Path(td).resolve()
        proc = cli(["doctor"], work, fake_sdk(work))
        out = proc.stdout
        errs = []
        if proc.returncode != 1:
            errs.append(f"exit {proc.returncode}, want 1")
        errs += [f"without {n!r}" for n in ("FAIL: cc65816", "FAIL: opensnes-rom", "FAIL: library", "WARN: opensnes-sprite") if n not in out]
        return errs


g.check("doctor --json in the repository", "valid, SDK found from bin/", doctor_json)
g.check("doctor on an SDK with nothing built", "exit 1, each missing binary named", doctor_empty_sdk)
sys.exit(g.report())
