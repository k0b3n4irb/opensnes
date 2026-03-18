/**
 * @file main.c
 * @brief Mode 7 perspective with HDMA split-screen (F-Zero style)
 * @ingroup examples
 *
 * Creates an F-Zero-style pseudo-3D ground plane by combining two BG modes
 * on a single screen using HDMA mid-frame register switching. The top portion
 * (96 scanlines) displays a Mode 3 sky background, while the bottom portion
 * (128 scanlines) renders a Mode 7 ground plane with per-scanline perspective
 * scaling. Four HDMA channels cooperate each frame: one switches BGMODE from
 * Mode 3 to Mode 7 at the split line, another toggles the main screen layer
 * enable (BG2 for sky, BG1 for ground), and two more write per-scanline M7A
 * and M7D values to create the perspective foreshortening effect where distant
 * rows appear more compressed.
 *
 * @par SNES Concepts
 * - Mid-frame BG mode switching via HDMA (Mode 3 sky + Mode 7 ground)
 * - Per-scanline M7A/M7D writes for perspective projection
 * - HDMA-driven main screen layer enable toggling
 * - Mode 7 ground plane scrolling with D-pad input
 * - Multi-channel HDMA coordination (4 channels synchronized)
 *
 * @par What to Observe
 * - The screen is split: a flat sky image on top and a perspective ground below
 * - Press D-pad to scroll the ground plane in any direction
 * - Distant ground rows appear more compressed (perspective foreshortening)
 *
 * @par Modules Used
 * console, dma, background, sprite, input, mode7
 *
 * @see mode7.h, hdma.h, background.h, input.h
 */

#include <snes.h>

/* Assembly helpers */
extern void asm_loadSkyData(void);
extern void asm_setupHdmaPerspective(u16 sx, u16 sy);

/* Ground data (defined in data.asm) */
extern u8 ground_map[], ground_map_end[];
extern u8 ground_tiles[], ground_tiles_end[];
extern u8 ground_pal[], ground_pal_end[];

u16 pad0;
u16 sx, sy;

int main(void) {
    consoleInit();

    /* Force blank for VRAM loading */
    setScreenOff();

    /* Load Mode 7 ground (1024x1024, 256 colors) to VRAM $0000 + palette */
    dmaCopyVramMode7(ground_map, ground_map_end - ground_map,
                     ground_tiles, ground_tiles_end - ground_tiles);
    dmaCopyCGram(ground_pal, 0, ground_pal_end - ground_pal);

    /* Load sky tiles+map to VRAM $4000/$5000 */
    asm_loadSkyData();

    /* Configure BG2 for sky display (used during Mode 3 portion):
     * Tilemap at VRAM $4000, 64x32 tiles, tile data at VRAM $5000 */
    REG_BG2SC = 0x41;    /* ($4000 / $400) << 2 | 1 (64x32) */
    REG_BG12NBA = 0x50;  /* BG1 tiles at $0000, BG2 tiles at $5000 */

    /* Start in Mode 7 — HDMA switches to Mode 3 for sky portion */
    setMode(BG_MODE7, 0);
    mode7Init();

    /* Initialize scroll */
    sx = 0;
    sy = 0;

    /* First HDMA setup + scroll register writes */
    asm_setupHdmaPerspective(sx, sy);

    /* Display on */
    setScreenOn();

    while (1) {
        pad0 = padHeld(0);

        if (pad0 & KEY_LEFT)  sx--;
        if (pad0 & KEY_RIGHT) sx++;
        if (pad0 & KEY_UP)    sy--;
        if (pad0 & KEY_DOWN)  sy++;

        /* Update scroll registers and re-enable HDMA every frame */
        asm_setupHdmaPerspective(sx, sy);

        WaitForVBlank();
    }

    return 0;
}
