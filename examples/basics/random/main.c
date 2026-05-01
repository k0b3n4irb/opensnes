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
#include <snes/gameloop.h>

/** @brief Current random value to display */
u16 current_value;

/**
 * @brief One-time setup. Static labels + first random + screen on.
 */
static void on_init(void) {
    textModeInit();
    textPrintAt(5, 4, "RANDOM NUMBER GENERATOR");
    textPrintAt(3, 8, "PRESS A FOR NEW NUMBER");
    textPrintAt(3, 9, "PRESS B TO RE-SEED");
    current_value = rand();
    WaitForVBlank();
    setScreenOn();
}

/**
 * @brief Per-frame: read input, regenerate on A, re-seed on B, print value.
 *
 * The framework calls this once per VBlank — `padPressed()` reads the
 * NMI-updated joypad buffer and returns only the bits that just
 * transitioned (single-press detection), so holding the button down
 * keeps producing the same number until release.
 */
static void on_update(void) {
    u16 pad = padPressed(0);

    if (pad & KEY_A) {
        current_value = rand();
    }

    if (pad & KEY_B) {
        srand(getFrameCount());
        current_value = rand();
        textPrintAt(6, 18, "** RE-SEEDED **");
    }

    textSetPos(8, 12);
    textPrint("HEX: 0x");
    textPrintHex(current_value, 4);
    textPrint("  ");

    textSetPos(8, 14);
    textPrint("DEC: ");
    textPrintU16(current_value);
    textPrint("      ");
}

/**
 * @brief Entry point — gameLoopRun never returns.
 */
int main(void) {
    GameLoopConfig cfg = {
        .init = on_init,
        .update = on_update,
    };
    gameLoopRun(&cfg);
    return 0;
}
