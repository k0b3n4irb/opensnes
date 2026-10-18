#!/usr/bin/env python3
"""Point the generated site's links to a Markdown FILE page at its PAGE.

Doxygen makes two things of each Markdown input: the page (getting_started.html,
the text) and an empty "getting_started.md File Reference" page. A file name
written in the text (`KNOWN_LIMITATIONS.md`, in prose or in a C comment) is
autolinked to the empty one. docs/doxygen_md_filter.py rewrites Markdown links
before Doxygen sees them; this rewrites what Doxygen links by itself, after.

Run by `make docs` from the repository root, on docs/build/html.
"""
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HTML = ROOT / "docs" / "build" / "html"


def escape(name: str) -> str:
    """Doxygen's file-name escaping for a compound's output file."""
    out = []
    for c in name:
        out.append({"_": "__", ".": "_8", "/": "_2", ":": "_1", " ": "_01"}.get(c, c))
    return "".join(out)


def main() -> int:
    sys.path.insert(0, str(ROOT / "docs"))
    os.chdir(ROOT / "docs")          # the filter resolves paths from docs/
    import doxygen_md_filter as f    # noqa: E402

    mapping: dict[str, str] = {}
    for path in sorted(f._pages()):
        text = open(path, encoding="utf-8").read()
        head = re.search(r"^#\s+(.+?)(?:\s*\{#([^}]+)\})?\s*$", text, re.MULTILINE)
        if not head:
            continue
        page = f._page_id(os.path.relpath(path), head.group(2))
        parts = Path(path).relative_to(ROOT).parts
        for i in range(len(parts)):     # longest suffix first: the first that
            name = escape("/".join(parts[i:])) + ".html"   # exists is this file's
            if (HTML / name).is_file():
                mapping[name] = f"{page}.html"
                break

    pattern = re.compile(r'href="(' + "|".join(map(re.escape, mapping)) + r')(#[^"]*)?"')
    changed = 0
    for page in HTML.glob("*.html"):
        if page.name.endswith("_8md.html"):
            continue
        text = page.read_text(encoding="utf-8")
        new, n = pattern.subn(lambda m: f'href="{mapping[m.group(1)]}{m.group(2) or ""}"', text)
        if n:
            page.write_text(new, encoding="utf-8")
            changed += n
    print(f"docs_relink: {changed} links from {len(mapping)} Markdown file pages moved to their page")
    return 0


if __name__ == "__main__":
    sys.exit(main())
