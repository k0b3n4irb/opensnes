"""Probe: audio v2 driver — DSP effects of the C command layer.

The libtest fixture (devtools/libtests) calls audioInit() then a batch
of setters with known vectors (see its main.c). The WRAM-visible
results are asserted by test_libtest.py; THIS probe asserts the other
half of the contract: the driver actually poked the S-DSP. We spc-dump
the libtest ROM after the fixture ran and check the DSP register file:

  MVOL L/R = 100            (audioSetVolume(100))
  V2 VOLL/VOLR = 80/40      (audioSetVoiceVolume(2, 80, 40))
  V3 PITCH = $1234          (audioSetVoicePitch(3, 0x1234))
  V1 ADSR = $FF/$E8         (audioSetADSR(1, 15,7,7,8) — the packing)
  V4 ADSR1 = 0, GAIN = $5A  (audioSetGain zeroes ADSR1: hot-swap hygiene)
  DIR = $0A                 (driver init: directory page $0A00)

plus the driver image itself at ARAM $0200 (upload integrity).
"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

from lib import find_luna

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
LIBTEST_ROM = REPO / "devtools" / "libtests" / "libtest.sfc"
DRIVER_BIN = REPO / "lib" / "source" / "audio_driver.spc700.bin"
STEPS = 3_000_000  # matches test_libtest.py: past audioInit + setters


def run() -> tuple[bool, str]:
    luna = find_luna()
    if not LIBTEST_ROM.is_file():
        return False, f"libtest.sfc not built ({LIBTEST_ROM})"

    with tempfile.NamedTemporaryFile(suffix=".spc", delete=False) as tf:
        spc_path = Path(tf.name)
    try:
        proc = subprocess.run(
            [luna, "spc-dump", "-n", str(STEPS), "-o", str(spc_path),
             str(LIBTEST_ROM)],
            capture_output=True, text=True, timeout=300,
        )
        if proc.returncode != 0:
            return False, f"spc-dump failed: {proc.stderr.strip()[:150]}"
        spc = spc_path.read_bytes()
    finally:
        spc_path.unlink(missing_ok=True)

    aram = spc[0x100:0x10100]
    dsp = spc[0x10100:0x10180]

    beep = bytes.fromhex("c37799779977997799")
    checks = [
        (dsp[0x0C] == 100 and dsp[0x1C] == 100,
         f"MVOL={dsp[0x0C]}/{dsp[0x1C]} (want 100/100)"),
        (dsp[0x20] == 80 and dsp[0x21] == 40,
         f"V2VOL={dsp[0x20]}/{dsp[0x21]} (want 80/40)"),
        (dsp[0x32] == 0x34 and dsp[0x33] == 0x12,
         f"V3PITCH={dsp[0x33]:02X}{dsp[0x32]:02X} (want 1234)"),
        (dsp[0x15] == 0xFF and dsp[0x16] == 0xE8,
         f"V1ADSR={dsp[0x15]:02X}/{dsp[0x16]:02X} (want FF/E8)"),
        (dsp[0x45] == 0x00 and dsp[0x47] == 0x5A,
         f"V4 ADSR1={dsp[0x45]:02X} GAIN={dsp[0x47]:02X} (want 00/5A)"),
        (dsp[0x5D] == 0x0A, f"DIR={dsp[0x5D]:02X} (want 0A)"),
        # phase 2: the sample pipeline. audioLoadSample streamed the
        # 9-byte beep to $0B00 and DIR_SET wrote entry 0 (loop offset 0
        # -> loop == start); audioPlaySampleEx keyed voice 0 with
        # pan-center volumes 59/67, default ADSR $8F/$E0, pitch $1000.
        (aram[0x0B00:0x0B09] == beep,
         "beep BRR streamed to $0B00"),
        (aram[0x0A00:0x0A04] == bytes([0x00, 0x0B, 0x00, 0x0B]),
         f"DIR[0]={aram[0x0A00:0x0A04].hex()} (want 000b000b)"),
        (dsp[0x04] == 0, f"V0SRCN={dsp[0x04]} (want 0)"),
        (dsp[0x00] == 59 and dsp[0x01] == 67,
         f"V0VOL={dsp[0x00]}/{dsp[0x01]} (want 59/67 = pan center)"),
        (dsp[0x02] == 0x00 and dsp[0x03] == 0x10,
         f"V0PITCH={dsp[0x03]:02X}{dsp[0x02]:02X} (want 1000)"),
        (dsp[0x05] == 0x8F and dsp[0x06] == 0xE0,
         f"V0ADSR={dsp[0x05]:02X}/{dsp[0x06]:02X} (want 8F/E0 default)"),
        (dsp[0x08] > 0, f"V0ENVX={dsp[0x08]} (want >0 — beep sounding)"),
        # phase 3: echo config (audioSetEcho(3,40,20,20) + FIR[0]=96 +
        # EnableEcho(0x01)) — ESA $C0, ring cleared, writes enabled.
        (dsp[0x6D] == 0xC0, f"ESA={dsp[0x6D]:02X} (want C0)"),
        (dsp[0x7D] == 3, f"EDL={dsp[0x7D]} (want 3)"),
        (dsp[0x0D] == 40, f"EFB={dsp[0x0D]} (want 40)"),
        (dsp[0x2C] == 20 and dsp[0x3C] == 20,
         f"EVOL={dsp[0x2C]}/{dsp[0x3C]} (want 20/20)"),
        (dsp[0x0F] == 96, f"FIR0={dsp[0x0F]} (want 96)"),
        (dsp[0x4D] == 0x01, f"EON={dsp[0x4D]:02X} (want 01)"),
        (dsp[0x6C] & 0x20 == 0, f"FLG={dsp[0x6C]:02X} (echo writes enabled)"),
    ]

    if DRIVER_BIN.is_file():
        drv = DRIVER_BIN.read_bytes()
        checks.append(
            (aram[0x200:0x200 + len(drv)] == drv,
             f"driver image at $0200 ({len(drv)} bytes)"))
    else:
        checks.append((False, f"driver bin missing ({DRIVER_BIN}) — build the lib"))

    bad = [m for ok, m in checks if not ok]
    if bad:
        return False, "FAILED: " + "; ".join(bad)
    return True, "; ".join(m for _, m in checks)


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "audio_v2: " + msg)
    sys.exit(0 if ok else 1)
