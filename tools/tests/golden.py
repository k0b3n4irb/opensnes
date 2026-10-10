"""tools/tests/golden.py — the golden-output runner the asset tools' tests share.

Every tool under tools/ has a `tests/run_golden.py` that runs the built
binary on committed fixtures and byte-compares what it writes against
`tests/golden/`. Until 2026-10-05 each of the eight scripts carried its own
copy of that cycle (70 to 217 lines, 96 to 249 differing lines between any
two); this module is the one copy, and a tool's script is its table of
cases:

    g = Golden("gfx4snes", __file__)
    g.expect_outputs("bg.png [-s 8 -m]", ["-s", "8", "-m", "-i", "bg.png"],
                     copy=["bg.png"], outputs=["bg.pic", "bg.pal", "bg.map"])
    g.expect_refused("too many tiles", ["-m", "-i", "toomany.png"],
                     copy=["toomany.png"], needles=["1024 at most"])
    sys.exit(g.report())

Each case runs in a fresh temporary directory holding copies of the named
fixtures (`copy="*"` copies them all) and any `write={name: bytes}`;
outputs are compared with `filecmp` (byte for byte) against
`tests/golden/` or `want_dir`. A tool's output is deterministic, so a
difference is a behaviour change: a regression, or an intentional change
to re-golden after reviewing the diff. `check()` records a hand-rolled
verdict for the cases this shape does not cover (a pixel round-trip, a
compressed twin).

The tool is `<repo>/bin/<name>`: run `make tools` first.
"""
from __future__ import annotations

import filecmp
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


class Golden:
    def __init__(self, tool: str, tests_file: str, timeout: int = 60) -> None:
        self.here = Path(tests_file).resolve().parent
        self.repo = self.here.parents[2]
        self.name = tool
        self.tool = self.repo / "bin" / tool
        self.fixtures = self.here / "fixtures"
        self.golden = self.here / "golden"
        self.timeout = timeout
        self.results: list[tuple[str, str, list[str]]] = []   # (label, what passed, errors)
        if not self.tool.is_file():
            sys.exit(f"ERROR: {self.tool} not found — run `make tools` first")

    # -- running the tool ---------------------------------------------------

    def stage(self, work: Path, copy=(), write: dict | None = None) -> None:
        """Copy fixtures (a list of names, or "*" for all) and write files into work."""
        names = sorted(p.name for p in self.fixtures.iterdir() if p.is_file()) if copy == "*" else copy
        for n in names:
            shutil.copy(self.fixtures / n, work / n)
        for n, data in (write or {}).items():
            (work / n).write_bytes(data if isinstance(data, bytes) else data.encode("utf-8"))

    def run(self, argv: list[str], work: Path, copy=(), write: dict | None = None
            ) -> subprocess.CompletedProcess:
        """Stage the inputs and run the tool in work."""
        work.mkdir(parents=True, exist_ok=True)
        self.stage(work, copy, write)
        return subprocess.run([str(self.tool), *argv], cwd=work, capture_output=True,
                              text=True, timeout=self.timeout)

    @staticmethod
    def said(proc: subprocess.CompletedProcess) -> str:
        return (proc.stdout or "") + (proc.stderr or "")

    @staticmethod
    def failure(proc: subprocess.CompletedProcess) -> str | None:
        if proc.returncode == 0:
            return None
        return f"exit {proc.returncode}: {(proc.stderr or proc.stdout).strip()[:200]}"

    # -- comparing ----------------------------------------------------------

    def compare(self, work: Path, outputs: list[str], want_dir: Path | None = None) -> list[str]:
        """Every named output exists in work and is byte-identical to want_dir's."""
        want_dir = want_dir or self.golden
        errs = []
        for out in outputs:
            got, want = work / out, want_dir / out
            if not got.is_file():
                errs.append(f"{out}: not produced")
            elif not want.is_file():
                errs.append(f"{out}: no golden at {want} — review the output and commit it")
            elif not filecmp.cmp(got, want, shallow=False):
                errs.append(f"{out}: differs from golden ({got.stat().st_size} vs "
                            f"{want.stat().st_size} bytes)")
        return errs

    def refused(self, work: Path, proc: subprocess.CompletedProcess, needles=(),
                nothing_written=()) -> list[str]:
        """The run failed, said each needle, and wrote none of nothing_written."""
        errs = []
        if proc.returncode == 0:
            errs.append("accepted (exit 0) — must be refused")
        text = self.said(proc)
        errs += [f"refused, but without {n!r}: {text.strip()[-160:]}" for n in needles if n not in text]
        left = [o for o in nothing_written if (work / o).exists()]
        if left:
            errs.append(f"wrote {', '.join(left)}")
        return errs

    # -- the case shapes ----------------------------------------------------

    def record(self, label: str, what: str, errs: list[str]) -> None:
        self.results.append((label, what, errs))

    def check(self, label: str, what: str, fn) -> None:
        """A hand-rolled case: fn() returns the list of errors (empty = pass)."""
        self.record(label, what, fn())

    def expect_outputs(self, label: str, argv: list[str], outputs: list[str], copy=(),
                       write: dict | None = None, want_dir: Path | None = None) -> None:
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            proc = self.run(argv, work, copy, write)
            err = self.failure(proc)
            errs = [err] if err else self.compare(work, outputs, want_dir)
        self.record(label, f"{len(outputs)} output{'s' if len(outputs) != 1 else ''} match", errs)

    def expect_stdout(self, label: str, argv: list[str], golden: str, copy=()) -> None:
        with tempfile.TemporaryDirectory() as td:
            proc = self.run(argv, Path(td), copy)
            err = self.failure(proc)
            if err:
                errs = [err]
            else:
                want = (self.golden / golden).read_text(encoding="utf-8")
                errs = [] if proc.stdout == want else [
                    f"{golden}: {len(proc.stdout)} bytes differ from golden ({len(want)} bytes)"]
        self.record(label, "stdout matches", errs)

    def expect_refused(self, label: str, argv: list[str], copy=(), write: dict | None = None,
                       needles=(), nothing_written=()) -> None:
        with tempfile.TemporaryDirectory() as td:
            work = Path(td)
            proc = self.run(argv, work, copy, write)
            errs = self.refused(work, proc, needles, nothing_written)
        what = "refused" + (", nothing written" if nothing_written else "")
        self.record(label, what, errs)

    # -- the verdict --------------------------------------------------------

    def report(self) -> int:
        for label, what, errs in self.results:
            if errs:
                print(f"  FAIL {label}: " + "; ".join(errs))
            else:
                print(f"  PASS {label} ({what})")
        ok = sum(1 for _, _, errs in self.results if not errs)
        print(f"\n{self.name} golden: {ok}/{len(self.results)} ok")
        return 0 if ok == len(self.results) else 1
