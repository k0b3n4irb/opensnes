/**
 * @file main.c
 * @brief Family 1 (Text) · rung 1.4 — move a message across the screen
 * @ingroup examples
 *
 * The Text ladder's "make it move" rung. It prints one line, then scrolls
 * the whole text background horizontally every frame, so the message marches
 * across the screen like a marquee and wraps around the 32x32 (256-pixel)
 * tilemap seamlessly. The lesson: text lives on a BG layer, so you move it
 * exactly like any other background — with bgSetScroll().
 *
 * @par SNES Concepts
 * - Text is just a BG layer: bgSetScroll(0, x, 0) scrolls BG1 (the text layer)
 * - Scroll writes are buffered — bgSetScroll() stores a value and marks it
 *   dirty; the NMI handler pushes it to the PPU scroll registers during
 *   VBlank, so the scroll never tears
 * - A 32x32 tilemap is exactly one screen wide, so incrementing X wraps the
 *   message around for a continuous marquee, for free
 *
 * @par What to Observe
 * - "OPENSNES -- TEXT THAT MOVES!" scrolling right-to-left, looping forever
 *
 * @par Modules Used
 * console, dma, text, background, sprite
 *
 * @see background.h, text.h, tutorial_scrolling
 */

#include <snes.h>

/**
 * @brief Entry point — print one line, then scroll BG1 forever.
 *
 * The message is written to the text tilemap once. From then on the frame
 * loop only changes BG1's horizontal scroll offset; the PPU re-reads the same
 * tilemap from a shifted origin, so the text appears to travel without a
 * single extra VRAM write.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    u16 scroll_x = 0;

    textModeInit();
    setColor(0, RGB(0, 0, 10));

    /* Printed once; the tilemap then scrolls under a fixed viewport. */
    textPrintAt(2, 14, "OPENSNES -- TEXT THAT MOVES!");

    WaitForVBlank();
    setScreenOn();

    while (1) {
        /* Increasing X scrolls the view right, so the text moves left.
         * bgSetScroll only buffers the value; the NMI flushes it in VBlank. */
        scroll_x++;
        bgSetScroll(0, scroll_x, 0);
        WaitForVBlank();
    }

    return 0;
}
