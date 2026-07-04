#!/usr/bin/env python3
"""Regression tests for check_doc_drift.py's TIER-1 extensions (stdlib unittest):
the phantom-API detector, the extended example-count patterns (incl. the
soft-wrap multi-line scan), path-existence extraction, category-row parsing,
and the ROADMAP footer-date anchor."""
import unittest

from check_doc_drift import (COUNT_PATTERNS, ROADMAP_FOOTER_RE,
                             extract_example_paths, parse_category_rows,
                             phantom_names_in_code, soft_wrap)

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

    def test_matches_heading_and_ratio_forms(self):
        # ROADMAP forms that historically escaped the scan.
        self.assertEqual(_count_matches("### Examples (56)"), 56)
        self.assertEqual(_count_matches("READMEs with explanations (56 / 56)"), 56)

    def test_does_not_match_prose_count(self):
        # bank0_budget.md: "12 examples were flagged" must NOT be read as a corpus count.
        self.assertIsNone(_count_matches("flagged that 12 examples were within"))


class TestSoftWrap(unittest.TestCase):
    def test_line_wrapped_claim_matches(self):
        # ROADMAP.md historically hid "54\nworking examples" from the
        # line-by-line scan; the soft-wrapped view must expose it.
        text = "The suite has 54\nworking examples covering everything."
        self.assertIsNone(_count_matches(text.splitlines()[0]))
        self.assertEqual(_count_matches(soft_wrap(text)), 54)

    def test_offsets_preserved_and_paragraphs_kept(self):
        text = "alpha\nbeta\n\ngamma"
        wrapped = soft_wrap(text)
        self.assertEqual(len(wrapped), len(text))       # offset-preserving
        self.assertEqual(wrapped, "alpha beta\n\ngamma")  # blank line survives


class TestExamplePaths(unittest.TestCase):
    CATS = {"text", "memory", "graphics"}

    def test_extracts_backticked_and_linked_paths(self):
        text = ("see `memory/superfx_3d` and\n"
                "[text/hello_world](text/hello_world/) but not `lib/source/dma`")
        found = extract_example_paths(text, self.CATS)
        self.assertEqual([p for p, _ in found],
                         ["memory/superfx_3d", "text/hello_world"])
        self.assertEqual([ln for _, ln in found], [1, 2])

    def test_non_category_prefix_ignored(self):
        self.assertEqual(
            extract_example_paths("`tools/luna-test/bin` and [x](docs/foo.md)",
                                  self.CATS), [])


class TestCategoryRows(unittest.TestCase):
    def test_parses_readme_table_rows(self):
        text = ("| [text/](text/) | 2 | Text display |\n"
                "| [games/](games/) | 5 | Complete games |\n"
                "| not a row | x |\n")
        self.assertEqual(parse_category_rows(text), {"text": 2, "games": 5})


class TestFooterDate(unittest.TestCase):
    def test_footer_regex(self):
        m = ROADMAP_FOOTER_RE.search("*Last updated: 2026-07-04. Anchored*")
        self.assertEqual(m.group(1), "2026-07-04")
        self.assertIsNone(ROADMAP_FOOTER_RE.search("Last update was recent"))


if __name__ == "__main__":
    unittest.main()
