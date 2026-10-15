#!/usr/bin/env python3
"""Regression tests for check_doc_drift.py's TIER-1 extensions (stdlib unittest):
the phantom-API detector, the extended example-count patterns (incl. the
soft-wrap multi-line scan), path-existence extraction, category-row parsing,
and the ROADMAP footer-date anchor."""
import unittest

import check_doc_drift

from check_doc_drift import (COUNT_PATTERNS, ROADMAP_FOOTER_RE,
                             declared_lib_modules, deprecated_citations_in_text,
                             modules_missing_from_readme,
                             extract_example_paths, parse_category_rows,
                             phantom_names_in_code, retired_tool_lines,
                             sdk_phantoms_in_text,
                             sdk_prefixes, soft_wrap)

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
            extract_example_paths("`testing/bin` and [x](docs/foo.md)",
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



class TestSdkNamesInDocs(unittest.TestCase):
    """2026-09-26: the misses of the audit, as negative controls."""
    API = {"colorMathSetCondition", "colorMathSetLayers", "objInitFunctions",
           "objInitEngine", "snesmodPlay", "snesmodLoadModule", "setScreenOn",
           "setMode"}

    def setUp(self):
        self.prefixes = sdk_prefixes(self.API)

    def test_prefixes_are_shared_module_words(self):
        self.assertIn("colorMath".split("M")[0], self.prefixes)  # "color"
        self.assertIn("obj", self.prefixes)
        self.assertIn("spc", self.prefixes)            # retired PVSnesLib prefix
        self.assertNotIn("set", self.prefixes)         # too generic

    def test_flags_renamed_and_pvsneslib_calls(self):
        text = ("colorMathSetMaskMain(COLORMATH_INSIDE);\n"
                "objRegisterTypes();\n"
                "Load it with `spcLoad(MOD_X)`.\n")
        names = [n for n, _ in sdk_phantoms_in_text(text, self.API, self.prefixes)]
        self.assertEqual(names, ["colorMathSetMaskMain", "objRegisterTypes", "spcLoad"])

    def test_real_calls_user_functions_and_local_definitions_pass(self):
        text = ("colorMathSetCondition(1); startGame(); renderBoard();\n"
                "void objHelper(void) { }\nobjHelper();\n")
        self.assertEqual(sdk_phantoms_in_text(text, self.API, self.prefixes), [])

    def test_deprecated_cited_as_current(self):
        hits = deprecated_citations_in_text("Call `dmaCopyVramBank(src, 1, 0, n)`.\n",
                                            {"dmaCopyVramBank"})
        self.assertEqual(hits, [("dmaCopyVramBank", 1)])

    def test_deprecated_said_so_on_the_line_or_the_one_before(self):
        text = ("| `dmaCopyVramBank` | **Deprecated** (2026-09-20). |\n"
                "It takes the bank. (Deprecated since 2026-09-20:\n"
                "`dmaCopyVramBank`.)\n")
        self.assertEqual(deprecated_citations_in_text(text, {"dmaCopyVramBank"}), [])


class TestRetiredTools(unittest.TestCase):
    def test_flags_current_use(self):
        self.assertEqual(retired_tool_lines("ok\nvalidate in Mesen2\ncd tools/opensnes-emu\n"), [2, 3])

    def test_allows_saying_it_is_retired(self):
        self.assertEqual(retired_tool_lines("Mesen2 was retired on 2026-07-05\n"), [])


class TestExampleModules(unittest.TestCase):
    def test_declared_reads_all_assignment_forms(self):
        mk = "LIB_MODULES := console dma   # c\nLIB_MODULES += sprite\n"
        self.assertEqual(declared_lib_modules(mk), ["console", "dma", "sprite"])

    def test_any_format_counts(self):
        readme = "## Modules Used\n\n| Module | Why |\n|---|---|\n| console | init |\n\nconsole, dma\n## Next\n"
        self.assertEqual(modules_missing_from_readme(readme, ["console", "dma"]), [])

    def test_missing_module_and_missing_section(self):
        self.assertEqual(modules_missing_from_readme("## Modules\n`console`\n", ["console", "gameloop"]), ["gameloop"])
        self.assertIsNone(modules_missing_from_readme("## Build\n", ["console"]))


class BuildKnobs(unittest.TestCase):
    MK = "CSRC ?= main.c\nUSE_HIROM   ?= 0\n# NOTE ?= in a comment\nX := 1\nUSE_HIROM ?= 1\n"

    def test_knobs_in_order_once(self):
        self.assertEqual(check_doc_drift.makefile_knobs(self.MK), ["CSRC", "USE_HIROM"])

    def test_missing_knob(self):
        page = "| `CSRC` | main.c |\nUSE_HIROM without backticks"
        self.assertEqual(check_doc_drift.knobs_missing(["CSRC", "USE_HIROM"], page),
                         ["USE_HIROM"])

    def test_real_tree_is_clean(self):
        self.assertEqual(check_doc_drift.check_build_knobs(), [])

class SlashCount(unittest.TestCase):
    def test_makefile_form(self):
        text = "# CI-gated on 54/56 examples — the two whose"
        self.assertTrue(any(int(m.group(1)) == 56 for r in COUNT_PATTERNS
                            for m in r.finditer(text)))

    def test_ratio_prose_is_not_a_count(self):
        text = "a 3/4 examples ratio"          # single digits: out of range
        self.assertFalse(any(10 <= int(m.group(1)) <= 999 for r in COUNT_PATTERNS
                             for m in r.finditer(text)))


class BenchmarkTable(unittest.TestCase):
    PAGE = (
        "| **Total cycle reduction** | **−20.0 %** vs PVSnesLib + 816-opt (40 vs 50 cycles) |\n"
        "```\n"
        "  add_u16                     30        20        15    -25.0%\n"
        "  sub_u16                     40        30        25    -16.7%\n"
        "  TOTAL                       70        50        40    -20.0%\n"
        "```\n")
    BASE = {"add_u16": 15, "sub_u16": 25, "extra": 9}

    def test_clean(self):
        self.assertEqual(check_doc_drift.bench_table_drifts(self.PAGE, self.BASE), [])

    def test_stale_row(self):
        out = check_doc_drift.bench_table_drifts(self.PAGE, dict(self.BASE, add_u16=14))
        self.assertEqual(len(out), 1)
        self.assertIn("add_u16", out[0])

    def test_wrong_total_and_summary(self):
        page = self.PAGE.replace("50        40    -20.0%", "50        41    -20.0%")
        out = check_doc_drift.bench_table_drifts(page, self.BASE)
        self.assertTrue(any("add up to" in d for d in out))
        page = self.PAGE.replace("−20.0 %", "−32.2 %")
        out = check_doc_drift.bench_table_drifts(page, self.BASE)
        self.assertTrue(any("summary" in d for d in out))


class NoHistoryInUserDocs(unittest.TestCase):
    RX = check_doc_drift._HISTORY_RX

    def test_dated_history_is_refused(self):
        for line in ("Until 2026-10-05 the build refused it.",
                     "### 🟢 `sramClear()` wrote a ramp (fixed 2026-09-15)",
                     "works since 2026-10-03 for objects in bank $00",
                     "SIWP polarity resolved 2026-09-02"):
            self.assertIsNotNone(self.RX.search(line), line)

    def test_present_tense_and_versions_pass(self):
        for line in ("It is refused since 0.49.", "PVSnesLib at `fa758c9b 2025-12-28`.",
                     "*Last updated: 2026-10-14.*", "the loop runs until the terminator"):
            self.assertIsNone(self.RX.search(line), line)

    def test_maintainer_notes_link(self):
        self.assertIsNotNone(check_doc_drift._NOTES_RX.search("see `.claude/notes/tech/x.md`"))


class ExampleReview20261015(unittest.TestCase):
    """The examples review before 1.0: stale motifs, @par Modules Used,
    files a comment names (anchor 19)."""

    def _stale(self, line):
        return [why for rx, why in check_doc_drift._STALE_EXAMPLE_CLAIMS if rx.search(line)]

    def test_new_stale_motifs(self):
        for line in ("WH0/WH1 must be refreshed every scanline",
                     "C pointer dereferences are bank-$00-hardcoded",
                     "Stack: 5-6,s = sy (cproc right-to-left push)",
                     "the map CANNOT be read from C directly",
                     "converted from a PNG by gfx4snes",
                     "smconv -s -o soundbank music/song.it",
                     "Verify smconv is built: `make -C $OPENSNES/bin smconv`"):
            self.assertTrue(self._stale(line), line)

    def test_present_wording_passes(self):
        for line in ("an entry writes its register once and holds it",
                     "the build runs opensnes-music bank on the .it file",
                     "smconv keeps its page (docs/tools/smconv.md)",
                     "C reads it as a const array"):
            self.assertFalse(self._stale(line), line)

    def test_par_modules(self):
        main = "/**\n * @par Modules Used\n * console, dma, sprite\n *\n * @see x.h\n */"
        self.assertEqual(check_doc_drift.modules_missing_from_par(main, ["console", "dma", "anim"]), ["anim"])
        self.assertEqual(check_doc_drift.modules_missing_from_par(main, ["console", "sprite"]), [])
        self.assertIsNone(check_doc_drift.modules_missing_from_par("/** no par */", ["console"]))

    def test_named_file_regex(self):
        rx = check_doc_drift._NAMED_FILE_RX
        for line, name in (("/** @brief tiles (defined in data.asm) */", "data.asm"),
                           ("Sprite palette (BGR555) from data.asm */", "data.asm"),
                           ("lives in res/track_class.bin", "res/track_class.bin")):
            m = rx.search(line)
            self.assertIsNotNone(m, line)
            self.assertEqual(m.group(1), name)
        self.assertIsNone(rx.search("converted by the build from res/"))

    def test_examples_cited(self):
        path = "@subpage examples_sprites_simple_sprite\n@ref examples_audio_echo"
        readmes = {"sprites": "| [simple_sprite](simple_sprite/) |", "audio": ""}
        gaps = check_doc_drift.example_citation_gaps(
            ["sprites/simple_sprite", "audio/echo", "sprites/world_sprites"], path, readmes)
        self.assertEqual(gaps, ["audio/echo (its category README)",
                                "sprites/world_sprites (learning path) (its category README)"])

    def test_named_file_missing(self):
        import tempfile
        from pathlib import Path
        with tempfile.TemporaryDirectory() as td:
            ex = Path(td)
            (ex / "res").mkdir()
            (ex / "res" / "hero.png.toml").write_text("tool = \"opensnes-sprite\"\n")
            (ex / "data.asm").write_text("")
            self.assertFalse(check_doc_drift.named_file_missing(ex, "data.asm"))
            self.assertFalse(check_doc_drift.named_file_missing(ex, "hero.pic"))   # produced by a settings file
            self.assertTrue(check_doc_drift.named_file_missing(ex, "ram.asm"))
            self.assertFalse(check_doc_drift.named_file_missing(ex, "crt0.asm"))   # a file of the SDK


if __name__ == "__main__":
    unittest.main()
