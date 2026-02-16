#!/usr/bin/env python3
"""Doxygen input filter for markdown files.

When multiple README.md files exist in different directories, Doxygen cannot
distinguish them. This filter injects a \\page directive at the top of each
markdown file, using the parent directory path as a unique page identifier.

Usage in Doxyfile:
    FILTER_PATTERNS = *.md="python3 docs/doxygen_md_filter.py"

Example:
    examples/text/hello_world/README.md  ->  \\page examples_text_hello_world Hello World
    examples/games/breakout/README.md    ->  \\page examples_games_breakout Breakout
    README.md (root)                     ->  (passed through unchanged)
"""

import os
import re
import sys


def main():
    if len(sys.argv) < 2:
        sys.exit(1)

    filepath = sys.argv[1]
    with open(filepath, "r") as f:
        content = f.read()

    # Don't filter the root README (it uses USE_MDFILE_AS_MAINPAGE)
    rel = os.path.relpath(filepath)
    if rel == "README.md":
        sys.stdout.write(content)
        return

    # Extract the first # heading for the page title
    match = re.match(r"^#\s+(.+?)(?:\s*\{#.*\})?\s*$", content, re.MULTILINE)
    if not match:
        sys.stdout.write(content)
        return

    title = match.group(1).strip()

    # Build page ID from directory path
    # examples/text/hello_world/README.md -> examples_text_hello_world
    dirpath = os.path.dirname(rel)
    page_id = dirpath.replace(os.sep, "_").replace("/", "_").replace("-", "_")

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
