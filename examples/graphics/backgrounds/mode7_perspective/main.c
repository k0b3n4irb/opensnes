/*
 * Mode 7 Perspective - F-Zero style ground with sky
 *
 * Ported from PVSnesLib Mode7Perspective by mills32 / alekmaul
 *
 * Uses HDMA to create a split-screen effect:
 *   - Top: Mode 3 sky background (96 scanlines)
 *   - Bottom: Mode 7 ground with per-scanline perspective scaling (128 scanlines)
 *
 * Controls:
 *   D-PAD: Scroll the ground plane
 *
 * Technique:
 *   4 HDMA channels run every frame:
 *     Ch1: Switches BGMODE from Mode 3 (sky) to Mode 7 (ground)
 *     Ch2: Switches main screen enable (BG2 for sky, BG1 for ground)
 *     Ch3: Per-scanline M7A (horizontal scale) for perspective
 *     Ch4: Per-scanline M7D (vertical scale) for perspective
 */

#include <snes.h>

/* Assembly helpers */
extern void asm_loadGroundData(void);
extern void asm_loadSkyData(void);
extern void asm_setupHdmaPerspective(u16 sx, u16 sy);

u16 pad0;
u16 sx, sy;

int main(void) {
    consoleInit();

    /* Force blank for VRAM loading */
    REG_INIDISP = 0x80;

    /* Load Mode 7 ground (1024x1024, 256 colors) to VRAM $0000 + palette */
    asm_loadGroundData();

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
