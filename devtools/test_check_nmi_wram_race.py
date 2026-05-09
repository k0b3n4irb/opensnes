#!/usr/bin/env python3
"""
Regression tests for check_nmi_wram_race.py.

Builds synthetic .asm fixtures in a tempdir and asserts the lint
returns the expected exit code:
  - clean fixture → 0
  - direct $2180 write inside NMI callback → 1
  - transitive $2181 write through a callee → 1
  - $2180 write in a function that's NOT in the NMI closure → 0
"""

import subprocess
import sys
import tempfile
from pathlib import Path


SCRIPT = Path(__file__).parent / "check_nmi_wram_race.py"


def write_fixture(tmpdir: Path, combined: str, main_c_asm: str) -> None:
    (tmpdir / "combined.asm").write_text(combined)
    (tmpdir / "main.c.asm").write_text(main_c_asm)


def run_lint(tmpdir: Path) -> tuple[int, str]:
    result = subprocess.run(
        [sys.executable, str(SCRIPT), "--rom-dir", str(tmpdir), "--quiet"],
        capture_output=True,
        text=True,
    )
    return result.returncode, result.stdout + result.stderr


# Minimal combined.asm: NmiHandler chains directly to a hand-written
# nmi_payload() helper. DefaultNmiCallback is empty.
COMBINED_BASE = """
NmiHandler:
    jml.l FastNmi
FastNmi:
    jsl tilemapFlush
    rti
DefaultNmiCallback:
    rtl
tilemapFlush:
    rtl
"""


def test_clean_callback() -> None:
    """Callback registered with nmiSet that does nothing dangerous."""
    main_asm = """
my_nmi_cb:
    nop
    rtl
main:
    pea.w my_nmi_cb
    jsl nmiSet
    rtl
"""
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        write_fixture(td_path, COMBINED_BASE, main_asm)
        rc, out = run_lint(td_path)
        assert rc == 0, f"clean callback should pass, got rc={rc}\n{out}"


def test_direct_port_write() -> None:
    """Callback that writes to $2181 directly — must be flagged."""
    main_asm = """
my_nmi_cb:
    lda #$05
    sta.l $002181
    rtl
main:
    pea.w my_nmi_cb
    jsl nmiSet
    rtl
"""
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        write_fixture(td_path, COMBINED_BASE, main_asm)
        rc, out = run_lint(td_path)
        assert rc == 1, f"direct port write should fail, got rc={rc}\n{out}"
        assert "my_nmi_cb" in out, f"violation should name my_nmi_cb\n{out}"
        assert "$002181" in out, f"violation should quote $002181\n{out}"


def test_transitive_port_write() -> None:
    """Callback that calls another function which writes $2180 — flagged."""
    main_asm = """
helper:
    sta $2180
    rtl
my_nmi_cb:
    jsl helper
    rtl
main:
    pea.w my_nmi_cb
    jsl nmiSet
    rtl
"""
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        write_fixture(td_path, COMBINED_BASE, main_asm)
        rc, out = run_lint(td_path)
        assert rc == 1, f"transitive port write should fail, got rc={rc}\n{out}"
        assert "helper" in out, f"violation should name helper\n{out}"


def test_unreachable_port_write() -> None:
    """A function that writes $2180 but is NEVER called from any
    NMI root → not flagged (lint scope is the NMI closure only)."""
    main_asm = """
unrelated_func:
    stz $2180
    rtl
my_nmi_cb:
    nop
    rtl
main:
    pea.w my_nmi_cb
    jsl nmiSet
    jsl unrelated_func
    rtl
"""
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        write_fixture(td_path, COMBINED_BASE, main_asm)
        rc, out = run_lint(td_path)
        assert rc == 0, f"unreachable port write should NOT fail, got rc={rc}\n{out}"


def test_nmiSetBank_callback() -> None:
    """nmiSetBank takes (callback, bank) — same first-arg pattern."""
    main_asm = """
my_nmi_cb:
    sta $2183
    rtl
main:
    pea.w my_nmi_cb
    pea.w 1
    jsl nmiSetBank
    rtl
"""
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        write_fixture(td_path, COMBINED_BASE, main_asm)
        rc, out = run_lint(td_path)
        assert rc == 1, f"nmiSetBank callback should be analysed too, got rc={rc}\n{out}"


def test_indirect_jump_not_followed() -> None:
    """`jml [<addr>]` is indirect; target unknown statically. The
    target is added as a root via the nmiSet detection, not by
    following the indirect jump."""
    main_asm = """
nasty:
    sta $2181
    rtl
my_nmi_cb:
    jml [some_pointer]
    rtl
main:
    pea.w my_nmi_cb
    jsl nmiSet
    jsl nasty
    rtl
"""
    with tempfile.TemporaryDirectory() as td:
        td_path = Path(td)
        write_fixture(td_path, COMBINED_BASE, main_asm)
        rc, out = run_lint(td_path)
        assert rc == 0, (
            "indirect jump should not pull `nasty` into the closure; "
            f"got rc={rc}\n{out}"
        )


TESTS = [
    test_clean_callback,
    test_direct_port_write,
    test_transitive_port_write,
    test_unreachable_port_write,
    test_nmiSetBank_callback,
    test_indirect_jump_not_followed,
]


def main() -> int:
    failed = 0
    for test in TESTS:
        name = test.__name__
        try:
            test()
        except AssertionError as e:
            print(f"FAIL  {name}: {e}")
            failed += 1
        except Exception as e:
            print(f"ERROR {name}: {e}")
            failed += 1
        else:
            print(f"PASS  {name}")
    if failed:
        print(f"\n{failed}/{len(TESTS)} test(s) failed")
        return 1
    print(f"\n{len(TESTS)}/{len(TESTS)} tests passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
