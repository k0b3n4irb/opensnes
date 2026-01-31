#!/usr/bin/env python3
"""
Memory Profiler for OpenSNES

Analyzes symbol files to report RAM/ROM usage statistics.
"""

import sys
import re
import argparse
from collections import defaultdict

def parse_symbol_file(filename):
    symbols = []
    pattern = re.compile(r'^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)', re.IGNORECASE)
    with open(filename, 'r') as f:
        for line in f:
            match = pattern.match(line.strip())
            if match:
                bank = int(match.group(1), 16)
                addr = int(match.group(2), 16)
                name = match.group(3)
                symbols.append({
                    'bank': bank, 'addr': addr, 'name': name,
                    'is_ram': bank == 0 and addr < 0x2000 or bank == 0x7E,
                    'is_rom': bank == 0 and addr >= 0x8000,
                })
    return symbols

def categorize_symbol(name):
    name_lower = name.lower()
    if any(x in name_lower for x in ['oam', 'sprite']): return 'Sprites'
    if any(x in name_lower for x in ['tile', 'gfx', 'chr', 'vram']): return 'Graphics'
    if any(x in name_lower for x in ['pad', 'joy', 'input', 'key']): return 'Input'
    if any(x in name_lower for x in ['audio', 'sound', 'music']): return 'Audio'
    if name.startswith('_') or name.startswith('.'): return 'Runtime'
    return 'User Code'

def main():
    parser = argparse.ArgumentParser(description='Memory Profiler')
    parser.add_argument('symfile', help='Symbol file (.sym)')
    parser.add_argument('--verbose', '-v', action='store_true')
    args = parser.parse_args()

    symbols = parse_symbol_file(args.symfile)
    ram_usage = defaultdict(list)
    rom_usage = defaultdict(list)

    for sym in symbols:
        cat = categorize_symbol(sym['name'])
        if sym['is_ram']: ram_usage[cat].append(sym)
        elif sym['is_rom']: rom_usage[cat].append(sym)

    ram_symbols = [s for s in symbols if s['is_ram']]
    rom_symbols = [s for s in symbols if s['is_rom']]

    print("=" * 50)
    print("OpenSNES Memory Profile")
    print("=" * 50)
    print(f"Total: {len(symbols)} | RAM: {len(ram_symbols)} | ROM: {len(rom_symbols)}")

    print("\n--- RAM by Category ---")
    for cat, syms in sorted(ram_usage.items(), key=lambda x: -len(x[1])):
        print(f"  {cat}: {len(syms)}")

    print("\n--- ROM by Category ---")
    for cat, syms in sorted(rom_usage.items(), key=lambda x: -len(x[1])):
        print(f"  {cat}: {len(syms)}")

    # Check for low RAM usage
    danger = [s for s in ram_symbols if s['addr'] < 0x0200]
    if danger:
        print(f"\n[WARN] {len(danger)} symbols in low RAM ($0000-$01FF)")
    else:
        print("\n[OK] No low RAM conflicts")

    return 0

if __name__ == '__main__':
    sys.exit(main())
