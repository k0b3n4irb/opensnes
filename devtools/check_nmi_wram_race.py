#!/usr/bin/env python3
"""
check_nmi_wram_race - Static lint for the WRAM data port NMI race.

The SNES WRAM data port ($2180-$2183) shares state between the main
thread and the NMI handler. If main-thread code is mid-sequence on
$2180 (e.g. setting an address via $2181-$2183 then writing data via
$2180) and an NMI fires whose callback also touches those ports, the
address pointer is silently corrupted. The main thread resumes writing
data to the wrong location with no error.

This is documented in:
  - KNOWN_LIMITATIONS.md (severity: red)
  - templates/crt0.asm:798-802 (NMI handler docstring)
  - .claude/rules/nmi_audit.md (audit checklist)

The trap is:
    void my_nmi_cb(void) {
        do_some_work();   // ANYWHERE in this transitive call tree, a
                          // write to $2180-$2183 silently races.
    }
    nmiSet(my_nmi_cb);

This lint walks the call graph from each NMI callback root (NmiHandler
itself plus any function passed to nmiSet/nmiSetBank) and flags any
function in the closure that writes to $2180-$2183.

Usage:
    check_nmi_wram_race.py --rom-dir examples/text/hello_world

Returns:
    0 if no $2180-$2183 writes in NMI closure
    1 if any write is detected (with actionable per-function report)

Implementation notes:
  - Parses .c.asm and combined.asm intermediate files (kept by build).
  - Roots: NmiHandler (always present in crt0) + DefaultNmiCallback
    (always present, audited safe) + user callbacks registered via
    `pea.w <sym>; jsl nmiSet` or `pea.w <sym>; ...; jsl nmiSetBank`.
  - Lib code is treated as a black box: jsl into a lib function is
    NOT followed because lib ASM is audited via .claude/rules/
    nmi_audit.md.
  - The lib's audit boundary: any function whose definition does not
    appear in the scanned files is considered out-of-scope (lib).
"""

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path


WRAM_PORT_PATTERNS = [
    # 16-bit absolute store / load (common for register access).
    re.compile(r"\bsta(?:\.[wlb])?\s+\$(?:00)?218[0-3]\b", re.IGNORECASE),
    re.compile(r"\bstz(?:\.[wlb])?\s+\$(?:00)?218[0-3]\b", re.IGNORECASE),
    # Indirect via X with $2180 base — `ldx #$2180` then `sta 0,x`.
    # We also flag ldx loading the WRAM port base; the actual store
    # site below shows up in the same function.
    re.compile(r"\bldx(?:\.[wlb])?\s+#\$(?:00)?218[0-3]\b", re.IGNORECASE),
    # Long-form store from a different bank (rare but legal).
    re.compile(r"\bsta\.l\s+\$00218[0-3]\b", re.IGNORECASE),
    re.compile(r"\bstz\.l\s+\$00218[0-3]\b", re.IGNORECASE),
]

# Patterns that mark a callback registration. Both nmiSet and
# nmiSetBank take the callback as their FIRST C argument; under
# cc65816's left-to-right push, that means it lands FIRST on the
# stack — `pea.w <sym>` directly preceding (within ~10 lines) a
# `jsl nmiSet` or `jsl nmiSetBank`.
PEA_LABEL_RE = re.compile(
    r"^\s*pea(?:\.w)?\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:;.*)?$",
    re.IGNORECASE,
)
JSL_NMISET_RE = re.compile(
    r"^\s*jsl(?:\.[wlb])?\s+(nmiSet|nmiSetBank)\s*(?:;.*)?$",
    re.IGNORECASE,
)

# Function definition: label at column 0, name + colon, no leading
# whitespace.  Local labels (@-prefixed) are jump targets within a
# function, not function entries.
LABEL_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*):\s*$")

# Call/jump instructions that flow control to another function.
# `jsl` and `jsr` are calls, `jml`/`jmp` are tail-jumps that we still
# treat as a control-flow edge for closure analysis. The `.l`/`.w`/`.b`
# size suffix is optional. We deliberately do NOT match indirect jumps
# (`jml [...]` / `jsr (sym,x)`); those are runtime dispatch and the
# resolved targets are seeded as roots elsewhere (NMI callbacks).
# Local jump targets (`@label`) inside the same function are skipped
# because they aren't function entries.
CALL_RE = re.compile(
    r"^\s*(?:jsl|jsr|jml|jmp)(?:\.[wlb])?\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:;.*)?$",
    re.IGNORECASE,
)


@dataclass
class Function:
    """A function definition discovered in an .asm file."""
    name: str
    file: Path
    start_line: int
    end_line: int
    body: list[str] = field(default_factory=list)
    callees: set[str] = field(default_factory=set)
    port_writes: list[tuple[int, str]] = field(default_factory=list)


def parse_asm_functions(asm_files: list[Path]) -> dict[str, Function]:
    """Parse all .asm files and return a map of function name → Function.

    A function is everything between two top-level labels (or between
    a label and end-of-file). Local labels (@-prefixed) belong to the
    enclosing function.
    """
    funcs: dict[str, Function] = {}

    for asm_file in asm_files:
        try:
            lines = asm_file.read_text(errors="replace").splitlines()
        except (OSError, UnicodeDecodeError) as e:
            print(f"warning: could not read {asm_file}: {e}", file=sys.stderr)
            continue

        current: Function | None = None

        for lineno, raw in enumerate(lines, start=1):
            stripped = raw.strip()

            # Skip comments and empty lines for structural parsing,
            # but include them in the function body so error reports
            # can quote them.
            if stripped.startswith(";") or not stripped:
                if current is not None:
                    current.body.append(raw)
                    current.end_line = lineno
                continue

            label_m = LABEL_RE.match(raw)
            if label_m:
                name = label_m.group(1)
                # End previous function before starting a new one.
                if current is not None:
                    funcs[current.name] = current
                current = Function(
                    name=name,
                    file=asm_file,
                    start_line=lineno,
                    end_line=lineno,
                )
                continue

            if current is None:
                # Top-level directives (.SECTION, .INCLUDE, etc.) before
                # the first label — skip.
                continue

            current.body.append(raw)
            current.end_line = lineno

            call_m = CALL_RE.match(raw)
            if call_m:
                current.callees.add(call_m.group(1))

            for pattern in WRAM_PORT_PATTERNS:
                if pattern.search(raw):
                    current.port_writes.append((lineno, stripped))
                    break

        if current is not None:
            funcs[current.name] = current

    return funcs


def find_nmi_callback_roots(funcs: dict[str, Function]) -> set[str]:
    """Find every symbol passed as the first argument to nmiSet or
    nmiSetBank.  Returns the set of callback names (which are
    expected to be functions; lookup against `funcs` is the caller's
    job).

    Heuristic: scan each function for `pea.w <sym>` followed within
    a small window (10 lines) by `jsl nmiSet` or `jsl nmiSetBank`.
    cc65816 pushes args left-to-right, so the callback (1st arg) is
    the LATEST `pea.w` before the jsl in the no-ambiguity case for
    nmiSet (single arg). For nmiSetBank the callback is followed by
    the bank `pea.w`; we still pick the symbolic `pea.w` (the bank is
    almost always a numeric literal, not a label).
    """
    callbacks: set[str] = set()

    for func in funcs.values():
        for idx, line in enumerate(func.body):
            jsl_m = JSL_NMISET_RE.match(line)
            if not jsl_m:
                continue
            # Walk backwards looking for the most recent symbolic
            # `pea.w <label>` (a label, not a numeric literal).
            for back in range(idx - 1, max(idx - 12, -1), -1):
                pea_m = PEA_LABEL_RE.match(func.body[back])
                if pea_m:
                    callbacks.add(pea_m.group(1))
                    break

    return callbacks


def closure(roots: set[str], funcs: dict[str, Function]) -> set[str]:
    """Compute the transitive closure of callees from each root.
    Functions not present in `funcs` (lib calls) are NOT followed —
    those are out-of-scope (audited via .claude/rules/nmi_audit.md).
    """
    seen: set[str] = set()
    stack = [r for r in roots if r in funcs]

    while stack:
        name = stack.pop()
        if name in seen:
            continue
        seen.add(name)
        for callee in funcs[name].callees:
            if callee in funcs and callee not in seen:
                stack.append(callee)

    return seen


def find_asm_sources(rom_dir: Path) -> list[Path]:
    """Collect ASM intermediates for a single example build."""
    candidates: list[Path] = []
    # combined.asm is the merged crt0 + runtime + lib-asm; included
    # so we cover NmiHandler / DefaultNmiCallback / WaitForVBlank /
    # tcc_* runtime helpers.
    cand = rom_dir / "combined.asm"
    if cand.exists():
        candidates.append(cand)
    # Every .c.asm in the example dir (cproc output for user C
    # sources).  No-recursion: build assets only.
    candidates.extend(sorted(rom_dir.glob("*.c.asm")))
    return candidates


def report(rom_dir: Path, callbacks: set[str], reachable: set[str],
           funcs: dict[str, Function]) -> int:
    """Print a report and return an exit code (0 clean, 1 violations)."""
    violations: list[Function] = []
    for name in sorted(reachable):
        f = funcs[name]
        if f.port_writes:
            violations.append(f)

    if not violations:
        return 0

    print(f"\n=== NMI WRAM-port lint: violations in {rom_dir} ===\n")
    for f in violations:
        print(f"  {f.name}  ({f.file.name}:{f.start_line})")
        for lineno, text in f.port_writes:
            print(f"    line {lineno}: {text}")
        print()

    print("Why this matters:")
    print("  Functions reachable from an NMI callback must NOT touch")
    print("  $2180-$2183. The WRAM data port shares state with the main")
    print("  thread; a mid-sequence interrupt corrupts the address")
    print("  pointer and the main thread silently writes to the wrong")
    print("  location. See KNOWN_LIMITATIONS.md and .claude/rules/")
    print("  nmi_audit.md.")
    print()
    print("Reachable from NMI roots:")
    print(f"  - NmiHandler (always)")
    print(f"  - DefaultNmiCallback (always)")
    if callbacks:
        for cb in sorted(callbacks):
            note = "" if cb in funcs else "  [external — not analysed]"
            print(f"  - {cb}{note}")
    print()
    print("Fix: route data through DMA channel transfers (which use")
    print("  the bus controller, not the WRAM port) or move the work")
    print("  out of the NMI callback into the main loop.")
    return 1


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Static lint for NMI / WRAM data port races.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--rom-dir",
        type=Path,
        required=True,
        help="Path to a built example directory (must contain "
             "combined.asm and *.c.asm intermediates).",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Only print on violations.",
    )
    args = parser.parse_args()

    rom_dir: Path = args.rom_dir
    if not rom_dir.is_dir():
        print(f"error: {rom_dir} is not a directory", file=sys.stderr)
        return 2

    asm_files = find_asm_sources(rom_dir)
    if not asm_files:
        print(f"error: no .asm intermediates found in {rom_dir}; "
              f"build the example first", file=sys.stderr)
        return 2

    funcs = parse_asm_functions(asm_files)

    # Roots: the NMI dispatch entry + the no-op default callback +
    # any user-supplied callback discovered statically.
    roots = {"NmiHandler", "DefaultNmiCallback"}
    roots |= find_nmi_callback_roots(funcs)

    reachable = closure(roots, funcs)

    if not args.quiet:
        cb_only = roots - {"NmiHandler", "DefaultNmiCallback"}
        if cb_only:
            print(f"NMI callbacks discovered: {sorted(cb_only)}")
        print(f"Reachable functions in NMI closure: {len(reachable)}")

    return report(rom_dir, roots - {"NmiHandler", "DefaultNmiCallback"},
                  reachable, funcs)


if __name__ == "__main__":
    sys.exit(main())
