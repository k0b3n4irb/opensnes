/**
 * @file main.c
 * @brief SA-1 Speed Demo — 10.74 MHz coprocessor counter
 * @ingroup examples
 *
 * The SA-1 coprocessor increments a 32-bit counter at 10.74 MHz
 * while the main CPU (3.58 MHz) displays the value each frame.
 * The counter visually demonstrates the SA-1's 3× speed advantage.
 *
 * @par What to Observe
 * - The counter increments extremely fast (millions per second)
 * - This speed is impossible on the main 3.58 MHz CPU alone
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input, sa1
 *
 * @see sa1.h
 */

#include <snes.h>
#include <snes/sa1.h>

/* I-RAM shared memory layout (both CPUs see $3000-$37FF) */
#define SA1_COUNTER_LO  (*(volatile u16*)0x3002)
#define SA1_COUNTER_HI  (*(volatile u16*)0x3004)

int main(void) {
    u16 counter_lo;
    u16 counter_hi;
    u16 prev_lo;
    u16 speed;

    consoleInit();
    setMode(BG_MODE0, 0);

    setColor(0, 0x0000);
    setColor(1, RGB(31, 31, 31));
    setColor(2, RGB(0, 31, 0));

    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    textPrintAt(4, 2, "SA-1 SPEED DEMO");
    textPrintAt(4, 3, "10.74 MHZ COPROCESSOR");

    if (sa1Init()) {
        textPrintAt(4, 6, "SA-1: RUNNING");
    } else {
        textPrintAt(4, 6, "SA-1: FAILED");
    }

    textPrintAt(4, 9, "32-BIT COUNTER:");
    textPrintAt(4, 12, "INCREMENTS/FRAME:");

    setMainScreen(LAYER_BG1);
    textFlush();
    WaitForVBlank();
    setScreenOn();

    prev_lo = 0;

    while (1) {
        /* Read 32-bit counter from I-RAM (SA-1 updates continuously) */
        counter_hi = SA1_COUNTER_HI;
        counter_lo = SA1_COUNTER_LO;

        /* Calculate speed (increments per frame) */
        speed = counter_lo - prev_lo;
        prev_lo = counter_lo;

        /* Display counter value */
        textSetPos(4, 10);
        textPrintHex(counter_hi, 4);
        textPrintHex(counter_lo, 4);

        /* Display speed */
        textSetPos(4, 13);
        textPrintU16(speed);
        textPrintAt(10, 13, "/FRAME    ");

        textFlush();
        WaitForVBlank();
    }
    return 0;
}
