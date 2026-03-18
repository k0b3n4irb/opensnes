/**
 * @file main.c
 * @brief Text module validation using textInit / textPrintAt / textFlush
 * @ingroup examples
 *
 * Exercises the OpenSNES text module, which provides a built-in ASCII font
 * and a buffered tilemap writer. The module maintains an off-screen text
 * buffer that is flushed to VRAM with textFlush(), letting you compose
 * multi-line text without manually building tilemap entries. This example
 * loads the default font into VRAM, prints a single string, and displays it
 * on BG1 in Mode 0.
 *
 * @par SNES Concepts
 * - Text module workflow: textInit() -> textLoadFont() -> textPrintAt() -> textFlush()
 * - BG tilemap and tile base pointer configuration with bgSetMapPtr / bgSetGfxPtr
 * - CGRAM palette setup via setColor() for background and text colors
 * - Mode 0 (2bpp) used because the built-in font only needs 4 colors
 *
 * @par What to Observe
 * - Dark blue screen with "TEXT MODULE TEST" displayed in white
 * - If only a dark blue screen appears (no text), textPrintAt or textFlush has a bug
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
 * Demonstrates the OpenSNES text module workflow: consoleInit() sets up
 * hardware, textInit() allocates the off-screen text buffer, textLoadFont()
 * DMA-transfers the built-in ASCII font tiles into VRAM, textPrintAt()
 * writes characters into the buffer, and textFlush() copies the buffer to
 * VRAM as a tilemap. A WaitForVBlank() before setScreenOn() ensures all
 * VRAM writes complete during blanking, preventing PPU corruption.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);

    /* Palette: color 0 = dark blue, color 1 = white */
    setColor(0, 0x2800);
    setColor(1, RGB(31, 31, 31));
    setMainScreen(LAYER_BG1);

    /* STEP 2: textInit (confirmed working) */
    textInit();

    /* STEP 3: font + BG pointers */
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* STEP 4: textPrintAt + textFlush */
    textPrintAt(8, 14, "TEXT MODULE TEST");
    textFlush();

    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
