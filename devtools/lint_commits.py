#!/usr/bin/env python3
"""lint_commits.py — enforce commit-message policy mechanically.

Implements the rules documented in .claude/rules/commits.md:

  1. Subject must follow Conventional Commits:
       type(scope): description
     where:
       type   ∈ {feat, fix, perf, refactor, test, docs, chore, build, style, ci, revert}
       scope  ∈ {lib, compiler, runtime, tools, examples, build, ci, devtools,
                 docs, claude, contributing, release, submodule, deps}  (or empty)
       description is non-empty with no trailing period.

     Case of the first letter is NOT checked. Conventional Commits doesn't
     mandate it — the case-first rule was an Angular convention that crept
     into many linters; enforcing it just made contributors fight regex
     exceptions for acronyms (OAM, C99, A1-followup, ...) without buying
     anything real. Style consistency across the history isn't worth the
     external-contributor friction.

  2. Body must NOT contain a `Co-Authored-By:` (or `Co-authored-by:`) trailer.
     The project does not want AI attribution in git history — see commits.md.

Usage:

    python3 devtools/lint_commits.py <range>

`<range>` is anything `git log` understands: `origin/main..HEAD`,
`HEAD~5..HEAD`, a single SHA, etc. CI passes the PR's commit range; locally
a contributor can run `make lint-commits` (defaults to `origin/develop..HEAD`).

Exit codes:
    0 — every commit in the range passes
    1 — at least one violation
    2 — bad invocation / git error
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys

# Match the leading `type(scope): description` line. Scope is optional. The
# allow-lists below are intentionally narrow — broaden them by editing this
# file (and updating .claude/rules/commits.md in the same commit).
ALLOWED_TYPES = {
    "feat", "fix", "perf", "refactor", "test", "docs",
    "chore", "build", "style", "ci", "revert",
}
ALLOWED_SCOPES = {
    # Per CLAUDE.md "Scopes":
    "lib", "compiler", "runtime", "tools", "examples", "build",
    # Practical extensions used in this repo's history:
    "ci", "devtools", "docs", "claude", "contributing", "release",
    "submodule", "deps", "readme", "changelog", "test", "tests",
    # Emerged-organically categories (added 2026-05-13 alongside the
    # function-inlining + lib-retrofit chantier batch — these match
    # real directories / files in the repo):
    #   `chantiers` -> .claude/notes/chantiers/
    #   `rules`     -> .claude/rules/
    #   `bench`     -> tools/opensnes-emu/test/fixtures/benchmark/
    "chantiers", "rules", "bench",
}

SUBJECT_RE = re.compile(
    r"^(?P<type>[a-z]+)(?:\((?P<scope>[a-z0-9_/-]+)\))?(?P<bang>!)?: (?P<desc>.+)$"
)
COAUTHOR_RE = re.compile(r"^\s*co-authored-by\s*:", re.IGNORECASE | re.MULTILINE)


def get_commits(rev_range: str) -> list[tuple[str, str]]:
    """Return [(sha, full_message)] for every commit in `rev_range`."""
    try:
        shas = subprocess.run(
            ["git", "log", "--format=%H", rev_range],
            check=True, capture_output=True, text=True,
        ).stdout.split()
    except subprocess.CalledProcessError as e:
        sys.stderr.write(f"error: git log failed: {e.stderr.strip()}\n")
        sys.exit(2)

    commits: list[tuple[str, str]] = []
    for sha in shas:
        msg = subprocess.run(
            ["git", "log", "-1", "--format=%B", sha],
            check=True, capture_output=True, text=True,
        ).stdout
        commits.append((sha, msg))
    return commits


def check_subject(subject: str) -> list[str]:
    """Return a list of human-readable violations for one subject line.

    Length is intentionally NOT checked: Conventional Commits recommends ≤ 72
    chars but it's a soft preference, not a correctness rule. Our existing
    history has entries up to ~80 chars and rejecting them would force
    contributors to abbreviate scope to fit, which hurts readability more
    than it helps.
    """
    errors: list[str] = []

    m = SUBJECT_RE.match(subject)
    if not m:
        errors.append(
            "subject does not match `type(scope): description` "
            "(Conventional Commits)"
        )
        return errors

    t = m.group("type")
    s = m.group("scope")
    desc = m.group("desc")

    if t not in ALLOWED_TYPES:
        errors.append(
            f"type {t!r} is not in {sorted(ALLOWED_TYPES)}"
        )
    if s is not None and s not in ALLOWED_SCOPES:
        errors.append(
            f"scope {s!r} is not in {sorted(ALLOWED_SCOPES)}"
        )
    if not desc:
        errors.append("empty description")
    elif desc.endswith("."):
        errors.append("description should not end with a period")

    return errors


def check_body(body: str) -> list[str]:
    errors: list[str] = []
    if COAUTHOR_RE.search(body):
        errors.append(
            "body contains a `Co-Authored-By:` trailer "
            "(forbidden by .claude/rules/commits.md)"
        )
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__.split("\n\n")[0],
    )
    parser.add_argument(
        "rev_range",
        help="git log range, e.g. `origin/develop..HEAD` or `HEAD~5..HEAD`",
    )
    args = parser.parse_args()

    commits = get_commits(args.rev_range)
    if not commits:
        print(f"OK: no commits in range {args.rev_range}")
        return 0

    failed = 0
    skipped_merges = 0
    for sha, msg in commits:
        lines = msg.splitlines()
        subject = lines[0] if lines else ""
        body = "\n".join(lines[2:]) if len(lines) > 2 else ""

        # Skip GitHub-style merge commits. `gh pr merge --merge` and the
        # web "Merge pull request" button both produce subjects of the
        # form `Merge pull request #N from owner/branch` (or `Merge
        # branch 'X' into Y`). These are autogenerated wrappers around
        # contributor commits — the contributor's own commits are
        # already in the range and get linted individually. Asking the
        # release flow to also satisfy Conventional Commits on the
        # merge wrapper would require either rewriting GitHub's output
        # or banning --merge entirely; both are worse than ignoring
        # this specific class of subject. Body still gets the
        # Co-Authored-By check via check_body() above to catch the
        # `Co-Authored-By:` trailer that GitHub never inserts but a
        # contributor amend might.
        if subject.startswith("Merge pull request ") \
           or subject.startswith("Merge branch ") \
           or subject.startswith("Merge remote-tracking branch "):
            skipped_merges += 1
            errors = check_body(body)
        else:
            errors = check_subject(subject) + check_body(body)
        if errors:
            failed += 1
            print(f"\n--- {sha[:12]}  {subject!r} ---")
            for e in errors:
                print(f"  - {e}")

    if failed:
        print(
            f"\n{failed} commit(s) failed lint. "
            "See .claude/rules/commits.md for the policy."
        )
        return 1

    note = f" ({skipped_merges} merge commit(s) skipped)" if skipped_merges else ""
    print(f"OK: all {len(commits)} commits pass lint{note}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
