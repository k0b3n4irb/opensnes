/**
 * @file main.c
 * @brief Mode 7 EXTBG: one plane split in two layers, an orb flying between them
 * @ingroup examples
 *
 * Mode 7 has a single layer — unless SETINI bit 6 (EXTBG) is set. Then BG2
 * shows the same plane as BG1 (same tilemap, same pixels, same transform),
 * but reads each pixel's bit 7 as a priority bit and bits 0-6 as its colour.
 * Front to back: sprites 3, sprites 2, BG2 pixels with bit 7 set, sprites 1,
 * BG1, sprites 0, BG2 pixels with bit 7 clear. A priority-1 sprite therefore
 * flies over the pixels whose bit 7 is clear and under those whose bit 7 is
 * set, in one and the same Mode 7 plane.
 *
 * The plane is the city seen from above with the elevated metro over it:
 * the viaducts and the train are drawn with colours 0x81-0x8D, the streets,
 * roofs and canal below with colours 16 and up. An energy orb crosses the
 * screen on its own, over the roofs and under the viaducts; press A to
 * switch EXTBG off (BG1 shown instead: one layer, the orb in front of
 * everything) and on.
 *
 * @par SNES Concepts
 * - SETINI ($2133) bit 6 through `mode7SetExtBg()`
 * - Mode 7 EXTBG priority: BG2's high pixels above priority-1 sprites, its
 *   low pixels below every sprite and below BG1 (hence BG2 alone on screen)
 * - One pixel, two colours: 0x81 is colour 1 on BG2 (7-bit) and colour 129
 *   on BG1 (8-bit), so the palette carries the viaducts' colours twice
 * - Mode 7 VRAM interleave (`dmaCopyVramMode7`)
 *
 * @par What to Observe
 * - The orb passes over the roofs, the streets and the canal, and under the
 *   three viaducts and the train — the same Mode 7 plane, split by bit 7 of
 *   its pixels.
 * - Press A: EXTBG off, BG1 on screen, the orb now covers the viaducts too.
 *
 * @par Modules Used
 * console, dma, sprite, input, mode7
 *
 * @see mode7.h (mode7SetExtBg), docs/tutorials/mode7.md
 */

#include <snes.h>
#include <snes/mode7.h>

#include "res/plane.inc"   /* generated: plane_tiles[], plane_map[], plane_pal[] (Mode 7 .pc7 / .mp7), each with _end */
#include "res/orb.inc"     /* generated: orb_tiles[], orb_pal[], each with _end */

/** @brief Sprite tiles in VRAM (words): name base slot 2, past Mode 7's 32 KB */
#define VRAM_OBJ   0x4000
/** @brief The orb's height on screen, in pixels */
#define ORB_Y      96

/** @brief Probe oracles: EXTBG on, the orb's x */
u8 extbg_on;
u16 orb_x;

/** @brief EXTBG on: BG2 (split by bit 7) on screen; off: BG1 */
static void applyExtBg(void) {
    mode7SetExtBg(extbg_on);
    setMainScreen(extbg_on ? (LAYER_BG2 | LAYER_OBJ) : (LAYER_BG1 | LAYER_OBJ));
}

int main(void) {
    s16 vx = 1;

    consoleInit();
    setScreenOff();

    dmaCopyVramMode7(plane_map, plane_map_end - plane_map,
                     plane_tiles, plane_tiles_end - plane_tiles);
    /* CGRAM 0-143: the city and the viaducts, whose colours 1-13 are
     * repeated at 129-141 for BG1, which reads all eight bits */
    dmaCopyCGram(plane_pal, 0, plane_pal_end - plane_pal);
    dmaCopyCGram(orb_pal, OBJ_CGRAM_BASE + 16, orb_pal_end - orb_pal);   /* OBJ palette 1 */
    dmaCopyVram(orb_tiles, VRAM_OBJ, orb_tiles_end - orb_tiles);

    oamInit(OBJ_SIZE8_L16, VRAM_OBJ >> 13);
    orb_x = 8;
    oamSet(0, orb_x, ORB_Y, 0, 1, 1, 0);   /* palette 1, priority 1 */
    oamSetSize(0, OBJ_LARGE);

    setMode(BG_MODE7, 0);
    mode7Init();
    mode7SetScale(0x0100, 0x0100);   /* 1:1 */
    mode7SetAngle(0);
    mode7SetScroll(0, 0);            /* texel row 0 on the first line (mode7Init centres the view) */

    extbg_on = 1;
    applyExtBg();
    setScreenOn();

    while (1) {
        WaitForVBlank();
        if (padPressed(0) & KEY_A) {
            extbg_on ^= 1;
            applyExtBg();
        }
        orb_x = (u16)(orb_x + vx);
        if (orb_x >= 232 || orb_x <= 8)
            vx = (s16)-vx;
        /* eight frames of a pulse, side by side: two tiles each */
        oamSet(0, orb_x, ORB_Y, (u16)(((getFrameCount() >> 2) & 7) << 1), 1, 1, 0);
    }
    return 0;
}
