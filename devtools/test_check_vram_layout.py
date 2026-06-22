#!/usr/bin/env python3
"""Regression tests for check_vram_layout.py (stdlib unittest)."""
import unittest

from check_vram_layout import lint_text


class TestVramLayout(unittest.TestCase):
    def _names(self, text):
        return {name for _, name, *_ in lint_text(text)}

    def test_aligned_literals_pass(self):
        text = """
            bgSetGfxPtr(0, 0x1000);
            bgSetMapPtr(0, 0x0400, BG_MAP_32x32);
            REG_OBJSEL = OBJSEL(OBJ_SIZE8_L16, 0x4000);
        """
        self.assertEqual(lint_text(text), [])

    def test_misaligned_bg_gfx(self):
        # 0x0800 is not 0x1000-aligned -> BG12NBA masks to 0x0000.
        issues = lint_text("bgSetGfxPtr(0, 0x0800);")
        self.assertEqual(len(issues), 1)
        _, name, addr, align, masked = issues[0]
        self.assertEqual((name, addr, align, masked), ("bgSetGfxPtr", 0x0800, 0x1000, 0x0000))

    def test_misaligned_tilemap(self):
        # 0x0401 -> BGnSC masks to 0x0400.
        issues = lint_text("bgSetMapPtr(0, 0x0401, SC_32x32);")
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues[0][4], 0x0400)

    def test_misaligned_objsel(self):
        # 0x1000 is not 0x2000-aligned -> sprites read from 0x0000.
        self.assertEqual(self._names("REG_OBJSEL = OBJSEL(OBJ_SIZE8_L16, 0x1000);"),
                         {"OBJSEL"})

    def test_define_resolution_flags_misaligned(self):
        text = "#define VRAM_TILES 0x0800\nbgSetGfxPtr(0, VRAM_TILES);"
        self.assertEqual(self._names(text), {"bgSetGfxPtr"})

    def test_define_resolution_passes_aligned(self):
        text = "#define VRAM_TILES 0x2000\nbgSetGfxPtr(0, VRAM_TILES);"
        self.assertEqual(lint_text(text), [])

    def test_unresolvable_arg_is_skipped(self):
        # A variable / computed base can't be checked statically -> no false fail.
        self.assertEqual(lint_text("bgSetGfxPtr(0, sprite_base);"), [])
        self.assertEqual(lint_text("#define V (BASE + 0x100)\nbgSetGfxPtr(0, V);"), [])


if __name__ == "__main__":
    unittest.main()
