#!/usr/bin/env python3
"""release_smoke.py — build and test a user project from the release zip.

The release zip is what users download; CI used to test only the source
tree. From July to v0.44.0 every zip failed at the first `make` of a user
project (make/common.mk ran devtools/check_bank_reads.py, which the zip did
not ship), and nothing noticed. This script consumes the zip exactly as a
user does:

  1. extract it into a scratch directory (exec bits restored);
  2. check the shipped starter/ carries no build output (a stale game.sfc
     would let step 3 pass without linking anything);
  3. `make` the starter with no OPENSNES in the environment (its Makefile
     must find the SDK on its own);
  4. scaffold a project with the zip's `bin/opensnes init --template game`
     and build it;
  5. when a luna binary is available (LUNA_BIN, or the SDK tree's
     testing/bin/luna), run the project's `make test-update` then
     `make test` — the "test your game" story of GETTING_STARTED;
  6. `opensnes budget` and `opensnes release` on that project.

Usage: python3 devtools/release_smoke.py release/<name>.zip
Exit 0 on success, 1 on the first failing step (its output is printed).
"""

import os
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
SDK_TREE = HERE.parent
BUILD_OUTPUTS = (".sfc", ".o", ".sym", ".obj")


def extract(zip_path: Path, dest: Path) -> None:
    """Extract, restoring the Unix mode bits zipfile ignores."""
    with zipfile.ZipFile(zip_path) as zf:
        for info in zf.infolist():
            out = Path(zf.extract(info, dest))
            mode = (info.external_attr >> 16) & 0o777
            if mode and not info.is_dir():
                out.chmod(mode)


def run(step: str, cmd: list, cwd: Path, env: dict) -> None:
    print(f"release-smoke: {step}: {' '.join(cmd)}", flush=True)
    proc = subprocess.run(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True)
    if proc.returncode != 0:
        print(proc.stdout)
        sys.exit(f"release-smoke: FAIL at '{step}' (exit {proc.returncode})")


def find_luna() -> str | None:
    env = os.environ.get("LUNA_BIN")
    if env and Path(env).is_file():
        return env
    for name in ("luna", "luna.exe"):   # .exe on Windows
        tree = SDK_TREE / "testing" / "bin" / name
        if tree.is_file():
            return str(tree)
    return None


def main() -> int:
    if len(sys.argv) != 2:
        sys.exit("usage: release_smoke.py release/<name>.zip")
    zip_path = Path(sys.argv[1]).resolve()
    if not zip_path.is_file():
        sys.exit(f"release-smoke: no such zip: {zip_path}")

    work = Path(tempfile.mkdtemp(prefix="opensnes-release-smoke-"))
    try:
        extract(zip_path, work)
        sdk = work / "opensnes"
        starter = sdk / "starter"
        if not (sdk / "make" / "common.mk").is_file():
            sys.exit("release-smoke: FAIL: the zip has no opensnes/make/common.mk")

        stale = sorted(p.name for p in starter.rglob("*") if p.suffix in BUILD_OUTPUTS)
        if stale:
            sys.exit(f"release-smoke: FAIL: starter/ ships build output: {', '.join(stale[:8])}")

        env = {k: v for k, v in os.environ.items()
               if k not in ("OPENSNES", "OPENSNES_HOME", "MAKEFLAGS", "MAKELEVEL", "MFLAGS")}
        run("build the starter", ["make"], starter, env)
        if not (starter / "game.sfc").is_file():
            sys.exit("release-smoke: FAIL: the starter build produced no game.sfc")

        # `bin/opensnes` is a compiled program since 2026-10-06 (a bash
        # script before, which Windows' CreateProcess could not start).
        cli = sdk / "bin" / ("opensnes.exe" if os.name == "nt" else "opensnes")
        run("scaffold a project", [str(cli), "init", "smoke-game", "--template", "game"], work, env)
        project = work / "smoke-game"
        # Through the CLI, and with no OPENSNES_HOME: the binary finds the SDK
        # from its own place (<sdk>/bin), as it does for whoever unzips it.
        penv = dict(env)
        run("opensnes build", [str(cli), "build"], project, penv)
        if not (project / "smoke-game.sfc").is_file():
            sys.exit("release-smoke: FAIL: `opensnes build` produced no smoke-game.sfc")

        luna = find_luna()
        if luna:
            penv["LUNA_BIN"] = luna
            run("record the project test baseline: opensnes test --update", [str(cli), "test", "--update"], project, penv)
            run("run the project test: opensnes test", [str(cli), "test"], project, penv)
            tested = "built, tested in luna and released through the opensnes CLI"
            release = [str(cli), "release", "--tag", "smoke"]
        else:
            tested = "built and released through the opensnes CLI (no luna binary here: project test skipped)"
            release = [str(cli), "release", "--tag", "smoke", "--no-test"]
        # budget and release start opensnes-rom from the CLI: the one place a
        # quoted path goes through the platform's shell (cmd.exe on Windows).
        run("opensnes budget", [str(cli), "budget"], project, penv)
        run("opensnes release", release, project, penv)
        if not (project / "release" / "smoke-game-smoke.sfc").is_file():
            sys.exit("release-smoke: FAIL: `opensnes release` wrote no release/smoke-game-smoke.sfc")
        print(f"release-smoke: OK — {zip_path.name}: starter and a scaffolded project {tested}")
        return 0
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
