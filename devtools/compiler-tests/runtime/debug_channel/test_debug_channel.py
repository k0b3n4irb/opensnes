"""Runtime harness: SDK debug channel (snes/debug.h → lib/source/debug.asm).

Nothing in the example corpus uses SNES_NOCASH / SNES_ASSERT, so this is the only
coverage of the debug module. The ROM writes a known Nocash string to $21FC and
fires a failing SNES_ASSERT (WDM $00). luna captures both channels: `--nocash-out`
must contain the string; `--wdm-out` must be non-empty (assertion fired).

Run: `cd devtools/compiler-tests/runtime/debug_channel && make && python3 test_debug_channel.py`
"""
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[3] / "tools" / "luna-test" / "probes"))
from lib import find_luna  # noqa: E402

ROM = HERE / "debug_channel.sfc"
MSG = "LUNA_DBG_OK"
STEPS = 200_000


def run():
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run make)")
    luna = find_luna()
    with tempfile.TemporaryDirectory() as td:
        nocash, wdm = Path(td) / "n.txt", Path(td) / "w.txt"
        subprocess.run([luna, "run", "-n", str(STEPS),
                        "--nocash-out", str(nocash), "--wdm-out", str(wdm), str(ROM)],
                       capture_output=True, text=True, timeout=120)
        ntxt = nocash.read_text(errors="replace") if nocash.is_file() else ""
        wtxt = wdm.read_text() if wdm.is_file() else ""
    if MSG not in ntxt:
        return False, f"nocash channel: '{MSG}' not in $21FC output ({ntxt!r:.60})"
    if not wtxt.strip():
        return False, "wdm channel: SNES_ASSERT(false) did not fire WDM $00"
    return True, f"nocash '{MSG}' captured + SNES_ASSERT fired WDM ({wtxt.strip().splitlines()[0]})"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "debug_channel: " + msg)
    sys.exit(0 if ok else 1)
