/**
 * @file main.c
 * @brief VBlank frame counter display, written against the gameloop framework
 * @ingroup examples
 *
 * Displays a live frame counter that increments every VBlank (60 fps NTSC,
 * 50 fps PAL). The simplest possible real-time display: read a hardware
 * counter, render it as text, repeat. Doubles as the canonical demo of
 * the opt-in `gameloop` framework — `init()` runs the standard SNES
 * boot sequence, `update()` runs once per VBlank, and main() never sees
 * a `while(1)` itself.
 *
 * Equivalent hand-rolled main loop:
 * @code{.c}
 *     while (1) {
 *         WaitForVBlank();
 *         update();
 *     }
 * @endcode
 * Both ship the same observable behaviour; the framework exists so the
 * loop boilerplate isn't repeated 53 times across the SDK and so future
 * scene/state machinery has one place to plug into.
 *
 * @par SNES Concepts
 * - VBlank frame counter (frame_count from crt0.asm, incremented by NMI)
 * - Text module for real-time number display (textPrintU16; NMI auto-flushes)
 * - gameloop framework — init/update callbacks, hidden VBlank sync
 * - Mode 0 with 2bpp font for text output
 *
 * @par What to Observe
 * - The counter increments by 1 every frame (60 per second on NTSC)
 * - At 65535 it wraps to 0 (~18 minutes of runtime)
 * - The display never stutters because each frame does minimal work
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input, gameloop
 *
 * @see gameloop.h, console.h (getFrameCount), text.h
 */

#include <snes.h>
#include <snes/text.h>
#include <snes/gameloop.h>

/**
 * @brief One-time hardware setup. Runs before the first VBlank sync.
 *
 * Standard text-mode boot: textModeInit() sets BG mode + text engine,
 * the PrintAt writes our static label into the off-screen tilemap
 * buffer, the explicit WaitForVBlank ensures the very first NMI has
 * fired (so the buffer has been DMA'd to VRAM) before we turn the
 * screen on.
 */
static void on_init(void) {
    textModeInit();
    textPrintAt(9, 8, "VBLANK COUNTER");
    WaitForVBlank();
    setScreenOn();
}

/**
 * @brief Per-frame work. Called once per VBlank by the gameloop.
 *
 * The framework has already done WaitForVBlank() before calling us,
 * so we are inside the VBlank window — VRAM/CGRAM/OAM writes are
 * safe at this point. Nothing here exceeds the budget so we never
 * drop a frame.
 */
static void on_update(void) {
    if (padPressed(0) & KEY_A) {
        frame_count = 0;
    }
    textSetPos(10, 10);
    textPrint("COUNT=");
    textPrintU16(frame_count);
    textPrint("   ");
    textPrintAt(7, 14, "PRESS A TO RESET");
}

/**
 * @brief Entry point — hand off to gameLoopRun().
 *
 * gameLoopRun never returns; the `return 0` exists only to satisfy
 * `int main(void)`'s control-flow contract.
 */
int main(void) {
    GameLoopConfig cfg = {
        .init = on_init,
        .update = on_update,
    };
    gameLoopRun(&cfg);
    return 0;
}
