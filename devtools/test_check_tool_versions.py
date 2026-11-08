"""Unit tests of check_tool_versions.py: the check must be seen failing."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import check_tool_versions as c  # noqa: E402

OK = "opensnes-rom 1.0.0 (OpenSNES 0.55.0, commit 0621c14c)"


class Faults(unittest.TestCase):
    def test_right_line_passes(self):
        self.assertIsNone(c.fault_of(OK, "opensnes-rom", "0.55.0"))
        self.assertIsNone(c.fault_of(OK.replace("c14c)", "c14c+dirty)"), "opensnes-rom", "0.55.0"))
        self.assertIsNone(c.fault_of(OK.replace("0621c14c", "unknown"), "opensnes-rom", "0.55.0"))

    def test_old_bare_version_is_a_fault(self):
        self.assertIsNotNone(c.fault_of("opensnes-rom 1.0.0", "opensnes-rom", "0.55.0"))

    def test_stale_sdk_version_is_a_fault(self):
        self.assertIsNotNone(c.fault_of(OK, "opensnes-rom", "0.56.0"))

    def test_malformed_commit_is_a_fault(self):
        self.assertIsNotNone(c.fault_of(OK.replace("0621c14c", "0621c1"), "opensnes-rom", "0.55.0"))
        self.assertIsNotNone(c.fault_of(OK.replace("0621c14c", "ZZZZZZZZ"), "opensnes-rom", "0.55.0"))

    def test_wrong_tool_name_is_a_fault(self):
        self.assertIsNotNone(c.fault_of(OK, "opensnes-sprite", "0.55.0"))

    def test_empty_output_is_a_fault(self):
        self.assertIsNotNone(c.fault_of("", "opensnes-rom", "0.55.0"))


if __name__ == "__main__":
    unittest.main()
