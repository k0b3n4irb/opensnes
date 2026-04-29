/**
 * @file main.c
 * @brief Single controller input display
 * @ingroup examples
 *
 * Reads the SNES controller and displays which button is currently held.
 * The simplest possible input example — one controller, one text line,
 * real-time feedback. This is the starting point for understanding how
 * SNES games read player input.
 *
 * The SNES joypad has 12 buttons: 4 face buttons (A, B, X, Y), 2 shoulder
 * buttons (L, R), a D-pad (4 directions), START, and SELECT. The NMI
 * handler reads the joypad automatically every VBlank. Your code just
 * calls padHeld(0) to get the current state as a 16-bit bitmask.
 *
 * Ported from PVSnesLib "controller" example by alekmaul.
 *
 * @par SNES Concepts
 * - padHeld(0) — 16-bit bitmask of currently held buttons
 * - KEY_A, KEY_B, KEY_X, KEY_Y, KEY_L, KEY_R — button constants
 * - KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT — D-pad constants
 * - KEY_START, KEY_SELECT — system button constants
 * - NMI handler reads joypads automatically (padUpdate is a no-op)
 *
 * @par What to Observe
 * - Press any button — its name appears on screen immediately
 * - Release — the display clears
 * - Try pressing multiple buttons — only one is shown (first match)
 * - The response is instant (1 frame = 16ms latency)
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input
 *
 * @see input.h
 */

#include <snes.h>
#include <snes/text.h>

/**
 * @brief Entry point — display held button name in real time
 * @return Never returns (infinite game loop)
 */
int main(void) {
    u16 pad;

    /* Init hardware */
    textModeInit();

    /* Static labels */
    textPrintAt(12, 1, "PAD TEST");
    textPrintAt(6, 5, "USE PAD TO SEE VALUE");

    /* Screen on (all VRAM ready) */
    WaitForVBlank();
    setScreenOn();

    /* Main loop */
    while (1) {
        WaitForVBlank();

        pad = padHeld(0);

        /* Display which button is held */
        if (pad & KEY_A)
            textPrintAt(9, 10, "A PRESSED      ");
        else if (pad & KEY_B)
            textPrintAt(9, 10, "B PRESSED      ");
        else if (pad & KEY_X)
            textPrintAt(9, 10, "X PRESSED      ");
        else if (pad & KEY_Y)
            textPrintAt(9, 10, "Y PRESSED      ");
        else if (pad & KEY_L)
            textPrintAt(9, 10, "L PRESSED      ");
        else if (pad & KEY_R)
            textPrintAt(9, 10, "R PRESSED      ");
        else if (pad & KEY_UP)
            textPrintAt(9, 10, "UP PRESSED     ");
        else if (pad & KEY_DOWN)
            textPrintAt(9, 10, "DOWN PRESSED   ");
        else if (pad & KEY_LEFT)
            textPrintAt(9, 10, "LEFT PRESSED   ");
        else if (pad & KEY_RIGHT)
            textPrintAt(9, 10, "RIGHT PRESSED  ");
        else if (pad & KEY_START)
            textPrintAt(9, 10, "START PRESSED  ");
        else if (pad & KEY_SELECT)
            textPrintAt(9, 10, "SELECT PRESSED ");
        else
            textPrintAt(9, 10, "               ");

    }

    return 0;
}
