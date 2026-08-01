/**
 * @file main.c
 * @brief Family 1 (Text) · rung 1.1 — print a string with the built-in font
 * @ingroup examples
 *
 * The gentlest possible program that still exercises the whole SNES rhythm
 * (tiles + palette + VRAM + VBlank). `textModeInit()` sets up the hardware
 * *and* the text engine in one call, `textPrintAt()` writes into an
 * off-screen buffer, and the NMI handler DMAs that buffer to VRAM as a
 * tilemap on the next VBlank — no manual flush needed. One string, on BG1,
 * in Mode 0. This is the first rung of the Text ladder; the under-the-hood
 * version (a glyph as raw 2bpp bitplanes) lives in
 * `examples/fundamentals/text_glyphs`.
 *
 * @par SNES Concepts
 * - The text module in one call: textModeInit() -> textPrintAt() (the NMI auto-flushes)
 * - Colour 0 is the universal backdrop; setColor() writes a 15-bit BGR value
 * - setScreenOn() comes last, after VRAM is ready, to avoid a garbage first frame
 *
 * @par What to Observe
 * - A dark blue screen with "TEXT MODULE TEST" printed in white
 *
 * @par Modules Used
 * console, dma, text, background, sprite
 *
 * @see text.h, video.h, tutorial_graphics
 */

#include <snes.h>

/**
 * @brief Entry point -- initialize text module and display a test string
 *
 * Demonstrates the OpenSNES text module workflow: textModeInit() sets up
 * hardware + the text engine in one call, textPrintAt() writes characters
 * into the off-screen buffer, and the NMI handler DMAs the buffer to
 * VRAM as a tilemap during the next VBlank. A WaitForVBlank() before
 * setScreenOn() ensures all VRAM writes complete during blanking,
 * preventing PPU corruption on the first frame.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    textModeInit();
    setColor(0, RGB(0, 0, 10));

    textPrintAt(8, 14, "TEXT MODULE TEST");

    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
