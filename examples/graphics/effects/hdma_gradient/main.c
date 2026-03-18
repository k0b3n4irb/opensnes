/**
 * @file main.c
 * @brief HDMA brightness gradient on an 8bpp Mode 3 background
 * @ingroup examples
 *
 * Demonstrates a vertical brightness gradient using HDMA writes to the
 * INIDISP register ($2100). Each scanline group receives a different
 * brightness level (0-15), creating a smooth fade from full brightness
 * at the top to darkness at the bottom. The hdmaBrightnessGradient()
 * library helper builds the HDMA table at runtime, interpolating between
 * a top brightness and a bottom brightness across 224 scanlines. The
 * background is an 8bpp Mode 3 image loaded via an assembly DMA helper
 * (required because the 32KB tile data spans multiple ROM banks, and
 * the C-level dmaCopyVram() hardcodes bank $00).
 *
 * @par SNES Concepts
 * - HDMA writing to INIDISP ($2100) for per-scanline brightness control
 * - hdmaBrightnessGradient() library helper for runtime table generation
 * - Mode 3 (8bpp, 256-color) background rendering
 * - Assembly DMA loader for multi-bank ROM data (SUPERFREE sections)
 *
 * @par What to Observe
 * - Press A repeatedly to cycle through gradient levels (top stays bright, bottom darkens)
 * - Press B to stop the gradient and restore full uniform brightness
 * - The 8bpp background image displays in full 256-color detail
 *
 * @par Modules Used
 * console, dma, background, sprite, hdma, input, math
 *
 * @see hdma.h, video.h, background.h, input.h
 */
#include <snes.h>
#include <snes/console.h>
#include <snes/video.h>
#include <snes/background.h>
#include <snes/input.h>
#include <snes/hdma.h>

/* Assembly loader — handles DMA with correct bank bytes for SUPERFREE data.
 * See data.asm: tile data (32KB) lands in bank $01, so dmaCopyVram()
 * (which hardcodes bank $00) would read garbage. */
extern void loadGraphics(void);

int main(void) {
    u8 gradient = 15;

    /* Initialize console (sets up NMI handler) */
    consoleInit();

    /* Force blank for VRAM writes */
    REG_INIDISP = 0x80;

    /* Load all graphics via assembly DMA (correct bank bytes) */
    loadGraphics();

    /* Configure BG1: tilemap at $0000, tiles at $1000 (word addresses) */
    REG_BG1SC = (0x0000 >> 8) | 0x00;  /* SC_32x32 */
    REG_BG12NBA = 0x01;                 /* BG1 tiles at $1000 */

    /* Mode 3 (256 colors on BG1), enable BG1 only */
    setMode(BG_MODE3, 0);

    /* Screen on */
    setScreenOn();

    while (1) {
        /* Press A to activate and cycle gradient levels (15 → 2 → 15) */
        if (padPressed(0) & KEY_A) {
            WaitForVBlank();
            hdmaBrightnessGradient(HDMA_CHANNEL_3, gradient, 0);
            gradient--;
            if (gradient < 2) gradient = 15;
        }

        /* Press B to stop the gradient and restore full brightness */
        if (padPressed(0) & KEY_B) {
            hdmaBrightnessGradientStop(HDMA_CHANNEL_3);
            gradient = 15;
        }

        WaitForVBlank();
    }

    return 0;
}
