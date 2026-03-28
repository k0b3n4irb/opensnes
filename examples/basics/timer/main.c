/**
 * @file main.c
 * @brief VBlank frame counter display
 * @ingroup examples
 *
 * Displays a live frame counter that increments every VBlank (60 fps NTSC,
 * 50 fps PAL). Demonstrates the simplest possible real-time display: read
 * a hardware counter, render it as text, repeat.
 *
 * The SNES NMI handler increments `frame_count` every VBlank. This is the
 * heartbeat of every SNES game — animation timing, input polling, physics
 * steps, and audio processing are all synchronized to this counter.
 *
 * Ported from PVSnesLib "timer" example by alekmaul.
 *
 * @par SNES Concepts
 * - VBlank frame counter (frame_count from crt0.asm, incremented by NMI)
 * - Text module for real-time number display (textPrintU16 + textFlush)
 * - WaitForVBlank synchronization (main loop runs at exactly 60/50 fps)
 * - Mode 0 with 2bpp font for text output
 *
 * @par What to Observe
 * - The counter increments by 1 every frame (60 per second on NTSC)
 * - At 65535 it wraps to 0 (~18 minutes of runtime)
 * - The display never stutters because each frame does minimal work
 *
 * @par Modules Used
 * console, sprite, dma, background, text
 *
 * @see console.h (getFrameCount), text.h
 */

#include <snes.h>
#include <snes/text.h>

/**
 * @brief Entry point — display a live VBlank frame counter
 * @return Never returns (infinite game loop)
 */
int main(void) {
    /* Step 1: Hardware init */
    textModeInit();

    /* Step 5: Static labels */
    textPrintAt(9, 8, "VBLANK COUNTER");

    /* Step 6: Screen enable */
    textFlush();
    WaitForVBlank();
    setScreenOn();

    /* Step 7: Main loop — update counter display every frame */
    while (1) {
        WaitForVBlank();

        /* Reset counter on A press */
        if (padPressed(0) & KEY_A) {
            frame_count = 0;
        }

        textSetPos(10, 10);
        textPrint("COUNT=");
        textPrintU16(frame_count);
        textPrint("   ");
        textPrintAt(7, 14, "PRESS A TO RESET");
        textFlush();
    }

    return 0;
}
