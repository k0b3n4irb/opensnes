#!/usr/bin/env python3
"""Doxygen input filter for markdown files.

When multiple README.md files exist in different directories, Doxygen cannot
distinguish them. This filter injects a \\page directive at the top of each
markdown file, using the parent directory path as a unique page identifier.

Usage in Doxyfile:
    FILTER_PATTERNS = *.md="python3 doxygen_md_filter.py"

Example:
    examples/text/hello_world/README.md  ->  \\page examples_text_hello_world Hello World
    examples/games/breakout/README.md    ->  \\page examples_games_breakout Breakout
    docs/hardware/MEMORY_MAP.md          ->  \\page memory_map SNES Memory Map
    CONTRIBUTING.md  (with {#contributing})  ->  \\page contributing Contributing to OpenSNES
    README.md (root)                     ->  (passed through unchanged)
"""

import os
import re
import sys


def main():
    # Force UTF-8 on stdout (Windows/MSYS2 defaults to cp1252)
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    if len(sys.argv) < 2:
        sys.exit(1)

    filepath = sys.argv[1]
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    rel = os.path.relpath(filepath)
    basename = os.path.basename(rel)

    # Don't filter the root README. mainpage.md is an ordinary page
    # ({#documentation}): Doxygen's own index.html is replaced by the landing
    # page, so the documentation hub must not be the main page.
    if rel == "README.md":
        sys.stdout.write(content)
        return

    # Extract the first # heading and optional explicit {#page_id}
    match = re.match(r"^#\s+(.+?)(?:\s*\{#([^}]+)\})?\s*$", content, re.MULTILINE)
    if not match:
        sys.stdout.write(content)
        return

    title = match.group(1).strip()
    page_id = _page_id(rel, match.group(2))

    if not page_id:
        sys.stdout.write(content)
        return

    content = _rewrite_links(content, filepath)

    # Inject \page before the first heading, replace the heading with \page
    new_first_line = f"\\page {page_id} {title}"
    content = re.sub(
        r"^#\s+.+$",
        lambda m: new_first_line,
        content,
        count=1,
        flags=re.MULTILINE,
    )

    # For README.md files only, auto-inject \subpage directives for every
    # immediate sub-directory that itself contains a README.md. This keeps
    # the navigation hierarchical without requiring each README to manually
    # list its children, and Doxygen treats \subpage inside a \page body as
    # a parent→child link.
    if basename == "README.md":
        children = _discover_subpage_children(filepath)
        if children:
            block = "\n\n<!-- Auto-generated subpage links (doxygen_md_filter.py) -->\n"
            for child_id in children:
                block += f"\\subpage {child_id}\n"
            block += "<!-- /auto-generated -->\n"
            content = content.rstrip() + block

    sys.stdout.write(content)


def _page_id(rel: str, explicit_id: str | None) -> str:
    """The page ID of the Markdown file at `rel` (relative to docs/)."""
    if explicit_id:
        # Use the explicit {#page_id} from the heading
        return explicit_id.strip()
    # Auto-generate page ID from directory path
    dirpath = os.path.dirname(rel)
    # Normalize: strip ".." components (from ../examples/ paths)
    parts = [
        p
        for p in dirpath.replace(os.sep, "/").split("/")
        if p and p != ".."
    ]
    # For non-README files, append the filename stem
    basename = os.path.basename(rel)
    if basename != "README.md":
        parts.append(os.path.splitext(basename)[0])
    return "_".join(parts).replace("-", "_")


REPO = "https://github.com/k0b3n4irb/opensnes/blob/develop/"
_page_files: set[str] | None = None


def _pages() -> set[str]:
    """Absolute paths of the Markdown files Doxygen makes pages of: those the
    Doxyfile's INPUT names, or that sit under a directory it names, minus its
    EXCLUDE_PATTERNS and the root README (passed through, not a page)."""
    global _page_files
    if _page_files is not None:
        return _page_files
    text = open("Doxyfile", encoding="utf-8").read()
    def setting(name: str) -> list[str]:
        m = re.search(rf"^{name}\s*=((?:.*\\\n)*.*)$", text, re.MULTILINE)
        return m.group(1).replace("\\\n", " ").split() if m else []
    excludes = [p.strip("*/") for p in setting("EXCLUDE_PATTERNS")]
    root = os.path.abspath("..")
    found: set[str] = set()
    for entry in setting("INPUT"):
        path = os.path.abspath(entry)
        if os.path.isfile(path) and path.endswith(".md"):
            found.add(path)
        elif os.path.isdir(path):
            for d, _, files in os.walk(path):
                found.update(os.path.join(d, f) for f in files if f.endswith(".md"))
    _page_files = {
        p for p in found
        if p != os.path.join(root, "README.md")
        and not any(f"/{e}/" in p or p.endswith(e) for e in excludes)
    }
    return _page_files


_LINK = re.compile(r"\]\(([^()\s#]+)(#[^()\s]*)?\)")


def _rewrite_links(content: str, filepath: str) -> str:
    """Point relative links to another Markdown file at that file's PAGE.

    Doxygen resolves `[text](tutorials/dma.md)` to the empty "dma.md File
    Reference" page, not to the tutorial (419 such links on 2026-10-14). A
    link to a page becomes `page_id.html` (its anchor kept); a link to a
    Markdown file that is not a page (ROADMAP.md, CHANGELOG.md, docs/README.md)
    becomes its URL on GitHub. A link to a directory with a README page goes
    to that page. Links inside fenced code are left alone; so are absolute
    URLs and links that do not resolve to a file.
    """
    here = os.path.dirname(os.path.abspath(filepath))
    root = os.path.abspath("..")
    pages = _pages()

    def target(m: re.Match) -> str:
        link, anchor = m.group(1), m.group(2) or ""
        if "://" in link or link.startswith(("mailto:", "@")):
            return m.group(0)
        path = os.path.normpath(os.path.join(here, link))
        if os.path.isdir(path) and os.path.join(path, "README.md") in pages:
            path = os.path.join(path, "README.md")
        if not path.endswith(".md") or not os.path.isfile(path):
            return m.group(0)
        if path in pages:
            head = re.search(r"^#\s+(.+?)(?:\s*\{#([^}]+)\})?\s*$",
                             open(path, encoding="utf-8").read(), re.MULTILINE)
            page = _page_id(os.path.relpath(path), head.group(2) if head else None)
            return f"]({page}.html{anchor})"
        if path.startswith(root + os.sep):
            return f"]({REPO}{os.path.relpath(path, root)}{anchor})"
        return m.group(0)

    out, fence = [], False
    for line in content.split("\n"):
        if line.lstrip().startswith(("```", "~~~")):
            fence = not fence
        out.append(line if fence else _LINK.sub(target, line))
    return "\n".join(out)


def _discover_subpage_children(filepath: str) -> list[str]:
    """Return page IDs for every direct sub-directory's README.md.

    The IDs follow the same dirpath→underscore convention used for the
    parent page, so they line up with Doxygen's \\page declarations
    elsewhere in the tree.
    """
    parent_dir = os.path.dirname(os.path.abspath(filepath))
    if not os.path.isdir(parent_dir):
        return []
    children: list[str] = []
    for entry in sorted(os.listdir(parent_dir)):
        sub = os.path.join(parent_dir, entry)
        if not os.path.isdir(sub):
            continue
        child_readme = os.path.join(sub, "README.md")
        if not os.path.isfile(child_readme):
            continue
        # Derive the child page ID the same way the main page-id pass does:
        # take the relative path to cwd, strip leading ".." components, join
        # with underscores.
        rel = os.path.relpath(sub)
        parts = [
            p
            for p in rel.replace(os.sep, "/").split("/")
            if p and p != ".."
        ]
        if not parts:
            continue
        children.append("_".join(parts).replace("-", "_"))
    return children


if __name__ == "__main__":
    main()
