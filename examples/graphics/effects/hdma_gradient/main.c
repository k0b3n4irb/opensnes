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

/**
 * @brief Assembly DMA loader for the 8bpp Mode 3 background (defined in data.asm).
 *
 * Handles DMA transfers with correct bank bytes for SUPERFREE data sections.
 * The 8bpp tile data is ~32KB, which exceeds bank $00's ROM space and spills
 * into bank $01. The C-level dmaCopyVram() hardcodes bank $00 in its DMA source
 * address, so it would read garbage for data in bank $01+. This assembly routine
 * uses the linker-resolved `:label` bank byte to set the correct DMA source bank.
 *
 * Transfers: tiles to VRAM $1000, tilemap to VRAM $0000, palette to CGRAM slot 0.
 * Must be called during forced blank.
 */
extern void loadGraphics(void);

/**
 * @brief Entry point -- HDMA brightness gradient on a Mode 3 background.
 *
 * Loads a 256-color (8bpp) background image via the assembly DMA loader,
 * then enters an interactive loop where button A cycles through brightness
 * gradient levels (top stays bright, bottom darkens progressively), and
 * button B restores uniform full brightness. The gradient is implemented
 * via hdmaBrightnessGradient(), which builds an HDMA table that writes
 * per-scanline values to INIDISP ($2100).
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    /** @brief Current bottom brightness for the gradient (15=full, 0=black).
     *
     * Each press of A decrements this value, making the bottom of the screen
     * darker. When it drops below 2, it wraps back to 15. The top brightness
     * is always 15 (full). */
    u8 gradient = 15;

    /* Initialize console (sets up NMI handler, enables auto-joypad read) */
    consoleInit();

    /* Force blank for VRAM writes -- bit 7 of INIDISP ($2100) enables
     * forced blank, which halts PPU rendering and allows unrestricted
     * VRAM/CGRAM/OAM access. Unlike VBlank (which only gives ~4KB DMA
     * budget), forced blank has no time limit. */
    setScreenOff();

    /* Load all graphics via assembly DMA (correct bank bytes).
     * The assembly routine handles the :label bank byte for SUPERFREE
     * sections that may reside in ROM banks other than $00. */
    loadGraphics();

    /* Configure BG1: tilemap at $0000, tiles at $1000 (word addresses).
     * REG_BG1SC ($2107): bits 7-2 = tilemap base address >> 10, bits 1-0 = size.
     * REG_BG12NBA ($210B): low nybble = BG1 tile base >> 12, high = BG2. */
    REG_BG1SC = (0x0000 >> 8) | 0x00;  /* SC_32x32 */
    REG_BG12NBA = 0x01;                 /* BG1 tiles at $1000 */

    /* Mode 3: 256 colors (8bpp) on BG1, 16 colors (4bpp) on BG2.
     * Only BG1 is used here. Mode 3 is the standard mode for displaying
     * full-color images (photos, painted backgrounds) on the SNES. */
    setMode(BG_MODE3, 0);

    /* Screen on */
    setScreenOn();

    while (1) {
        /* Press A to activate and cycle gradient levels (15 -> 2 -> 15).
         * hdmaBrightnessGradient() builds an HDMA table in RAM that
         * interpolates INIDISP brightness from 'gradient' (top of screen)
         * to 0 (bottom of screen). Each press decreases 'gradient',
         * making the bright region smaller. HDMA must be set up during
         * VBlank to avoid partial-frame artifacts. */
        if (padPressed(0) & KEY_A) {
            WaitForVBlank();
            hdmaBrightnessGradient(HDMA_CHANNEL_3, gradient, 0);
            gradient--;
            if (gradient < 2) gradient = 15;
        }

        /* Press B to stop the gradient and restore full brightness.
         * hdmaBrightnessGradientStop() disables the HDMA channel and
         * writes INIDISP = 0x0F (full brightness, forced blank off). */
        if (padPressed(0) & KEY_B) {
            hdmaBrightnessGradientStop(HDMA_CHANNEL_3);
            gradient = 15;
        }

        WaitForVBlank();
    }

    return 0;
}
