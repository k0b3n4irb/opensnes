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
    if len(sys.argv) < 2:
        sys.exit(1)

    filepath = sys.argv[1]
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    rel = os.path.relpath(filepath)
    basename = os.path.basename(rel)

    # Don't filter the root README or mainpage (handled by USE_MDFILE_AS_MAINPAGE)
    if rel == "README.md" or basename == "mainpage.md":
        sys.stdout.write(content)
        return

    # Extract the first # heading and optional explicit {#page_id}
    match = re.match(r"^#\s+(.+?)(?:\s*\{#([^}]+)\})?\s*$", content, re.MULTILINE)
    if not match:
        sys.stdout.write(content)
        return

    title = match.group(1).strip()
    explicit_id = match.group(2)

    if explicit_id:
        # Use the explicit {#page_id} from the heading
        page_id = explicit_id.strip()
    else:
        # Auto-generate page ID from directory path
        dirpath = os.path.dirname(rel)
        # Normalize: strip ".." components (from ../examples/ paths)
        parts = [
            p
            for p in dirpath.replace(os.sep, "/").split("/")
            if p and p != ".."
        ]

        # For non-README files, append the filename stem
        if basename != "README.md":
            stem = os.path.splitext(basename)[0]
            parts.append(stem)

        page_id = "_".join(parts).replace("-", "_")

    if not page_id:
        sys.stdout.write(content)
        return

    # Inject \page before the first heading, replace the heading with \page
    new_first_line = f"\\page {page_id} {title}"
    content = re.sub(
        r"^#\s+.+$",
        lambda m: new_first_line,
        content,
        count=1,
        flags=re.MULTILINE,
    )

    sys.stdout.write(content)


if __name__ == "__main__":
    main()
