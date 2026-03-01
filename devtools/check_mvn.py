#!/usr/bin/env python3
"""
check_mvn - MVN/MVP Operand Linter for SNES Assembly

Scans .asm source files for raw MVN/MVP instructions that are NOT inside
known safe macros. Flags suspicious patterns that may indicate swapped
bank operands.

Usage:
    python3 devtools/check_mvn.py lib/source/*.asm
    python3 devtools/check_mvn.py --strict lib/source/*.asm

Exit codes:
    0 = No issues (or only warnings in non-strict mode)
    1 = Errors found (raw MVN outside macros in strict mode)
"""

import argparse
import re
import sys
from pathlib import Path

# ANSI colors
class Colors:
    RED = '\033[91m'
    YELLOW = '\033[93m'
    GREEN = '\033[92m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

# Known safe macro names that contain MVN/MVP instructions.
# Lines inside these macro definitions are not flagged.
SAFE_MACROS = {
    'BLOCK_COPY_7E_TO_00',
    'BLOCK_COPY_00_TO_7E',
    'BLOCK_COPY',
    'SYNC_TO_WORKSPACE',
    'SYNC_FROM_WORKSPACE',
}

# Pattern to match MVN/MVP instructions (case-insensitive)
MVN_PATTERN = re.compile(
    r'^\s+(?:mvn|mvp)\s+\$?([0-9A-Fa-f]+)\s*,\s*\$?([0-9A-Fa-f]+)',
    re.IGNORECASE
)

# Pattern to detect macro definition start/end
MACRO_START = re.compile(r'^\s*\.MACRO\s+(\w+)', re.IGNORECASE)
MACRO_END = re.compile(r'^\s*\.ENDM', re.IGNORECASE)


def scan_file(filepath: Path, verbose: bool = False):
    """Scan a single .asm file for MVN/MVP usage.

    Returns a list of findings: (filepath, line_num, line_text, in_macro, src_bank, dst_bank)
    """
    findings = []
    current_macro = None
    in_safe_macro = False

    try:
        lines = filepath.read_text(encoding='utf-8', errors='replace').splitlines()
    except OSError as e:
        print(f"{Colors.RED}ERROR: Cannot read {filepath}: {e}{Colors.RESET}",
              file=sys.stderr)
        return findings

    for line_num, line in enumerate(lines, 1):
        # Track macro context
        macro_match = MACRO_START.match(line)
        if macro_match:
            current_macro = macro_match.group(1)
            in_safe_macro = current_macro in SAFE_MACROS
            continue

        if MACRO_END.match(line):
            current_macro = None
            in_safe_macro = False
            continue

        # Skip comment-only lines
        stripped = line.lstrip()
        if stripped.startswith(';') or not stripped:
            continue

        # Check for MVN/MVP
        mvn_match = MVN_PATTERN.match(line)
        if mvn_match:
            src_bank = int(mvn_match.group(1), 16)
            dst_bank = int(mvn_match.group(2), 16)
            findings.append({
                'file': filepath,
                'line': line_num,
                'text': line.rstrip(),
                'macro': current_macro,
                'safe': in_safe_macro,
                'src_bank': src_bank,
                'dst_bank': dst_bank,
            })

    return findings


def check_suspicious(finding):
    """Check if a finding looks suspicious.

    Returns a list of warning strings, or empty list if OK.
    Items inside safe macros are never flagged (already vetted).
    """
    if finding['safe']:
        return []

    warnings = []

    # Flag raw MVN outside known macros
    if finding['macro'] is None:
        warnings.append("raw MVN/MVP outside any macro")

    # Flag reads from bank $00 I/O space (addresses handled by hardware, not RAM)
    # This is a heuristic — MVN reads from src_bank sequentially.
    # If src_bank is $00 and the code doesn't ensure addresses stay < $2000,
    # it may read I/O registers instead of RAM.
    if finding['src_bank'] == 0x00:
        warnings.append("source is Bank $00 — ensure addresses < $2000 (WRAM mirror)")

    return warnings


def main():
    parser = argparse.ArgumentParser(
        description='Lint MVN/MVP instructions in SNES assembly files'
    )
    parser.add_argument('files', nargs='+', type=Path,
                        help='.asm files to scan')
    parser.add_argument('--strict', action='store_true',
                        help='Exit with error on any raw MVN outside macros')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Show all MVN/MVP occurrences, not just warnings')
    args = parser.parse_args()

    all_findings = []
    for filepath in args.files:
        if not filepath.exists():
            print(f"{Colors.YELLOW}SKIP: {filepath} not found{Colors.RESET}",
                  file=sys.stderr)
            continue
        findings = scan_file(filepath, args.verbose)
        all_findings.extend(findings)

    # Analyze findings
    warnings_count = 0
    total_mvn = len(all_findings)

    if args.verbose:
        print(f"\n{Colors.BOLD}MVN/MVP Usage Report{Colors.RESET}")
        print(f"{'=' * 60}")

    for f in all_findings:
        warns = check_suspicious(f)
        macro_info = f"in .MACRO {f['macro']}" if f['macro'] else "NOT in macro"
        safe_tag = f"{Colors.GREEN}[SAFE]{Colors.RESET}" if f['safe'] else ""

        if warns:
            warnings_count += 1
            print(f"{Colors.YELLOW}WARNING{Colors.RESET}: {f['file']}:{f['line']}: "
                  f"{', '.join(warns)}")
            print(f"  {f['text']}")
            print(f"  ({macro_info}) {safe_tag}")
            print()
        elif args.verbose:
            print(f"{Colors.GREEN}OK{Colors.RESET}: {f['file']}:{f['line']}: "
                  f"mvn ${f['src_bank']:02X}, ${f['dst_bank']:02X} "
                  f"({macro_info}) {safe_tag}")

    # Summary
    print(f"\n{Colors.BOLD}Summary{Colors.RESET}: "
          f"{total_mvn} MVN/MVP instructions found, "
          f"{warnings_count} warnings")

    if warnings_count == 0:
        print(f"{Colors.GREEN}All MVN/MVP usage looks safe.{Colors.RESET}")
    else:
        print(f"{Colors.YELLOW}{warnings_count} potential issues found.{Colors.RESET}")

    if args.strict and warnings_count > 0:
        sys.exit(1)

    sys.exit(0)


if __name__ == '__main__':
    main()
