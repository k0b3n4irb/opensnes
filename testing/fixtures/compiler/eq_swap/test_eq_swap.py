#!/usr/bin/env python3
"""An equality compares its operands either way round (eq_swap, w65816 emitter).

Assumes `make` produced eq_swap.sfc. Checks, in order:
  1. the emitted text: eq_loop(), ne_loop() and eq_val() compare g, left in
     A by its own load and mask, with the other operand (`lda.w g / and.w
     #$00FF / cmp`, no store of g); eq_reuse() keeps the store of g (read
     again after the compare); lt_loop() and ge_loop() (ordered compares)
     keep `lda / cmp` with the operands in their place;
  2. on luna, after the 256 values of g: no mismatch against the same
     results computed without the shape, and each function's checksum
     equal to the one of a model of the C computed here;
  3. the provoked fault: rebuilt with QBE_EQ_SWAP_FAULT=1 (an unsigned
     `<` / `>=` swapped too), lt_loop()'s and ge_loop()'s values must be
     wrong; rebuilt with QBE_NO_EQ_SWAP=1 every value must stay right and
     the text check of the taken forms must fail. The ROM is rebuilt clean
     after (`--no-faults` skips this part).
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
from lib import find_luna  # noqa: E402

ROM = HERE / "eq_swap.sfc"
ASM = HERE / "main.c.asm"
STEPS = 120_000_000     # instructions; the run ends near 55 M
M16 = 0xFFFF


def le16(v: int) -> str:
    return (v & M16).to_bytes(2, "little").hex().upper()


def model() -> list[tuple[str, str]]:
    """The C of main.c on the host (int 16 bits)."""
    gen = [(p * 37 + 11) & 0xFF for p in range(256)]
    tv = [(p * 13 + 5) & 0xFF for p in range(256)]
    h = dict.fromkeys(("eq", "miss", "ne", "lt", "ge", "val", "reuse"), 0)

    def mix(v: int, r: int) -> int:
        return ((((v << 5) | (v >> 11)) & M16 ^ r) + 0x9E37) & M16

    for g in range(256):
        r = dict.fromkeys(("eq", "miss", "ne", "lt", "ge", "val", "reuse"), 0)
        for p in range(256):
            x = gen[p]
            if x == g:
                r["eq"] = (r["eq"] + tv[p]) & M16
                r["ne"] ^= p
                r["reuse"] = mix(r["reuse"], (g * 3 + 1) & M16)
            else:
                r["miss"] = (r["miss"] + p) & M16
                r["ne"] = (r["ne"] + tv[p]) & M16
                r["reuse"] = mix(r["reuse"], (x + g) & M16)
            if x < g:
                r["lt"] = (r["lt"] + tv[p]) & M16
                r["ge"] ^= p
            else:
                r["lt"] ^= p
                r["ge"] = (r["ge"] + tv[p]) & M16
            r["val"] = (r["val"] * 3 + (x == g)) & M16
        for k in h:
            h[k] = mix(h[k], r[k])
    cases = [(f"h_{k}", le16(v)) for k, v in h.items()]
    cases += [(f"bad_{k}", "0000") for k in ("eq", "ne", "val", "reuse", "lt", "ge")]
    return cases + [("done", "EFBE")]


def body(text: str, fn: str) -> str:
    head = f"\n{fn}:\n"
    return text.split(head, 1)[1].split(".ENDS", 1)[0] if head in text else ""


TAKEN = re.compile(r"\tlda\.w g\n\tand\.w #\$00FF\n\tcmp")
STORED = re.compile(r"\tlda\.w g\n\tand\.w #\$00FF\n\tsta")
ORDERED = re.compile(r"\tsta\.b (tcc__lf\+\d+)\n\tlda[^\n]*\n\tcmp\.b \1\n\tb(?:cc|cs)")


def text_checks() -> list[tuple[str, bool]]:
    text = ASM.read_text() if ASM.is_file() else ""
    res = []
    for fn in ("eq_loop", "ne_loop", "eq_val"):
        b = body(text, fn)
        res.append((f"{fn}: g compared from A, not stored", bool(TAKEN.search(b))
                    and not STORED.search(b)))
    b = body(text, "eq_reuse")
    res.append(("eq_reuse: g compared from A, its store kept (read after)",
                bool(re.search(r"\tlda\.w g\n\tand\.w #\$00FF\n\tsta[^\n]*\n\tcmp", b))))
    for fn in ("lt_loop", "ge_loop"):
        res.append((f"{fn}: an ordered compare keeps its operands' place",
                    bool(ORDERED.search(body(text, fn)))))
    return res


def values(luna: str) -> dict[str, bool]:
    """One luna run for every assertion: luna prints one PASS / FAIL line
    per --assert, in order."""
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
    env = {k: v for k, v in os.environ.items() if "EQ_SWAP" not in k}
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
        build({"QBE_EQ_SWAP_FAULT": "1"})
        v = values(luna)
        ok = not v["h_lt"] and not v["bad_lt"] and not v["h_ge"] and not v["bad_ge"]
        print(("PASS " if ok else "FAIL ")
              + "QBE_EQ_SWAP_FAULT=1: lt_loop and ge_loop must be wrong (an ordered compare swapped)")
        fails += not ok
        build({"QBE_NO_EQ_SWAP": "1"})
        v = values(luna)
        ok = all(v.values())
        print(("PASS " if ok else "FAIL ") + "QBE_NO_EQ_SWAP=1: every value right")
        fails += not ok
        ok = not any(o for _, o in text_checks()[:3])
        print(("PASS " if ok else "FAIL ") + "QBE_NO_EQ_SWAP=1: g is stored and reloaded again")
        fails += not ok
    finally:
        build({})
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
