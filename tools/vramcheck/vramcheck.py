#!/usr/bin/env python3
"""
vramcheck - VRAM Layout Analyzer for SNES Development

Analyzes assembly files to detect potentially dangerous VRAM configurations:
- Overlapping tilemap regions
- Split DMA patterns to overlapping regions
- VBlank between related DMA transfers

This tool was created after debugging a "pink border" bug in breakout that
was caused by splitting DMA transfers to overlapping VRAM regions across
multiple VBlanks.

Usage:
    vramcheck game.asm                    # Check single file
    vramcheck --dir examples/game/1_breakout  # Check directory
    vramcheck --sym game.sym --asm game.asm   # Combined analysis
"""

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    RESET = '\033[0m'

    @classmethod
    def disable(cls):
        cls.RED = cls.GREEN = cls.YELLOW = cls.BLUE = cls.CYAN = cls.BOLD = cls.RESET = ''


@dataclass
class VRAMRegion:
    """Represents a VRAM region used by a background or sprite layer"""
    name: str           # e.g., "BG1 tilemap", "BG3 tilemap"
    start: int          # Start address in VRAM (word address)
    end: int            # End address in VRAM (word address)
    source_line: int    # Line number in source file
    source_file: str    # Source file path

    @property
    def size(self) -> int:
        return self.end - self.start

    def overlaps(self, other: 'VRAMRegion') -> bool:
        """Check if this region overlaps with another"""
        return not (self.end <= other.start or other.end <= self.start)

    def overlap_range(self, other: 'VRAMRegion') -> Optional[tuple]:
        """Return the overlapping range, or None if no overlap"""
        if not self.overlaps(other):
            return None
        return (max(self.start, other.start), min(self.end, other.end))


@dataclass
class DMATransfer:
    """Represents a DMA transfer found in code"""
    dest_addr: int      # Destination VRAM address
    size: int           # Transfer size in bytes
    source_line: int    # Line number
    source_file: str    # Source file
    function: str       # Function name (e.g., "dmaCopyVram")
    in_function: str = ""  # Name of containing C function


@dataclass
class VBlankCall:
    """Represents a WaitForVBlank call"""
    source_line: int
    source_file: str
    in_function: str = ""  # Name of containing C function


@dataclass
class DMASequence:
    """A sequence of DMA transfers between VBlanks"""
    vblank_line: int
    transfers: list     # List of DMATransfer


class VRAMAnalyzer:
    """Analyzes VRAM usage patterns in SNES code"""

    # Common VRAM sizes for tilemap configurations
    TILEMAP_SIZES = {
        'SC_32x32': 0x800,   # 2KB = 32x32 tiles x 2 bytes
        'SC_64x32': 0x1000,  # 4KB
        'SC_32x64': 0x1000,  # 4KB
        'SC_64x64': 0x2000,  # 8KB
    }

    def __init__(self):
        self.vram_regions: list[VRAMRegion] = []
        self.dma_transfers: list[DMATransfer] = []
        self.vblank_calls: list[VBlankCall] = []
        self.warnings: list[str] = []
        self.errors: list[str] = []

    def parse_c_file(self, path: Path) -> None:
        """Parse a C file for VRAM configuration calls"""
        with open(path, 'r') as f:
            content = f.read()
            lines = content.split('\n')

        current_function = ""
        brace_depth = 0

        for i, line in enumerate(lines, 1):
            # Track function context by looking for function definitions
            # Simple heuristic: look for "type name(...) {" pattern
            func_match = re.search(r'^\s*(?:static\s+)?(?:void|int|u8|u16|s16|s8)\s+(\w+)\s*\([^)]*\)\s*\{?\s*$', line)
            if func_match:
                current_function = func_match.group(1)
                # Reset brace counting when entering a function
                brace_depth = line.count('{') - line.count('}')
            else:
                # Track brace depth to detect function exits
                brace_depth += line.count('{') - line.count('}')
                if brace_depth <= 0 and current_function:
                    current_function = ""
                    brace_depth = 0

            # Look for bgSetMapPtr calls: bgSetMapPtr(bg, addr, size)
            match = re.search(r'bgSetMapPtr\s*\(\s*(\d+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*(\w+)\s*\)', line)
            if match:
                bg_num = int(match.group(1))
                addr = int(match.group(2), 0)
                size_name = match.group(3)
                size = self.TILEMAP_SIZES.get(size_name, 0x800)  # Default to 32x32

                self.vram_regions.append(VRAMRegion(
                    name=f"BG{bg_num + 1} tilemap",
                    start=addr,
                    end=addr + size,
                    source_line=i,
                    source_file=str(path)
                ))

            # Look for dmaCopyVram calls: dmaCopyVram(src, dest, size)
            match = re.search(r'dmaCopyVram\s*\([^,]+,\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\)', line)
            if match:
                dest = int(match.group(1), 0)
                size = int(match.group(2), 0)

                self.dma_transfers.append(DMATransfer(
                    dest_addr=dest,
                    size=size,
                    source_line=i,
                    source_file=str(path),
                    function="dmaCopyVram",
                    in_function=current_function
                ))

            # Look for WaitForVBlank calls
            if re.search(r'WaitForVBlank\s*\(\s*\)', line):
                self.vblank_calls.append(VBlankCall(
                    source_line=i,
                    source_file=str(path),
                    in_function=current_function
                ))

    def parse_asm_file(self, path: Path) -> None:
        """Parse an assembly file for VRAM operations"""
        with open(path, 'r') as f:
            lines = f.readlines()

        for i, line in enumerate(lines, 1):
            # Look for VRAM address stores: sta.l $2116 (VMADDL)
            # This is where VRAM destination is set
            match = re.search(r'lda[.\w]*\s+#?\$?([0-9a-fA-F]{4})', line)
            if match and i + 1 < len(lines):
                next_line = lines[i]
                if '$2116' in next_line or 'VMADDL' in next_line.upper():
                    addr = int(match.group(1), 16)
                    # Try to find the size from nearby instructions
                    # This is a heuristic

            # Look for JSL dmaCopyVram patterns
            if 'jsl dmaCopyVram' in line.lower() or 'jsl dmaCopyCGram' in line.lower():
                self.dma_transfers.append(DMATransfer(
                    dest_addr=0,  # Would need more context to determine
                    size=0,
                    source_line=i,
                    source_file=str(path),
                    function="dmaCopyVram" if "vram" in line.lower() else "dmaCopyCGram"
                ))

            # Look for WaitForVBlank
            if 'jsl WaitForVBlank' in line or 'jsr WaitForVBlank' in line:
                self.vblank_calls.append(VBlankCall(
                    source_line=i,
                    source_file=str(path)
                ))

    def check_vram_overlaps(self) -> list[tuple]:
        """Check for overlapping VRAM regions"""
        overlaps = []

        for i, r1 in enumerate(self.vram_regions):
            for r2 in self.vram_regions[i + 1:]:
                if r1.overlaps(r2):
                    overlap_range = r1.overlap_range(r2)
                    overlaps.append((r1, r2, overlap_range))

        return overlaps

    def check_split_dma_patterns(self) -> list[dict]:
        """
        Check for dangerous patterns where DMA transfers to overlapping
        VRAM regions are split across VBlanks.

        This was the root cause of the breakout "pink border" bug.
        """
        issues = []

        # Group DMA transfers by their position relative to VBlanks
        # Only check within the same function to reduce false positives

        # Sort all events by line number (within same file)
        events = []
        for dma in self.dma_transfers:
            events.append(('dma', dma.source_line, dma))
        for vb in self.vblank_calls:
            events.append(('vblank', vb.source_line, vb))

        events.sort(key=lambda x: x[1])

        # Look for patterns: DMA, VBlank, DMA (potential split)
        # Only flag if all three are in the same function
        for i in range(len(events) - 2):
            if (events[i][0] == 'dma' and
                events[i + 1][0] == 'vblank' and
                events[i + 2][0] == 'dma'):

                dma1 = events[i][2]
                vblank = events[i + 1][2]
                dma2 = events[i + 2][2]

                # Only flag if all in the same function (reduces false positives)
                if not (dma1.in_function and dma1.in_function == vblank.in_function == dma2.in_function):
                    continue

                # Check if these DMAs might target overlapping VRAM
                # Additional check: do the DMAs actually target overlapping regions?
                dma1_end = dma1.dest_addr + dma1.size
                dma2_end = dma2.dest_addr + dma2.size
                regions_overlap = not (dma1_end <= dma2.dest_addr or dma2_end <= dma1.dest_addr)

                if dma1.function == dma2.function == 'dmaCopyVram':
                    issues.append({
                        'type': 'split_dma',
                        'dma1_line': dma1.source_line,
                        'vblank_line': vblank.source_line,
                        'dma2_line': dma2.source_line,
                        'file': dma1.source_file,
                        'function': dma1.in_function,
                        'regions_overlap': regions_overlap,
                        'message': f"Potential split DMA in {dma1.in_function}(): VRAM transfers at lines {dma1.source_line} and {dma2.source_line} separated by VBlank at line {vblank.source_line}"
                    })

        return issues

    def analyze(self) -> tuple[list, list]:
        """Run all analysis checks and return (warnings, errors)"""
        warnings = []
        errors = []

        # Check VRAM overlaps
        overlaps = self.check_vram_overlaps()
        for r1, r2, overlap_range in overlaps:
            warnings.append(
                f"VRAM overlap: {r1.name} (0x{r1.start:04X}-0x{r1.end:04X}) overlaps with "
                f"{r2.name} (0x{r2.start:04X}-0x{r2.end:04X}) at 0x{overlap_range[0]:04X}-0x{overlap_range[1]:04X}\n"
                f"  {r1.source_file}:{r1.source_line} and {r2.source_file}:{r2.source_line}\n"
                f"  NOTE: Overlapping regions MUST be updated in the SAME VBlank!"
            )

        # Check for split DMA patterns
        split_issues = self.check_split_dma_patterns()
        for issue in split_issues:
            # If the DMAs themselves target overlapping regions, this is an error
            if issue.get('regions_overlap'):
                errors.append(
                    f"DANGEROUS: {issue['message']}\n"
                    f"  File: {issue['file']}\n"
                    f"  The DMA regions overlap! This WILL cause visual corruption!\n"
                    f"  FIX: Move all related DMA transfers before the WaitForVBlank."
                )
            # If we have overlapping VRAM tilemap regions configured, warn
            elif overlaps:
                warnings.append(
                    f"REVIEW: {issue['message']}\n"
                    f"  File: {issue['file']}\n"
                    f"  Project has overlapping VRAM regions - verify these DMAs don't conflict."
                )
            # Otherwise just note it
            else:
                warnings.append(
                    f"INFO: {issue['message']}\n"
                    f"  File: {issue['file']}\n"
                    f"  If these target overlapping VRAM regions, corruption may occur."
                )

        return warnings, errors


def analyze_directory(path: Path, analyzer: VRAMAnalyzer) -> None:
    """Analyze all relevant files in a directory"""
    # Look for C files
    for c_file in path.glob('*.c'):
        analyzer.parse_c_file(c_file)

    # Look for ASM files (but not generated ones)
    for asm_file in path.glob('*.asm'):
        if not asm_file.name.endswith('.c.asm'):  # Skip compiler output
            analyzer.parse_asm_file(asm_file)


def main():
    parser = argparse.ArgumentParser(
        description='VRAM Layout Analyzer for SNES Development',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
This tool detects VRAM configuration issues that can cause visual corruption.

The most common issue is overlapping tilemap regions that are updated
with split DMA transfers across VBlanks. When VRAM regions overlap,
ALL related DMAs must complete in the SAME VBlank.

Example of the bug this detects (from breakout):
  BG1 tilemap: 0x0000-0x07FF
  BG3 tilemap: 0x0400-0x0BFF  <- OVERLAPS at 0x0400-0x07FF!

  BAD:
    WaitForVBlank();
    dmaCopyVram(blockmap, 0x0000, 0x800);
    WaitForVBlank();  // Frame rendered with corrupt BG3!
    dmaCopyVram(backmap, 0x0400, 0x800);

  GOOD:
    WaitForVBlank();
    dmaCopyVram(blockmap, 0x0000, 0x800);
    dmaCopyVram(backmap, 0x0400, 0x800);  // Same VBlank!
"""
    )

    parser.add_argument('path', type=Path, nargs='?',
                        help='File or directory to analyze')
    parser.add_argument('--dir', type=Path,
                        help='Directory to analyze')
    parser.add_argument('--no-color', action='store_true',
                        help='Disable colored output')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Show detailed analysis')

    args = parser.parse_args()

    if args.no_color or not sys.stdout.isatty():
        Colors.disable()

    path = args.dir or args.path
    if not path:
        parser.print_help()
        return 1

    if not path.exists():
        print(f"{Colors.RED}ERROR: Path not found: {path}{Colors.RESET}")
        return 1

    analyzer = VRAMAnalyzer()

    if path.is_dir():
        analyze_directory(path, analyzer)
    else:
        if path.suffix == '.c':
            analyzer.parse_c_file(path)
        elif path.suffix == '.asm':
            analyzer.parse_asm_file(path)
        else:
            print(f"{Colors.YELLOW}Unknown file type: {path}{Colors.RESET}")

    # Run analysis
    warnings, errors = analyzer.analyze()

    # Print results
    print(f"{Colors.BOLD}=== VRAM Layout Analysis ==={Colors.RESET}\n")

    if args.verbose:
        print(f"Found {len(analyzer.vram_regions)} VRAM regions:")
        for r in analyzer.vram_regions:
            print(f"  {r.name}: 0x{r.start:04X}-0x{r.end:04X} ({r.size} bytes)")
        print()

        print(f"Found {len(analyzer.dma_transfers)} DMA transfers")
        print(f"Found {len(analyzer.vblank_calls)} VBlank calls")
        print()

    if errors:
        print(f"{Colors.RED}{Colors.BOLD}ERRORS ({len(errors)}):{Colors.RESET}")
        for err in errors:
            print(f"\n{Colors.RED}{err}{Colors.RESET}")
        print()

    if warnings:
        print(f"{Colors.YELLOW}{Colors.BOLD}WARNINGS ({len(warnings)}):{Colors.RESET}")
        for warn in warnings:
            print(f"\n{Colors.YELLOW}{warn}{Colors.RESET}")
        print()

    if not errors and not warnings:
        print(f"{Colors.GREEN}No VRAM issues detected.{Colors.RESET}")
        return 0

    if errors:
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
