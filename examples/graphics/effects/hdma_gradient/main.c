/**
 * HDMA Gradient — Brightness gradient effect using HDMA
 *
 * Demonstrates HDMA writing to the INIDISP register ($2100) to create
 * a vertical brightness gradient across the screen. Press A to cycle
 * through 15 different gradient levels. Press B to reset.
 *
 * Mode 3 (256 colors, 8bpp BG1) with a large tiled image.
 *
 * Based on PVSnesLib HDMAGradient example by Alekmaul.
 * Uses hdmaBrightnessGradient() library helper.
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
