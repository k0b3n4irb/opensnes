#!/usr/bin/env python3
"""Runtime assertions for lib/source functions (libtest.sfc).

Builds nothing — assumes `make` produced libtest.sfc. Resolves each result
global from the .sym and asserts its WRAM value via `luna state --assert`.
This is the execution gate the library lacks: examples only exercise lib
functions transitively, and the visual-regression hash can't see a wrong
return value that doesn't change pixels.

Vectors covered (see main.c):
  - math: div16/mod16 (incl. divisor-0 contract and the 65535/1 worst
    case of the old O(quotient) loop), mul16, sqrt16
  - text: cursor_y wrap — the tilemapBuffer overflow guard
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "tools" / "luna-test" / "probes"))
from lib import find_luna, assert_mem  # noqa: E402

ROM = HERE / "libtest.sfc"
# audioInit() blocks on the APU IPL boot + driver upload (~1.5M CPU
# instructions) before the rest of the fixture runs — hence the budget.
STEPS = 3_000_000

# (global, width-bytes, expected-value)
CASES = [
    ("r_div_a",    2, 14),
    ("r_mod_a",    2, 2),
    ("r_div_max",  2, 65535),
    ("r_div_zero", 2, 0),
    ("r_mod_zero", 2, 0),
    ("r_mul",      2, 5535),
    ("r_sdiv_cast", 2, 0xFD9C),  # (s16)-30000/(s16)49 — signed div through casts (#114)
    ("r_nmi_mul",  2, 17243),  # 123*673 & 0xFFFF, computed in the nmiSet callback (#113)
    ("r_nmi_div",  2, 4714),   # 33000/7 in the callback — 8-bit-divisor (hardware) path pre-fix
    ("r_nmi_mod",  2, 2),      # 33000%7 in the callback
    ("r_sqrt",     2, 12),
    ("r_rmw_u8",      2, 200),
    ("r_anim_loop",   2, 10),
    ("r_anim_once",   2, 6),
    ("r_anim_done",   2, 1),
    ("r_anim_switch", 2, 77),
    ("r_anim_cont",   2, 20),
    ("r_anim_stop",   2, 0xFFFF),
    # map getters called from C (issue #103): expected values host-parsed
    # from the committed map_scroll blobs (entry(1280,80)=tile 21,
    # b16[21]=0xFF00=T_SOLID). Pre-fix these read open bus at $00:3000+.
    ("r_map_tile",  2, 21),
    ("r_map_prop",  2, 0xFF00),
    ("r_map_prop0", 2, 0),
    ("s_map_width", 1, 32),
    ("s_cursor_y",  1, 8),
    # audio v2 phase 1: full boot chain (IPL, driver upload, PING) +
    # mirrored master volume. DSP-side effects of the voice setters are
    # asserted by probes/audio_v2.py via spc-dump.
    ("r_audio_ready", 2, 1),
    ("r_audio_vol",   2, 100),
    # phase 2: sample pipeline. load=AUDIO_OK; free = 0xC000-0x0B00-9;
    # slot-0 address = sample base; play returns round-robin voice 0.
    ("r_audio_load",  2, 0),
    ("r_audio_free",  2, 0xB4F7),
    ("r_audio_addr",  2, 0x0B00),
    ("r_audio_voice", 2, 0),
    # phase 3: the DSP->CPU read path — voice 0's envelope is live
    # (looping beep, full-sustain default ADSR) so active == 1.
    ("r_audio_active", 2, 1),
    ("r_done",     2, 0xBEEF),
]


# Expected-fail vectors: real, minimally-pinned compiler bugs. A FAIL here is
# the known baseline; an unexpected PASS (XPASS) means the compiler got fixed —
# promote the vector out. Pattern copied from test_a6_farptr.py.
#
# r_rmw_u8 lived here until opensnes#99 was fixed (qbe w65816 emit: the byte
# load's indirect path now emits rep #$20 before the 16-bit address reload,
# so a preceding byte store no longer corrupts the pointer's high byte). It
# is now a normal passing vector below — the reproducer stays as a permanent
# regression pin.
KNOWN_FAIL = set()


def le_bytes(value: int, width: int) -> str:
    return "".join(f"{(value >> (8 * i)) & 0xFF:02X}" for i in range(width))


def run() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna()
    fails = 0
    for name, width, want in CASES:
        # luna resolves the symbol name itself (v1.7.0, auto-detected .sym)
        ok, detail = assert_mem(luna, ROM, STEPS, [(name, le_bytes(want, width))])
        if name in KNOWN_FAIL:
            if ok:
                print(f"  XPASS {name} == 0x{want:0{width*2}X}  <- fixed! promote out of KNOWN_FAIL")
                fails += 1
            else:
                print(f"  XFAIL {name} (known compiler bug, see KNOWN_FAIL)")
        elif ok:
            print(f"  PASS  {name} == 0x{want:0{width*2}X}")
        else:
            print(f"  FAIL  {name} == 0x{want:0{width*2}X}  [{detail}]")
            fails += 1
    print(f"\nLib runtime assertions: {len(CASES) - fails}/{len(CASES)} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(run())
