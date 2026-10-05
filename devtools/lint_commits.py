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

  3. Author and committer are the maintainer, `k0b3n4irb <k0b3n4irb@gmail.com>`
     (AUTHOR_NAME / AUTHOR_EMAILS below). No bot (`dependabot[bot]`,
     `github-actions[bot]`), no `noreply` address, no tool: a change a bot
     proposes is applied by hand and committed as the maintainer's own. In
     --message-file mode the identity comes from `git var GIT_AUTHOR_IDENT`,
     so the commit-msg hook refuses the commit before it exists
     (2026-10-06, after Dependabot's PR #163 — see commits.md).

Usage:

    python3 devtools/lint_commits.py <range>

`<range>` is anything `git log` understands: `origin/main..HEAD`,
`HEAD~5..HEAD`, a single SHA, etc. CI passes the PR's commit range; locally
a contributor can run `make lint-commits` (defaults to `origin/develop..HEAD`).

    python3 devtools/lint_commits.py --message-file .git/COMMIT_EDITMSG

lints one message before it becomes a commit. `make hooks` installs the
`commit-msg` and `pre-push` hooks of `scripts/githooks/`, which run these two
forms: two non-conforming subjects reached `develop` in two days (`756da353`,
`0d30ec65`) because the lint ran after the push, where only CI reads it.

Exit codes:
    0 — every commit in the range passes
    1 — at least one violation
    2 — bad invocation / git error
"""

from __future__ import annotations

import argparse
import pathlib
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
    #   `bench`     -> devtools/cyclecount/ (cycle-count benchmark fixtures)
    "chantiers", "rules", "bench",
    # luna test-harness migration (2026-06-21): real path testing/
    "luna-test",
    "testing",
    # conventions notes (2026-06-23): real path .claude/notes/conventions/
    # (sibling of `chantiers`/`rules` above).
    "conventions",
    # tech notes (2026-07-26): real path .claude/notes/tech/ (sibling of
    # `chantiers`/`conventions` above — technical references and analyses).
    "tech",
    # status notes (2026-09-22): real path .claude/notes/status/ (sibling of
    # `tech`/`conventions` above — the current state of a moving target).
    # Used since 7408483c; the lint only noticed at the v0.44.0 release,
    # because a push lints its own commits, never the accumulated range.
    "status",
    # game-craft docs (2026-08-02): real path docs/craft/ (sibling of
    # docs/tutorials/ — design/decision guides, not API reference).
    "craft",
}

# The one identity every commit on develop and main carries (commits.md,
# "One author"). History before 2026-10-06 holds two older spellings of the
# same person (`K0b3 <K0b3@nowhere.zz>`, `k0b3n4irb@nowhere.zz`); the lint
# runs on a push range, never on that history.
AUTHOR_NAME = "k0b3n4irb"
AUTHOR_EMAILS = {"k0b3n4irb@gmail.com"}

SUBJECT_RE = re.compile(
    r"^(?P<type>[a-z]+)(?:\((?P<scope>[a-z0-9_/, -]+)\))?(?P<bang>!)?: (?P<desc>.+)$"
)
COAUTHOR_RE = re.compile(r"^\s*co-authored-by\s*:", re.IGNORECASE | re.MULTILINE)
RELEASE_MERGE_RE = re.compile(r"^release: v\d+\.\d+\.\d+$")


def check_identity(role: str, name: str, email: str) -> list[str]:
    """The author or committer identity against the one the project allows."""
    errors: list[str] = []
    if name != AUTHOR_NAME or email not in AUTHOR_EMAILS:
        who = f"{name} <{email}>"
        why = "a bot" if "[bot]" in name else "a noreply address" if "noreply" in email else "not the maintainer's identity"
        errors.append(
            f"{role} is {who!r}: {why} — every commit is authored and committed by "
            f"{AUTHOR_NAME} <{sorted(AUTHOR_EMAILS)[0]}> (.claude/rules/commits.md, One author)"
        )
    return errors


def identities(sha: str) -> list[str]:
    """Violations of the author's and committer's identity of one commit."""
    out = subprocess.run(
        ["git", "log", "-1", "--format=%an%x00%ae%x00%cn%x00%ce", sha],
        check=True, capture_output=True, text=True,
    ).stdout.strip("\n").split("\0")
    if len(out) != 4:
        return [f"cannot read the identities of {sha}"]
    return check_identity("author", out[0], out[1]) + check_identity("committer", out[2], out[3])


def pending_identity() -> list[str]:
    """The identity of the commit being made (the commit-msg hook): `git var`."""
    errors: list[str] = []
    for role, var in (("author", "GIT_AUTHOR_IDENT"), ("committer", "GIT_COMMITTER_IDENT")):
        ident = subprocess.run(["git", "var", var], capture_output=True, text=True).stdout.strip()
        m = re.match(r"^(.*?) <([^>]*)> \d+ [+-]\d{4}$", ident)
        if not m:
            errors.append(f"cannot read the {role} identity (`git var {var}` said {ident!r})")
            continue
        errors += check_identity(role, m.group(1), m.group(2))
    return errors


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
    if s is not None:
        # A comma-separated list of scopes is allowed (e.g. `feat(compiler,lib):`);
        # each must be valid.
        for one in (part.strip() for part in s.split(",")):
            if one not in ALLOWED_SCOPES:
                errors.append(
                    f"scope {one!r} is not in {sorted(ALLOWED_SCOPES)}"
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
        "rev_range", nargs="?",
        help="git log range, e.g. `origin/develop..HEAD` or `HEAD~5..HEAD`",
    )
    parser.add_argument(
        "--message-file", metavar="PATH",
        help="lint one commit message read from PATH instead of a range "
             "(the `commit-msg` git hook in scripts/githooks passes "
             "$GIT_DIR/COMMIT_EDITMSG)",
    )
    args = parser.parse_args()
    if (args.rev_range is None) == (args.message_file is None):
        parser.error("give a rev range or --message-file, not both")

    if args.message_file:
        # Strip the lines git treats as comments, as git itself does
        # (commit.cleanup=strip, the default for an edited message).
        text = pathlib.Path(args.message_file).read_text(encoding="utf-8")
        kept = [l for l in text.splitlines() if not l.startswith("#")]
        while kept and not kept[0].strip():
            kept.pop(0)
        commits = [("(message)", "\n".join(kept) + "\n")]
    else:
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
        # The release merge is titled `release: vX.Y.Z` by .claude/rules/release.md
        # (step 1) — a merge wrapper too, not a contributor commit; without this
        # exemption every Lint run on main after a release was red (v0.48.0,
        # 2026-10-05).
        if subject.startswith("Merge pull request ") \
           or subject.startswith("Merge branch ") \
           or subject.startswith("Merge remote-tracking branch ") \
           or RELEASE_MERGE_RE.match(subject):
            skipped_merges += 1
            errors = check_body(body)
        else:
            errors = check_subject(subject) + check_body(body)
        # Who made it: the maintainer, in person — a bot's PR is closed and its
        # change re-applied by hand (commits.md, One author). The merge commits
        # are held to it too: GitHub's web merge button would commit as
        # `GitHub <noreply@github.com>`.
        errors += pending_identity() if sha == "(message)" else identities(sha)
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
