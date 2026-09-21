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
from lib import find_luna, assert_mem, dump_vram  # noqa: E402

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
    # object engine (2026-09-18): the workspace is owner-tracked, so a callback
    # that peeks at another object no longer overwrites itself with it; and a
    # type with no registered callback is skipped instead of jumping to $00:0000
    ("r_obj_alive", 2, 0xA11E),   # objUpdateAll returned at all
    ("r_obj_calls", 2, 1),        # the registered callback ran once
    ("r_obj_type",  2, 0),        # the peeker is still itself (was: a copy, type 1)
    ("r_obj_edit",  2, 0x1234),   # its pre-peek edit survived
    ("r_obj_other", 2, 0x0BAD),   # the object it looked at is untouched
    ("r_obj_fr_off", 2, 0x0300),  # objCollidMap1D: no friction unless opted in
    ("r_obj_fr_x",   2, 0x0200),  # objInitFriction1D(0x100): xvel decelerates
    ("r_obj_fr_y",   2, 0),       # ...and a small yvel clamps at zero, no sign flip
    ("r_obj_pool",   2, 80),      # objKillAll returns the WHOLE pool (was 79: a slot leaked)
    # coverage lot B (2026-09-19): the public functions nothing executed
    ("r_fix_abs_n",    2, 0x0300), ("r_fix_abs_p",    2, 0x0200),
    ("r_fix_clamp_lo", 2, 0xFF00), ("r_fix_clamp_hi", 2, 0x0100), ("r_fix_clamp_in", 2, 0x0080),
    ("r_fix_sqrt",     2, 0x0400),
    # atan2_8: four axes, the diagonal, and a mid-LUT value (26.57 deg = 18.9 -> 19)
    ("r_atan_e", 2, 0), ("r_atan_s", 2, 64), ("r_atan_w", 2, 128), ("r_atan_n", 2, 192),
    ("r_atan_se", 2, 32), ("r_atan_lut", 2, 19),
    ("r_bg_sx",        2, 300),    ("r_bg_sy",        2, 77),     ("r_bg_init",      2, 0),
    ("r_text_x",       2, 2),      ("r_text_flush",   2, 1),      ("r_frame_reset",  2, 0),
    ("mapoptions",     1, 3),      ("r_pad_raw",      2, 0),     # mapSetMapOptions(1WAY|BG2), bank $7E byte
    ("r_mouse",        2, 0),      ("r_mouse_sens",   2, 0),     # no mouse: the NMI never applies the request...
    ("mouseRequestChangeSensitivity", 1, 0x82),                  # ...but mouseSetSensitivity(0, HIGH) recorded it
    ("r_scope",        2, 0),      ("r_scope_delay",  2, 7),
    ("r_obj_grav",     2, 0x0040), ("r_obj_refresh",  2, 2),      # both objects are on screen
    ("r_obj_cobj",     2, 1), ("r_obj_cobj_h", 2, 0x0100),      ("r_obj_cobj_no",  2, 0),
    ("r_prof_frames",  2, 1),      ("r_prof_scan",    2, 1),
    ("r_mosaic",       2, 15),
    # bank-byte chantier (2026-09-20): data outside bank $00, asymmetric values
    ("r_bank_irq",     2, 4),      # plain irqSet reached a handler in banks 7-1
    ("r_bank_sram",    2, 16),     # const template saved and read back intact
    ("r_bank_ck",      2, 0x10),   # sramChecksum read the ROM bytes, not WRAM
    # sram bounds: capacity from the ROM header's SRAMSIZE byte (8 KB here); a refusal copies nothing
    ("r_sram_edge", 2, 0), ("r_sram_range", 2, 1), ("r_sram_kept", 2, 8),
    ("r_sram_ldrange", 2, 0x5A01), ("r_sram_wrap", 2, 1),
    # const Rect in an asset bank, read far through the now-const parameters
    ("r_crect_hit", 2, 1), ("r_crect_miss", 2, 0), ("r_crect_cx", 2, 18), ("r_crect_cy", 2, 24),
    ("r_crect_bk", 2, 1),          # premise: the const Rect is outside bank $00
    # functions that could not report failure (API audit 3.5)
    ("r_getptr_live", 2, 2), ("r_getptr_stale", 2, 0),
    ("r_scene_push", 2, 8), ("r_scene_full", 2, 0), ("r_scene_pop", 2, 1),
    ("r_f32div_zero",  2, 0),      # fix32Div by zero: 0 like the rest of the family (was 0xFFFFFFFF)
    ("r_bank_irq_bk",  2, 1),      # premise: the handler really is outside bank $00
    ("r_bank_tpl_bk",  2, 1),      # premise: so is the const template
    # audio error returns (API audit 3.5, 2026-09-21): they used to be swallowed
    ("r_aud_init", 2, 0), ("r_aud_badvoice", 2, 2), ("r_aud_badstop", 2, 2),
    ("r_aud_setvol", 2, 0), ("r_aud_noplay", 2, 0xFF),
    ("r_aud_on", 2, 6), ("r_aud_on_bad", 2, 0xFF), ("r_aud_on_rr", 2, 1),   # audioPlaySampleOn: the caller picks the voice
    # types: fixLerp's t is a u16 so 1.0 is reachable; sprite ids are u16 so the range check sees 256
    ("r_lerp_t256", 2, 9472), ("r_lerp_t300", 2, 9472), ("r_oam_id256", 2, 0x4221),
    # coverage lot C (2026-09-20)
    ("r_aud_v0_live",  2, 1),      ("r_aud_v0_stop",  2, 0),
    ("r_aud_v1_live",  2, 1),      ("r_aud_all_stop", 2, 0),
    ("r_aud_unload",   2, 3),      ("r_aud_unfree",   2, 0xB500),
    ("r_meta_n",       2, 12),     ("oam_dyn_sprite_size", 1, 16),
    # fixed32: the asm sine and the C expression the header says is miscompiled
    ("r_f32sin_asm", 4, 0xFFFF0000),   # fix32Sin(192) = -1.0 in 16.16
    ("r_f32sin_c",   4, 0xFFFF0000),   # the same, computed in C
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
    # coverage lot B (2026-09-19). Dotted keys step into the JSON.
    ("mosaic", 0xF0),               # mosaicSetSize(20) clamped to 15, no layer
    ("setini", 0x06),               # OBJ interlace + overscan on, pseudo-hires set then cleared
    ("cgadsub", 0x41),              # colorMathTransparency50(BG1): half + BG1, add
    ("cgwsel", 0x12),               # colorMathSetCondition(INSIDE): bits 5-4 = 01; bit 1: sub-screen source
    ("coldata_r", 10), ("coldata_g", 10), ("coldata_b", 20),   # SetBrightness(10) then SetChannel(BLUE, 20)
    ("bgs.1.h_scroll", 300), ("bgs.1.v_scroll", 76),   # bgSetScrollX/Y(1, 300, 77): VOFS = y - 1
    ("cgram.250", 0x001F), ("cgram.251", 0x03E0),      # dmaCopyCGramBank: red, green
    ("cgram.254", 0x7C00), ("cgram.255", 0x7FFF),      # dmaTransfer to CGDATA: blue, white
    ("oam_full.14", 0xAB), ("oam_full.15", 0x01),       # oamSetTile(3, 0x1AB) + dmaCopyOam
    # bank-byte chantier: dmaCopyOam from a const (ROM) table
    ("oam_full.0", 0x4D), ("oam_full.1", 0x58), ("oam_full.2", 0x5A), ("oam_full.3", 0x31),
    ("oam_full.4", 0x21), ("oam_full.5", 0x43), ("oam_full.6", 0x65), ("oam_full.7", 0x07),
    # lot C: oamDrawMetaFlip(10, x=100, y=50, flipX, box 16): item dx=0 -> 108, dx=8 -> 100;
    # bit 6 of the attribute byte is the H-flip the mirror set
    ("oam_full.40", 108), ("oam_full.41", 49), ("oam_full.44", 100), ("oam_full.45", 49),   # OAM Y = y - 1
    ("oam_full.43", 0x40),
]

# VRAM bytes written by bgInitTileSetData (16 at word 0x6000) and
# dmaCopyVramBank (16 more at word 0x6008): the fixture's lotb_vram pattern.
VRAM_CASES = [
    (0xC000, bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x10,
                    0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90,
                    0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x01, 0x02])),
    # dmaFillVRAM(0x1234, word 0x6100, 8 bytes): a word fill (was 34 34 34 34 ...)
    (0xC200, bytes([0x34, 0x12] * 4)),
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
        got = ppu
        for step in field.split("."):
            try:
                got = got[int(step)] if isinstance(got, list) else got.get(step)
            except (IndexError, ValueError, AttributeError):
                got = None
                break
        if got == want:
            print(f"  PASS  ppu.{field} == {want}")
        else:
            print(f"  FAIL  ppu.{field} == {want}  [luna reports {got}]")
            fails += 1
    vram = dump_vram(luna, ROM, STEPS)
    for addr, want in VRAM_CASES:
        got = vram[addr:addr + len(want)]
        if got == want:
            print(f"  PASS  vram[{addr:#06x}..+{len(want)}] == pattern")
        else:
            print(f"  FAIL  vram[{addr:#06x}..+{len(want)}] == pattern  [got {got.hex()}]")
            fails += 1
    total = len(CASES) + len(REGION_CASES[region]) + len(PPU_CASES) + len(VRAM_CASES)
    print(f"\nLib runtime assertions ({region}): {total - fails}/{total} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--region", choices=("ntsc", "pal"), default="ntsc",
                    help="video standard forced on luna (default: the ROM header, NTSC)")
    sys.exit(run(ap.parse_args().region))
