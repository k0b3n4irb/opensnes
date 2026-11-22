"""Unit tests of check_wram_journal.py: the check must be seen failing."""
import datetime
import hashlib
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import check_wram_journal as c  # noqa: E402

B = b'{"a": 1}\n'
H = hashlib.sha256(B).hexdigest()[:12]
ROW = "| 2026-11-09 | abcd1234 | 3 of 65 | benign | a reason | `%s` |\n"


class Journal(unittest.TestCase):
    def test_matching_last_row_passes(self):
        self.assertIsNone(c.mismatch(B, ROW % H))

    def test_rewrite_without_a_line_is_refused(self):
        self.assertIn("rewritten without a line", c.mismatch(B + b" ", ROW % H))

    def test_only_the_last_row_counts(self):
        text = (ROW % ("0" * 12)) + (ROW % H)
        self.assertIsNone(c.mismatch(B, text))
        self.assertIsNotNone(c.mismatch(B, (ROW % H) + (ROW % ("0" * 12))))

    def test_empty_journal_is_refused(self):
        self.assertIsNotNone(c.mismatch(B, "# nothing\n"))

    def test_stats_counts_drifts_and_catches_in_the_window(self):
        text = ("| 2026-11-09 | a | 65 of 65 | benign | x | `%s` |\n"
                "| 2026-11-08 | b | 1 of 65 | **CATCH** | y | `%s` |\n"
                "| 2026-11-07 | c | 0 of 65 | benign | no rewrite | `%s` |\n"
                "| 2026-09-01 | d | 65 of 65 | benign | old | `%s` |\n") % (H, H, H, H)
        n, k = c.stats(text, 30, today=datetime.date(2026, 11, 9))
        self.assertEqual((n, k), (2, 1))


if __name__ == "__main__":
    unittest.main()
