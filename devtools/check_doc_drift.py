#!/usr/bin/env python3
"""Doc-drift sentinel for OpenSNES.

Compares anchored claims across multiple docs against the canonical sources of
truth, and exits non-zero if any drift is detected.

Anchored claims currently watched:

  1. ``lib/include/snes.h`` ``OPENSNES_VERSION_{MAJOR,MINOR,PATCH,STRING}``
     must match the head version in ``CHANGELOG.md`` (the topmost
     ``## [X.Y.Z]`` heading after ``## [Unreleased]``).

  2. ``ROADMAP.md`` "Current Status: post-vX.Y.Z" line must match
     ``CHANGELOG.md`` head — i.e. the in-development line cannot be more
     than one minor version behind the latest release.

  3. ``find examples -name 'main.c' | wc -l`` must match every "N example(s)"
     claim in active rule files (``.claude/rules/*.md``) and ``ROADMAP.md``.
     Historical entries in ``CHANGELOG.md`` are exempt — they freeze a
     past state.

Why this exists: the v0.1.0-dev macro stuck at v0.16.0, the post-v0.13.0
ROADMAP stale by three minor versions, and the "53 examples" / "54 examples"
count drift across rules — all of these were caught by a 1-day external
review and would have been caught earlier by this script. See the related
``.claude/rules/doc_consistency.md`` and the lint job in ``.github/workflows/lint.yml``.

Usage:

    python3 devtools/check_doc_drift.py            # run all checks
    python3 devtools/check_doc_drift.py --fix      # autofix where safe (TBD)
    python3 devtools/check_doc_drift.py --quiet    # only print drift / errors

Exit codes:
    0   no drift
    1   drift detected (or invocation error)
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


# --------------------------------------------------------------------------
# Helpers
# --------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parent.parent


def repo_path(*parts: str) -> Path:
    return REPO_ROOT.joinpath(*parts)


class Drift(Exception):
    """A specific drift finding. Aggregated and reported at the end."""


# --------------------------------------------------------------------------
# Canonical truth: head version + examples count
# --------------------------------------------------------------------------

CHANGELOG_HEAD_RE = re.compile(
    r"^##\s*\[(?P<ver>\d+\.\d+\.\d+)\]\s*[—-]\s*(?P<date>\d{4}-\d{2}-\d{2})",
    re.MULTILINE,
)


def canonical_version() -> tuple[str, str]:
    """Return (version, date) of the head release in CHANGELOG.md.

    Skips ``## [Unreleased]`` if present.
    """
    text = repo_path("CHANGELOG.md").read_text(encoding="utf-8")
    for match in CHANGELOG_HEAD_RE.finditer(text):
        return match.group("ver"), match.group("date")
    raise SystemExit("CHANGELOG.md: no '## [X.Y.Z] - YYYY-MM-DD' heading found")


def canonical_examples_count() -> int:
    """Number of example ROMs (one ``main.c`` per example)."""
    examples = list(repo_path("examples").rglob("main.c"))
    return len(examples)


# --------------------------------------------------------------------------
# Check 1: snes.h version macros
# --------------------------------------------------------------------------

VERSION_RE = {
    "MAJOR": re.compile(r"#define\s+OPENSNES_VERSION_MAJOR\s+(\d+)"),
    "MINOR": re.compile(r"#define\s+OPENSNES_VERSION_MINOR\s+(\d+)"),
    "PATCH": re.compile(r"#define\s+OPENSNES_VERSION_PATCH\s+(\d+)"),
    "STRING": re.compile(r'#define\s+OPENSNES_VERSION_STRING\s+"([^"]+)"'),
}


def check_snes_h_version(canonical: str) -> list[str]:
    """Return drift messages for ``lib/include/snes.h`` version macros."""
    drifts: list[str] = []
    header = repo_path("lib/include/snes.h").read_text(encoding="utf-8")
    major, minor, patch = canonical.split(".")

    expected = {
        "MAJOR": major,
        "MINOR": minor,
        "PATCH": patch,
        "STRING": canonical,
    }

    for key, rx in VERSION_RE.items():
        match = rx.search(header)
        if not match:
            drifts.append(f"lib/include/snes.h: OPENSNES_VERSION_{key} macro missing")
            continue
        actual = match.group(1)
        if actual != expected[key]:
            drifts.append(
                f"lib/include/snes.h: OPENSNES_VERSION_{key} = {actual!r} "
                f"but CHANGELOG.md head is {expected[key]!r}"
            )
    return drifts


# --------------------------------------------------------------------------
# Check 2: ROADMAP.md "post-vX.Y.Z" line
# --------------------------------------------------------------------------

ROADMAP_STATUS_RE = re.compile(
    r"^## Current Status:\s*post-v(?P<ver>\d+\.\d+\.\d+)",
    re.MULTILINE,
)


def check_roadmap_status(canonical: str) -> list[str]:
    text = repo_path("ROADMAP.md").read_text(encoding="utf-8")
    match = ROADMAP_STATUS_RE.search(text)
    if not match:
        return ["ROADMAP.md: '## Current Status: post-vX.Y.Z' line not found"]
    actual = match.group("ver")
    if actual != canonical:
        return [
            f"ROADMAP.md: 'Current Status: post-v{actual}' but CHANGELOG.md "
            f"head is {canonical} — bump the line in the same release commit"
        ]
    return []


# --------------------------------------------------------------------------
# Check 3: examples count consistency across active docs
# --------------------------------------------------------------------------

# Look for "<N> example(s)" or "All <N> examples" in active docs.
# Active = ROADMAP.md + .claude/rules/*.md.
# Excluded = CHANGELOG.md (historical) and any docs/ tutorial that pins a
# past state intentionally.
COUNT_PATTERNS = [
    re.compile(r"\b(\d{2,3})\s+working\s+examples?\b", re.IGNORECASE),
    re.compile(r"\b(\d{2,3})\s+examples?\s+(?:cover|organized|across)", re.IGNORECASE),
    re.compile(r"\bAll\s+(\d{2,3})\s+examples?\b", re.IGNORECASE),
    re.compile(r"\ball\s+(\d{2,3})\s+examples\s+compile\s+cleanly\b", re.IGNORECASE),
]

# Files where an example count is allowed to be stale (historical record).
COUNT_STALE_OK_GLOBS = ["CHANGELOG.md"]


def _gather_active_doc_paths() -> list[Path]:
    paths: list[Path] = [repo_path("ROADMAP.md"), repo_path("README.md")]
    rules = repo_path(".claude/rules")
    if rules.is_dir():
        paths.extend(sorted(rules.glob("*.md")))
    return paths


def check_examples_count(canonical: int) -> list[str]:
    seen: set[tuple[str, int, int]] = set()
    drifts: list[str] = []
    for path in _gather_active_doc_paths():
        if path.name in COUNT_STALE_OK_GLOBS:
            continue
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8")
        for lineno, line in enumerate(text.splitlines(), 1):
            for rx in COUNT_PATTERNS:
                m = rx.search(line)
                if not m:
                    continue
                count = int(m.group(1))
                # Skip obviously-irrelevant numbers (e.g. "all 256 colours").
                if count < 10 or count > 999:
                    continue
                if count == canonical:
                    continue
                rel = str(path.relative_to(REPO_ROOT))
                key = (rel, lineno, count)
                if key in seen:
                    continue
                seen.add(key)
                drifts.append(
                    f"{rel}:{lineno}: claims {count} example(s) but "
                    f"`find examples -name main.c | wc -l = {canonical}` — "
                    f"update the line or, if it's an off-by-one drift, "
                    f"replace the number with `<run --list>`-style language"
                )
    return drifts


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------


def run_checks(quiet: bool) -> int:
    canonical_ver, canonical_date = canonical_version()
    canonical_n = canonical_examples_count()

    if not quiet:
        print(f"canonical version : {canonical_ver} ({canonical_date})")
        print(f"canonical examples: {canonical_n}")
        print()

    all_drifts: list[str] = []
    all_drifts.extend(check_snes_h_version(canonical_ver))
    all_drifts.extend(check_roadmap_status(canonical_ver))
    all_drifts.extend(check_examples_count(canonical_n))

    if all_drifts:
        print("DRIFT DETECTED:", file=sys.stderr)
        for d in all_drifts:
            print(f"  - {d}", file=sys.stderr)
        print(
            f"\nFix the items above and rerun `make lint-docs`. See "
            f"`.claude/rules/doc_consistency.md` for the full policy.",
            file=sys.stderr,
        )
        return 1

    if not quiet:
        print("OK: no doc drift detected.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="OpenSNES doc-drift sentinel — see .claude/rules/doc_consistency.md",
    )
    parser.add_argument("--quiet", action="store_true",
                        help="suppress non-error output")
    args = parser.parse_args()
    try:
        return run_checks(args.quiet)
    except FileNotFoundError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
