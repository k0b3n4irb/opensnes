#!/usr/bin/env python3
"""Every compiled tool of bin/ says which SDK release and which commit built it.

`opensnes --version` used to print `opensnes 1.0.0`, the tool's own number,
with nothing to tie a report to a build (audit of 2026-11-08, A3). It now
prints `opensnes 1.0.0 (OpenSNES 0.55.0, commit 0621c14c)`; the commit has
`+dirty` appended when the tree had uncommitted changes, or `unknown` outside
a git checkout. This check runs every `bin/opensnes*` with `--version` and
refuses a line that lacks the SDK version of `lib/include/snes.h` (a stale
binary, a tool built outside tools/tool.mk) or a well-formed commit.

Usage:  python3 devtools/check_tool_versions.py [ROOT]     (exit 1 on a fault)
"""
import re
import subprocess
import sys
from pathlib import Path

COMMIT = r"(?:[0-9a-f]{8}(?:\+dirty)?|unknown)"


def sdk_version(root):
    text = (Path(root) / "lib" / "include" / "snes.h").read_text()
    m = re.search(r'#define\s+OPENSNES_VERSION_STRING\s+"([^"]+)"', text)
    if not m:
        raise SystemExit("check_tool_versions: no OPENSNES_VERSION_STRING in lib/include/snes.h")
    return m.group(1)


def fault_of(line, name, sdk):
    """Return a message when `line` (the output of `<name> --version`) is wrong, else None."""
    pattern = r"^" + re.escape(name) + r" \S+ \(OpenSNES " + re.escape(sdk) + r", commit " + COMMIT + r"\)$"
    if re.match(pattern, line.strip()):
        return None
    return f"{name}: '{line.strip()}' is not '{name} <version> (OpenSNES {sdk}, commit <8 hex>[+dirty]|unknown)'"


def main(argv):
    root = Path(argv[1]) if len(argv) > 1 else Path(".")
    sdk = sdk_version(root)
    tools = sorted(p for p in (root / "bin").glob("opensnes*") if p.is_file() and p.suffix in ("", ".exe"))
    if not tools:
        print("check_tool_versions: no bin/opensnes* tool built (run make tools)", file=sys.stderr)
        return 2
    bad = 0
    for tool in tools:
        out = subprocess.run([str(tool), "--version"], capture_output=True, text=True).stdout.splitlines()
        msg = fault_of(out[0] if out else "", tool.stem, sdk)
        if msg:
            print(msg)
            bad += 1
    if bad:
        print("(a stale binary is rebuilt by `make tools`)")
        return 1
    print(f"OK: {len(tools)} tools report OpenSNES {sdk} and their build commit")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
