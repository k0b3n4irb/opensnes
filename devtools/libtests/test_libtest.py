#!/usr/bin/env python3
"""Runtime assertions for lib/source functions (libtest.sfc).

Builds nothing — assumes `make` produced libtest.sfc. Resolves each result
global from the .sym and asserts its WRAM value via `luna state --assert`.
This is the execution gate the library lacks: examples only exercise lib
functions transitively, and the visual-regression hash can't see a wrong
return value that doesn't change pixels.

Vectors covered (see main.c):
  - math: div16/mod16 (incl. divisor-0 contract and the 65535/1 worst
    case of the old O(quotient) loop), mul16, sqrt16, fixMul/fixDiv/fixLerp
    and fix32Mul/fix32Div (L2b)
  - text: cursor_y wrap — the tilemapBuffer overflow guard
  - L2c (2026-09-15): collision (rect/point/tile), sram round trip in bank
    $70, the raw IRQ path (V/H timer counts), console region + vblank
    getters, and the window module asserted on luna's PPU register view
    (`luna state` JSON: windows, w12sel, w34sel, wobjsel, wbglog, wobjlog,
    tmw, tsw) — write-only registers the WRAM oracle cannot see.

`--region pal` runs the same ROM under `--force-region pal`: getRegion() /
isPAL() must then read 1 and every other vector must hold (the PAL pass of
the gaps review, R2).
"""
from __future__ import annotations

import argparse
import json
import subprocess

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
    # fixed-point helpers (L2b, 2026-09-14): signs, fractions, zero divisor
    ("r_fmul_a",    2, 0x0100), ("r_fmul_neg",  2, 0xFA00), ("r_fmul_frac", 2, 0x0240), ("r_fmul_nn", 2, 0x0100),
    ("r_fdiv_a",    2, 5120),   ("r_fdiv_frac", 2, 64),     ("r_fdiv_neg",  2, 0xFD00), ("r_fdiv_zero", 2, 0),
    ("r_lerp_mid",  2, 12800),  ("r_lerp_t0",   2, 2560),   ("r_lerp_down", 2, 3840),   ("r_lerp_t255", 2, 25500),
    ("r_f32mul",    4, 0x00060000), ("r_f32mul_n", 4, 0xFFFA0000), ("r_f32mul_f", 4, 0x00024000),
    ("r_f32div",    4, 0x00030000), ("r_f32div_n", 4, 0xFFFD0000), ("r_f32div_f", 4, 0x00004000), ("r_f32div_r", 4, 0x00005555),
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
    # L2c: collision — geometry in the main.c comments
    ("r_col_rect", 2, 1), ("r_col_rect_no", 2, 0), ("r_col_pt_in", 2, 1), ("r_col_pt_edge", 2, 0),
    ("r_col_ex", 2, 1), ("r_col_ex_ox", 2, 0xFFFA), ("r_col_ex_oy", 2, 0xFFFA),
    ("r_col_ex_no", 2, 0), ("r_col_ex_no_ox", 2, 0),
    ("r_col_tile", 2, 1), ("r_col_tile0", 2, 0), ("r_col_tile_neg", 2, 1), ("r_col_tile_far", 2, 1),
    ("r_col_tile16", 2, 1), ("r_col_tile8x", 2, 2), ("r_col_rtile0", 2, 0), ("r_col_rtile1", 2, 1),
    ("r_rect_x", 2, 5), ("r_rect_y", 2, 6), ("r_rect_w", 2, 7), ("r_rect_h", 2, 8),
    ("r_rect_px", 2, 0xFFFD), ("r_rect_py", 2, 9), ("r_rect_cx", 2, 18), ("r_rect_cy", 2, 18),
    ("r_rect_in", 2, 1), ("r_rect_out", 2, 0),
    # L2c: sram — bank $70 round trip, offsets, XOR checksum, clear
    ("r_sram_rt", 2, 16), ("r_sram_off", 2, 22), ("r_sram_off0", 2, 1),
    ("r_sram_ck", 2, 32), ("r_sram_ck0", 2, 0), ("r_sram_clear", 2, 0),
    # L2c: IRQ path — one V-timer IRQ per waited frame, none while disabled,
    # the default handler after irqClear() acknowledges without counting
    ("r_irq_a", 2, 10), ("r_irq_b", 2, 10), ("r_irq_c", 2, 12), ("r_irq_d", 2, 12),
    # input: an idle connected pad must read as connected. padIsConnected()
    # rejected $0000 as well as $FFFF until 2026-09-18, so a pad with nothing
    # pressed — almost every frame — reported unplugged.
    ("r_pad_conn",  2, 0xFF),   # TRUE
    ("r_pad_idle",  2, 0),      # nothing pressed
    ("r_pad_conn4", 2, 0),      # multitap slot: nothing can fill it, so FALSE
    ("r_pad_oob",   2, 0),      # out of range
    # L2c: console — HVBJOY bit 7 right after WaitForVBlank, then clear.
    # The getters return TRUE, which snes/types.h defines as 0xFF (not 1).
    ("r_invb_in", 2, 0xFF), ("r_invb_out", 2, 0),
    ("r_done",     2, 0xBEEF),
]

# Region getters: NTSC by default (the header's country byte), PAL under
# `--region pal` (luna --force-region). Same ROM, same asserts otherwise.
REGION_CASES = {
    "ntsc": [("r_region", 2, 0), ("r_ispal", 2, 0)],
    "pal":  [("r_region", 2, 1), ("r_ispal", 2, 0xFF)],   # isPAL() returns TRUE = 0xFF
}

# Window module: the PPU registers luna reports in `luna state` JSON (ppu.*)
# after the windowSplit(100) / windowCentered(WINDOW_2, 64) tail of main.c.
PPU_CASES = [
    ("windows", [0, 99, 96, 159]),  # WH0..WH3
    ("w12sel", 0x03),   # W1 on BG1, inverted
    ("w34sel", 0x08),   # W2 on BG3
    ("wobjsel", 0x02),  # W1 on OBJ; W2/MATH enabled then disabled
    ("wbglog", 0x08),   # BG2 XOR
    ("wobjlog", 0x01),  # OBJ AND
    ("tmw", 0x11),      # main mask BG1 | OBJ
    ("tsw", 0x04),      # sub mask BG3
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


def ppu_state(luna: str, region: str) -> dict:
    cmd = [luna, "state", "-n", str(STEPS), "--out", "-", *region_args(region), str(ROM)]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=300, check=True)
    return json.loads(proc.stdout)["ppu"]


def region_args(region: str) -> list[str]:
    return ["--force-region", region] if region != "ntsc" else []


def run(region: str = "ntsc") -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make` first)")
    luna = find_luna()
    fails = 0
    extra = region_args(region)
    if region != "ntsc":
        print(f"  (luna --force-region {region})")
    for name, width, want in CASES + REGION_CASES[region]:
        # luna resolves the symbol name itself (v1.7.0, auto-detected .sym)
        ok, detail = assert_mem(luna, ROM, STEPS, [(name, le_bytes(want, width))], extra=extra)
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
    ppu = ppu_state(luna, region)
    for field, want in PPU_CASES:
        got = ppu.get(field)
        if got == want:
            print(f"  PASS  ppu.{field} == {want}")
        else:
            print(f"  FAIL  ppu.{field} == {want}  [luna reports {got}]")
            fails += 1
    total = len(CASES) + len(REGION_CASES[region]) + len(PPU_CASES)
    print(f"\nLib runtime assertions ({region}): {total - fails}/{total} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--region", choices=("ntsc", "pal"), default="ntsc",
                    help="video standard forced on luna (default: the ROM header, NTSC)")
    sys.exit(run(ap.parse_args().region))
