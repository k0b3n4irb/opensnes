/**
 * @file main.c
 * @brief Text module validation using textInit / textLoadFont / textPrintAt
 * @ingroup examples
 *
 * Exercises the OpenSNES text module, which provides a built-in ASCII font
 * and a buffered tilemap writer. The module maintains an off-screen text
 * buffer; every text writer (textPutChar, textPrint, textPrintAt, ...)
 * sets a dirty flag and the NMI handler DMAs the buffer to VRAM during
 * the next VBlank — no manual flush call is needed. This example loads
 * the default font into VRAM, prints a single string, and displays it
 * on BG1 in Mode 0.
 *
 * @par SNES Concepts
 * - Text module workflow: textInit() -> textLoadFont() -> textPrintAt() (NMI auto-flushes)
 * - BG tilemap and tile base pointer configuration with bgSetMapPtr / bgSetGfxPtr
 * - CGRAM palette setup via setColor() for background and text colors
 * - Mode 0 (2bpp) used because the built-in font only needs 4 colors
 *
 * @par What to Observe
 * - Dark blue screen with "TEXT MODULE TEST" displayed in white
 * - If only a dark blue screen appears (no text), textPrintAt has a bug
 * - If the screen is black, a crash occurred during text module initialization
 *
 * @par Modules Used
 * console, dma, text, background, sprite
 *
 * @see text.h, background.h, video.h
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
