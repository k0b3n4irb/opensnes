#!/usr/bin/env python3
"""Refuse a download in the CI workflows or the scripts that has no fallback.

On 2026-11-08 GitHub answered 502/503/504 to release assets and source
archives for about an hour; every `curl` without a retry turned a green
tree red (seven runs, a release held back). A download must either carry its
own retry (`curl --retry`, `wget --tries`) or say, on its own line or the one
above, that something else retries it:  `# retried: <what retries it>`.

Checked: `.github/workflows/*.yml` and `scripts/*.sh`. A line that only
mentions the word in a comment is not a command and is skipped; a command
continued with a trailing backslash is read as one line.

Usage:  python3 devtools/check_ci_downloads.py [ROOT]     (exit 1 on a fault)
"""
import re
import sys
from pathlib import Path

CMD = re.compile(r"(?<![\w./-])(curl|wget)(?=\s)")
MARK = re.compile(r"#\s*retried:\s*\S")


def logical_lines(text):
    """Yield (first_line_number, joined_text, previous_physical_line)."""
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        start = i
        joined = lines[i]
        while joined.rstrip().endswith("\\") and i + 1 < len(lines):
            i += 1
            joined = joined.rstrip()[:-1] + " " + lines[i].strip()
        prev = lines[start - 1] if start > 0 else ""
        yield start + 1, joined, prev
        i += 1


def faults_in(text):
    out = []
    for no, line, prev in logical_lines(text):
        code = line.split("#", 1)[0] if line.lstrip().startswith("#") is False else ""
        m = CMD.search(code)
        if not m:
            continue
        tool = m.group(1)
        retry_flag = "--retry" if tool == "curl" else "--tries"
        if retry_flag in code:
            continue
        if MARK.search(line) or MARK.search(prev):
            continue
        out.append((no, tool, line.strip()))
    return out


def targets(root):
    root = Path(root)
    return sorted(list((root / ".github" / "workflows").glob("*.yml")) + list((root / "scripts").glob("*.sh")))


def main(argv):
    root = argv[1] if len(argv) > 1 else "."
    bad = 0
    files = targets(root)
    if not files:
        print("check_ci_downloads: no workflow or script found under", root, file=sys.stderr)
        return 2
    for f in files:
        for no, tool, line in faults_in(f.read_text(encoding="utf-8", errors="replace")):
            print(f"{f}:{no}: {tool} without a retry (add --retry/--tries, or '# retried: <what>'): {line[:110]}")
            bad += 1
    if bad:
        return 1
    print(f"OK: every download of {len(files)} workflow/script files has a retry")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
