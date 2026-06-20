"""Shared helpers for luna functional probes (WS-R 2.3).

Headless equivalent of the snes9x-MCP `test/functional/*.test.mjs` probes:
drive luna with a scripted joypad, run, and assert on WRAM read back via
`luna state --peek`. Symbol names resolve through the example's `.sym`
(the headless equivalent of devtools/snesdbg's `sym.addr("name")`).

luna `--input` is frame-latched (`frame:hex` held until the next checkpoint);
`-n` is *instructions*, so we end runs at a generous -n that comfortably passes
the last input checkpoint (reads happen at -n). Probes assert directionally or
on a held value, so exact frame landing is not required.
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parent.parent.parent
sys.path.insert(0, str(HERE.parent))
from luna_runner import find_luna  # noqa: E402  (reuse the binary resolver)

# JOY1 button masks (matches luna --input / SNES joypad register).
B, Y, SELECT, START = 0x8000, 0x4000, 0x2000, 0x1000
UP, DOWN, LEFT, RIGHT = 0x0800, 0x0400, 0x0200, 0x0100
A, X, L, R = 0x0080, 0x0040, 0x0020, 0x0010

_SYM_RE = re.compile(r"^([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})\s+(\S+)")


def load_symbols(rom: Path) -> dict[str, tuple[int, int]]:
    """Parse `<rom>.sym` → {name: (bank, offset)} (wlalink symbol map)."""
    sym = rom.with_suffix(".sym")
    out: dict[str, tuple[int, int]] = {}
    for line in sym.read_text().splitlines():
        m = _SYM_RE.match(line)
        if m:
            out.setdefault(m.group(3), (int(m.group(1), 16), int(m.group(2), 16)))
    return out


# Each peek dump line is "$AABBCC  XX XX …" (≤16 bytes). A multi-byte peek
# spans several such lines, so collect bytes from ALL of them.
_PEEK_LINE_RE = re.compile(r"\$[0-9A-Fa-f]{6}\s+((?:[0-9A-Fa-f]{2}\s*)+)")


def peek(luna: str, rom: Path, steps: int, bank: int, offset: int,
         count: int, input_script: str | None = None) -> list[int]:
    """Run `luna state -n steps [--input …] --peek bank:off:count`; return bytes."""
    cmd = [luna, "state", "-n", str(steps), "--out", "/dev/null",
           "--peek", f"{bank:02X}:{offset:04X}:{count}", str(rom)]
    if input_script:
        cmd[6:6] = ["--input", input_script]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
    out: list[int] = []
    for m in _PEEK_LINE_RE.finditer(proc.stderr):
        out += [int(b, 16) for b in m.group(1).split()]
    if len(out) < count:
        raise RuntimeError(f"peek parse failed for {rom.name} "
                           f"(got {len(out)}/{count}): {proc.stderr.strip()[:200]}")
    return out[:count]


def dump_vram(luna: str, rom: Path, steps: int) -> bytes:
    """Run `luna state --dump-vram` and return the raw 64 KB VRAM bytes."""
    import tempfile
    with tempfile.NamedTemporaryFile(suffix=".vram") as tf:
        proc = subprocess.run(
            [luna, "state", "-n", str(steps), "--out", "/dev/null",
             "--dump-vram", tf.name, str(rom)],
            capture_output=True, text=True, timeout=300)
        if proc.returncode != 0:
            raise RuntimeError(f"dump-vram failed: {proc.stderr.strip()[:200]}")
        return Path(tf.name).read_bytes()


def cgram_words(luna: str, rom: Path, steps: int) -> list[int]:
    """Return CGRAM as 256 15-bit colour words (from `luna state` JSON ppu.cgram)."""
    cmd = [luna, "state", "-n", str(steps), "--out", "-", str(rom)]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
    import json as _json
    return _json.loads(proc.stdout)["ppu"]["cgram"]


def peek_byte(luna: str, rom: Path, steps: int, bank: int, offset: int,
              input_script: str | None = None) -> int:
    return peek(luna, rom, steps, bank, offset, 1, input_script)[0]


def peek_word(luna: str, rom: Path, steps: int, bank: int, offset: int,
              input_script: str | None = None) -> int:
    lo, hi = peek(luna, rom, steps, bank, offset, 2, input_script)
    return lo | (hi << 8)


def peek_sword(luna: str, rom: Path, steps: int, bank: int, offset: int,
               input_script: str | None = None) -> int:
    """Signed 16-bit read (two's complement) — for s16 world coords."""
    v = peek_word(luna, rom, steps, bank, offset, input_script)
    return v - 0x10000 if v >= 0x8000 else v


def sym_of(rom: Path, name: str) -> tuple[int, int]:
    return load_symbols(rom)[name]


_SIZE_RE = re.compile(r"^([0-9A-Fa-f]{8})\s+_sizeof_(\S+)")


def sym_size(rom: Path, name: str) -> int:
    """`_sizeof_<name>` from the .sym (wlalink emits one per data symbol)."""
    for line in rom.with_suffix(".sym").read_text().splitlines():
        m = _SIZE_RE.match(line)
        if m and m.group(2) == name:
            return int(m.group(1), 16)
    raise KeyError(f"_sizeof_{name} not in {rom.name}.sym")


def rom_path(rel: str) -> Path:
    return REPO_ROOT / "examples" / rel
