#!/usr/bin/env python3
"""Generate 4bpp font data from existing 2bpp font in text.asm.

Each 2bpp tile (16 bytes) becomes a 4bpp tile (32 bytes):
  - First 16 bytes: original 2bpp data (BP0/BP1)
  - Last 16 bytes: all zeros (BP2/BP3)

This produces font tiles that use colors 0-3 of their palette slot,
identical appearance to 2bpp but compatible with 4bpp BG layers.

Usage: python3 tools/gen_font_4bpp.py > /dev/null
       (output is appended directly to lib/source/text.asm)
"""

import re
import sys


def main():
    asm_path = "lib/source/text.asm"

    with open(asm_path, "r") as f:
        content = f.read()

    # Extract tiles from the 2bpp font section
    in_font = False
    tiles = []
    current_comment = ""
    current_data = []

    for line in content.split("\n"):
        if "opensnes_font_2bpp:" in line:
            in_font = True
            continue
        if "opensnes_font_2bpp_end:" in line:
            if current_data:
                tiles.append((current_comment, current_data))
            break
        if not in_font:
            continue

        stripped = line.strip()
        if stripped.startswith("; ") and ":" in stripped and "'" in stripped:
            if current_data:
                tiles.append((current_comment, current_data))
            current_comment = stripped
            current_data = []
        elif stripped.startswith(".db"):
            current_data.append(stripped)

    # Generate 4bpp data lines
    lines = []
    lines.append("opensnes_font_4bpp:")
    lines.append(
        "    ; 96 characters, 32 bytes per tile = 3072 bytes total"
    )
    lines.append(
        "    ; Format: 16 bytes 2bpp (BP0/BP1) + 16 bytes zero (BP2/BP3)"
    )

    zero_line = ".db $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00"

    for comment, data_lines in tiles:
        lines.append(f"    {comment}")
        for db_line in data_lines:
            lines.append(f"    {db_line}")
        lines.append(f"    {zero_line}")

    lines.append("")
    lines.append("opensnes_font_4bpp_end:")

    # Print the generated assembly
    for line in lines:
        print(line)

    print(f"\n; Generated {len(tiles)} tiles, {len(tiles) * 32} bytes total", file=sys.stderr)


if __name__ == "__main__":
    main()
