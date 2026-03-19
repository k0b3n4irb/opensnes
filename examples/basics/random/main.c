/**
 * @file main.c
 * @brief Random number generation demo
 * @ingroup examples
 *
 * Generates and displays random 16-bit numbers on button press.
 * Demonstrates the LFSR-based pseudo-random number generator and
 * how to seed it for varied sequences.
 *
 * Every game needs randomness — enemy spawn positions, item drops,
 * damage variance, shuffle order. The SNES has no hardware RNG, so
 * we use a software PRNG seeded from the frame counter at game start.
 *
 * Ported from PVSnesLib "random" example by alekmaul.
 *
 * @par SNES Concepts
 * - rand() — 16-bit LFSR pseudo-random number generator
 * - srand() — seed the PRNG (use frame_count for unpredictable sequences)
 * - Text module for hex and decimal display
 * - padPressed() for single-press button detection
 *
 * @par What to Observe
 * - Press any button to generate a new random number
 * - Both hex and decimal values are shown
 * - Press B to re-seed the PRNG from the frame counter
 * - After re-seeding, the sequence changes unpredictably
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input
 *
 * @see console.h (rand, srand)
 */

#include <snes.h>
#include <snes/text.h>

/** @brief Current random value to display */
u16 current_value;

/**
 * @brief Entry point — display random numbers on button press
 * @return Never returns (infinite game loop)
 */
int main(void) {
    u16 pad;

    /* Step 1: Hardware init */
    consoleInit();
    setMode(BG_MODE0, 0);

    /* Step 2: Palette */
    setColor(0, 0x0000);
    setColor(1, RGB(31, 31, 31));

    /* Step 3: Text setup */
    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* Step 4: Static labels */
    textPrintAt(5, 4, "RANDOM NUMBER GENERATOR");
    textPrintAt(3, 8, "PRESS A FOR NEW NUMBER");
    textPrintAt(3, 9, "PRESS B TO RE-SEED");

    /* Step 5: Generate first value */
    current_value = rand();

    /* Step 6: Screen on (all VRAM ready) */
    setMainScreen(LAYER_BG1);
    textFlush();
    WaitForVBlank();
    setScreenOn();

    /* Step 7: Main loop */
    while (1) {
        WaitForVBlank();

        pad = padPressed(0);

        /* Generate new random number */
        if (pad & KEY_A) {
            current_value = rand();
        }

        /* Re-seed from frame counter (makes sequence unpredictable) */
        if (pad & KEY_B) {
            srand(getFrameCount());
            current_value = rand();
            textPrintAt(6, 18, "** RE-SEEDED **");
        }

        /* Display current value in hex and decimal */
        textSetPos(8, 12);
        textPrint("HEX: 0x");
        textPrintHex(current_value, 4);
        textPrint("  ");

        textSetPos(8, 14);
        textPrint("DEC: ");
        textPrintU16(current_value);
        textPrint("      ");

        textFlush();
    }

    return 0;
}
