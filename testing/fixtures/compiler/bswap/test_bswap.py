#!/usr/bin/env python3
"""The byte swap of a 16-bit word is one `xba` (mark_bswap, w65816 emitter).

Assumes `make` produced bswap.sfc. Checks, in order:
  1. the emitted text: sw_lr(), sw_rl(), sw_far() and sw_glob() swap with
     ONE `xba` and no mask; sw_s16() and sw_u32() keep their shifts (no
     `xba` without its mask, i.e. they are not taken for a swap);
  2. on luna, after the sweep of the 65 536 values of w: no mismatch
     against the swap built by an add, the branch on the swap's value
     right for zero and sign, and each form's checksum equal to the one
     of a model of the C computed here;
  3. the provoked fault: rebuilt with QBE_BSWAP_FAULT=1 (a `sar` taken
     for a `shr`), sw_s16()'s checksum must differ; and rebuilt with
     QBE_NO_BSWAP=1 every value must stay right and the text check must
     fail (the rule is what makes the `xba`). The ROM is rebuilt clean
     after (`--no-faults` skips this part).
"""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
sys.path.insert(0, str(REPO / "testing"))
from lib import find_luna  # noqa: E402

ROM = HERE / "bswap.sfc"
ASM = HERE / "main.c.asm"
STEPS = 60_000_000      # instructions; the sweep ends near 40 M
M16 = 0xFFFF
M32 = 0xFFFFFFFF


def le16(v: int) -> str:
    return (v & M16).to_bytes(2, "little").hex().upper()


def model() -> list[tuple[str, str]]:
    """The C of main.c on the host: C's integer rules at this target's
    widths (int 16 bits, long 32, `>>` of a negative int arithmetic)."""
    h = dict.fromkeys(("lr", "rl", "far", "glob", "mask", "s16", "lo", "hi"), 0)

    def mix(k: str, r: int) -> None:
        v = h[k]
        h[k] = ((((v << 5) | (v >> 11)) & M16 ^ r) + 0x9E37) & M16

    for w in range(0x10000):
        swap = ((w << 8) | (w >> 8)) & M16
        for k in ("lr", "rl", "far", "glob", "mask"):
            mix(k, swap)
        s = w - 0x10000 if w & 0x8000 else w
        mix("s16", (((w << 8) & M16) | ((s >> 8) & M16)) & M16)
        x = (((w ^ 0xA5C3) << 16) + w) & M32
        x = ((x << 8) | (x >> 8)) & M32
        mix("lo", x & M16)
        mix("hi", x >> 16)
    names = {"lr": "h_lr", "rl": "h_rl", "far": "h_far", "glob": "h_glob",
             "mask": "h_mask", "s16": "h_s16", "lo": "h_u32lo", "hi": "h_u32hi"}
    cases = [(names[k], le16(v)) for k, v in h.items()]
    cases += [(b, "0000") for b in ("bad_lr", "bad_rl", "bad_far", "bad_glob",
                                    "bad_mask", "bad_s16", "bad_u32", "bad_nz",
                                    "bad_neg")]
    return cases + [("done", "EFBE")]


def body(text: str, fn: str) -> str:
    head = f"\n{fn}:\n"
    return text.split(head, 1)[1].split(".ENDS", 1)[0] if head in text else ""


def text_checks() -> list[tuple[str, bool]]:
    text = ASM.read_text() if ASM.is_file() else ""
    res = []
    for fn in ("sw_lr", "sw_rl", "sw_far", "sw_glob"):
        b = body(text, fn)
        res.append((f"{fn}: one xba, no mask",
                    b.count("\txba\n") == 1 and "and.w #$FF00" not in b
                    and "and.w #$00FF" not in b))
    b = body(text, "sw_s16")
    res.append(("sw_s16: not taken for a swap (its mask stays)", "and.w #$FF00" in b))
    b = body(text, "sw_u32")
    res.append(("sw_u32: not taken for a swap (no xba)", b != "" and "xba" not in b))
    return res


def values(luna: str) -> dict[str, bool]:
    """One luna run for every assertion (the sweep takes about 40 M
    instructions): luna prints one PASS / FAIL line per --assert, in order."""
    cases = model()
    cmd = [luna, "state", "-n", str(STEPS), "--out", os.devnull]
    for name, want in cases:
        cmd += ["--assert", f"{name}={want}"]
    out = subprocess.run(cmd + [str(ROM)], capture_output=True, text=True,
                         timeout=600, stdin=subprocess.DEVNULL).stdout
    lines = [l.strip() for l in out.splitlines()
             if l.strip().startswith(("PASS", "FAIL"))]
    if len(lines) != len(cases):
        sys.exit(f"luna gave {len(lines)} verdicts for {len(cases)} assertions:\n{out}")
    return {name: l.startswith("PASS") for (name, _), l in zip(cases, lines)}


def build(env_extra: dict[str, str]) -> None:
    env = {k: v for k, v in os.environ.items() if "BSWAP" not in k}
    env.update(env_extra)
    for target in (["clean"], []):
        subprocess.run(["make", "-s", "-C", str(HERE)] + target, env=env, check=True,
                       stdout=subprocess.DEVNULL, stdin=subprocess.DEVNULL)


def report(title: str, checks) -> int:
    fails = 0
    for what, ok in checks:
        print(("PASS " if ok else "FAIL ") + title + what)
        fails += not ok
    return fails


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna()
    fails = report("", text_checks())
    fails += report("", values(luna).items())
    if "--no-faults" in sys.argv or fails:
        return 1 if fails else 0
    try:
        build({"QBE_BSWAP_FAULT": "1"})
        v = values(luna)
        ok = not v["h_s16"] and not v["bad_s16"]
        print(("PASS " if ok else "FAIL ")
              + "QBE_BSWAP_FAULT=1: sw_s16 must be wrong (a sar taken for a shr)")
        fails += not ok
        build({"QBE_NO_BSWAP": "1"})
        v = values(luna)
        ok = all(v.values())
        print(("PASS " if ok else "FAIL ") + "QBE_NO_BSWAP=1: every value right")
        fails += not ok
        ok = not all(o for _, o in text_checks()[:4])
        print(("PASS " if ok else "FAIL ") + "QBE_NO_BSWAP=1: the single xba is gone")
        fails += not ok
    finally:
        build({})
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
