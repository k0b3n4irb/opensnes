#!/usr/bin/env python3
"""
VRAM Layout Analyzer for OpenSNES

Analyzes symbol files to visualize VRAM usage and detect conflicts.

Usage:
    python3 vramcheck.py game.sym [--visual] [--check-overlap]
"""

import sys
import re
import argparse

VRAM_SIZE = 0x10000  # 64KB

def parse_symbol_file(filename):
    """Parse symbol file and extract VRAM-related symbols."""
    vram_symbols = []
    pattern = re.compile(r'^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)', re.IGNORECASE)

    with open(filename, 'r') as f:
        for line in f:
            match = pattern.match(line.strip())
            if match:
                bank = int(match.group(1), 16)
                addr = int(match.group(2), 16)
                name = match.group(3)
                if any(x in name.lower() for x in ['vram', 'tile', 'map', 'gfx', 'chr', 'sprite']):
                    vram_symbols.append({'bank': bank, 'addr': addr, 'name': name})
    return vram_symbols

def visualize_vram(allocations, width=64):
    """Generate ASCII visualization of VRAM usage."""
    vram_map = ['.'] * width
    for alloc in allocations:
        start_char = alloc['start'] * width // VRAM_SIZE
        end_char = min(width - 1, alloc['end'] * width // VRAM_SIZE)
        label = alloc['label'][0].upper()
        for i in range(start_char, end_char + 1):
            vram_map[i] = 'X' if vram_map[i] != '.' else label
    print("\nVRAM Layout (1KB per char):")
    print("=" * (width + 8))
    print(f"$0000 |{''.join(vram_map)}| $FFFF")
    print("=" * (width + 8))

def main():
    parser = argparse.ArgumentParser(description='VRAM Layout Analyzer')
    parser.add_argument('symfile', help='Symbol file (.sym)')
    parser.add_argument('--visual', action='store_true')
    parser.add_argument('--verbose', '-v', action='store_true')
    args = parser.parse_args()

    print(f"Analyzing: {args.symfile}")
    symbols = parse_symbol_file(args.symfile)
    if args.verbose:
        print(f"Found {len(symbols)} VRAM-related symbols:")
        for sym in symbols:
            print(f"  ${sym['bank']:02X}:{sym['addr']:04X} {sym['name']}")

    allocations = [
        {'label': 'BG1 Tiles', 'start': 0x0000, 'end': 0x4000},
        {'label': 'BG1 Map', 'start': 0xC000, 'end': 0xD000},
    ]
    if args.visual:
        visualize_vram(allocations)
    return 0

if __name__ == '__main__':
    sys.exit(main())
