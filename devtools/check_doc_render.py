#!/usr/bin/env python3
"""Doc-render leak check — scan the GENERATED Doxygen HTML for un-rendered Markdown.

Run AFTER `make docs`. Catches the class where an inline code/emphasis span fails
to render and leaks into the page as visible text — two symptoms of the same
Doxygen markdown bug (an inline `code` span sitting inside an ASCII "..." quote):

  1. literal escaped tags:  &lt;tt&gt;left&lt;/tt&gt;   (shows "<tt>left</tt>")
  2. raw backtick spans:    "read from bank `$00`"      (backticks shown verbatim)

A correctly-rendered code span becomes <span class="tt">…</span> with NO angle-tag
text and NO backticks, so any of the above in prose is a real defect.

Pure stdlib. A source-side regex can't reliably detect this (a `code` span BETWEEN
two quotes is indistinguishable from one INSIDE a quote), so we check the rendered
output instead — zero false positives. Skips `*_source.html` (verbatim code
listings) and Doxygen code fragments.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

HTML_DIR = Path(__file__).resolve().parent.parent / "docs" / "build" / "html"

LITERAL_TAG = re.compile(r"&lt;/?(?:tt|em|b|i|strong|code|sub|sup|br)&gt;")
# A raw backtick code-span sitting inside a "..." quote, in prose (not a fragment).
QUOTE_CODE = re.compile(r'"[^"<]*`[^`<]+`[^"<]*"')


def main() -> int:
    if not HTML_DIR.is_dir():
        print(f"doc-render: {HTML_DIR} not found — run `make docs` first (skipping).")
        return 0
    findings: list[str] = []
    for f in sorted(HTML_DIR.glob("*.html")):
        if f.name.endswith("_source.html"):
            continue  # source-code listings legitimately contain backticks/brackets
        for lineno, line in enumerate(f.read_text(errors="replace").splitlines(), 1):
            if "class=\"fragment\"" in line or "<pre" in line:
                continue  # code blocks
            if LITERAL_TAG.search(line):
                findings.append(f"{f.name}:{lineno}: literal inline-format tag — "
                                "un-rendered Markdown (inline code/emphasis inside a \"...\" quote?)")
            if QUOTE_CODE.search(line):
                findings.append(f"{f.name}:{lineno}: raw backtick code-span in prose — "
                                "`code` inside a \"...\" quote; move the code outside the quotes")
    for x in findings:
        print(x)
    if findings:
        print(f"\ndoc-render: {len(findings)} leak(s) — see "
              "the 'code spans inside \"...\"' note in CONTRIBUTING / doc_consistency.")
        return 1
    print("doc-render: OK (no un-rendered Markdown in the generated HTML).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
