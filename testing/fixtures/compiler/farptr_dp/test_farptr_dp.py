#!/usr/bin/env python3
"""A far pointer read in its direct-page slot and stepped on 16 bits (lot E1).

Assumes `make` produced farptr_dp.sfc. Checks, in order:
  1. the placement the cases rest on (.sym): `tail` at $7E:FFF0 (it ends at
     the last byte of the bank), `strad` at $7E:FFF8 (it crosses into $7F);
  2. the emitted text: copy2() reads both pointers in place (`[tcc__lf+N]`,
     no staging in tcc__r9) and steps them on 16 bits (no `adc.w #0`);
     cmp_tail() and ext_walk() keep the 32-bit step;
  3. every value against a model of the C (main.c says what each does);
  4. the provoked faults: rebuilt with QBE_FARPTR_FAULT=1 (any symbol taken
     for a root) ext_walk() must read the wrong bank, and with
     QBE_FARPTR_FAULT=2 (the use rule skipped) cmp_tail() must run to its
     bound; the ROM is rebuilt clean after (`--no-faults` skips this part).
"""
from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna, assert_mem  # noqa: E402

ROM = HERE / "farptr_dp.sfc"
ASM = HERE / "main.c.asm"
SYM = HERE / "farptr_dp.sym"
STEPS = 2_000_000


def le16(v: int) -> str:
    return (v & 0xFFFF).to_bytes(2, "little").hex().upper()


def model() -> list[tuple[str, str]]:
    src = [(i * 5 + 1) & 0xFF for i in range(64)]
    dst = [src[i] if i % 2 == 0 else src[i] ^ 0x55 for i in range(64)]
    r_copy = 0
    for v in dst:
        r_copy = (r_copy * 3 + v) & 0xFFFF
    tail = [i + 1 for i in range(16)]
    hi = [0x40 + i for i in range(8)]
    rom = [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7, 9, 3]
    r_back = 0
    for i in range(16):
        r_back = (r_back * 3 + tail[15 - i]) & 0xFFFF
    return [
        ("r_copy", le16(r_copy)),
        ("r_tail", le16(sum(tail))),
        ("r_cmp", le16(sum(tail))),
        ("n_cmp", le16(16)),
        ("r_ext", le16(sum(tail[8:]) + sum(hi))),
        ("r_rom", le16(sum(v * (i + 1) for i, v in enumerate(rom)))),
        ("r_near", le16(sum((i * 7) & 0xFF for i in range(16)))),
        ("r_back", le16(r_back)),
        ("done", "EFBE"),
    ]


def body(text: str, fn: str) -> str:
    head = f"\n{fn}:\n"
    return text.split(head, 1)[1].split(".ENDS", 1)[0] if head in text else ""


def placement() -> list[str]:
    sym = SYM.read_text() if SYM.is_file() else ""
    bad = []
    for name, addr in (("tail", "7e:fff0"), ("strad", "7e:fff8")):
        if not re.search(rf"^{addr} {name}$", sym, re.M):
            bad.append(f"{name} not at {addr}")
    return bad


def text_checks() -> list[tuple[str, bool]]:
    text = ASM.read_text() if ASM.is_file() else ""
    c2, ct, ce = body(text, "copy2"), body(text, "cmp_tail"), body(text, "ext_walk")
    return [
        ("copy2 reads its pointers in place", "[tcc__lf+" in c2 and "sta.b tcc__r9" not in c2),
        ("copy2 steps them on 16 bits", c2 != "" and "adc.w #0" not in c2),
        ("cmp_tail keeps the 32-bit step", "adc.w #0" in ct),
        ("ext_walk keeps the 32-bit step", "adc.w #0" in ce),
    ]


def values(luna: str) -> dict[str, bool]:
    res = {}
    for name, want in model():
        ok, _ = assert_mem(luna, ROM, STEPS, [(name, want)])
        res[name] = ok
    return res


def build(env_extra: dict[str, str]) -> None:
    env = {k: v for k, v in os.environ.items() if not k.startswith("QBE_FARPTR")}
    env.update(env_extra)
    for target in (["clean"], []):
        subprocess.run(["make", "-s", "-C", str(HERE)] + target, env=env, check=True,
                       stdout=subprocess.DEVNULL)


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    fails = 0
    for b in placement():
        print("FAIL placement: " + b)
        fails += 1
    for what, ok in text_checks():
        print(("PASS " if ok else "FAIL ") + what)
        fails += not ok
    luna = find_luna()
    for name, ok in values(luna).items():
        print(("PASS " if ok else "FAIL ") + name)
        fails += not ok
    if "--no-faults" in sys.argv or fails:
        return 1 if fails else 0
    # the provoked faults: each must turn its own case red
    try:
        for fault, case in (("1", "r_ext"), ("2", "n_cmp")):
            build({"QBE_FARPTR_FAULT": fault})
            res = values(luna)
            seen = not res[case]
            print(("PASS " if seen else "FAIL ") + f"QBE_FARPTR_FAULT={fault} turns {case} red")
            fails += not seen
    finally:
        build({})                       # the normal ROM back
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
