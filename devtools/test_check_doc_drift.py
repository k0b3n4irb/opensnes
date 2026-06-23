#!/usr/bin/env python3
"""Regression tests for check_doc_drift.py's TIER-1 extensions (stdlib unittest):
the phantom-API detector and the extended example-count patterns."""
import unittest

from check_doc_drift import COUNT_PATTERNS, phantom_names_in_code

API = {"textPrintAt", "textModeInit", "setScreenOn", "WaitForVBlank",
       "oamSet", "audioPlaySample"}


def _count_matches(line: str):
    for rx in COUNT_PATTERNS:
        m = rx.search(line)
        if m:
            return int(m.group(1))
    return None


class TestPhantomApi(unittest.TestCase):
    def test_flags_phantom_call(self):
        self.assertEqual(phantom_names_in_code('consoleDrawText(8, 10, "x");', API),
                         ["consoleDrawText"])

    def test_real_api_ok(self):
        self.assertEqual(phantom_names_in_code('textPrintAt(8, 10, "x"); setScreenOn();', API), [])

    def test_commented_call_ignored(self):
        self.assertEqual(phantom_names_in_code('// oldThing(0);\n/* gone(1); */', API), [])

    def test_locally_defined_ignored(self):
        self.assertEqual(phantom_names_in_code('void helper(void){} helper();', API), [])

    def test_upper_macro_and_stdlib_ignored(self):
        self.assertEqual(phantom_names_in_code('REG_FOO(1); memcpy(a, b, 4); if (x) {}', API), [])


class TestCountPatterns(unittest.TestCase):
    def test_matches_real_phrasings(self):
        for line in ["56 examples organized by topic", "All 56 examples organized",
                     "**56 examples**", "56 example ROMs", "through 56 examples",
                     "56 examples from \"Hello World\""]:
            self.assertEqual(_count_matches(line), 56, line)

    def test_does_not_match_prose_count(self):
        # bank0_budget.md: "12 examples were flagged" must NOT be read as a corpus count.
        self.assertIsNone(_count_matches("flagged that 12 examples were within"))


if __name__ == "__main__":
    unittest.main()
