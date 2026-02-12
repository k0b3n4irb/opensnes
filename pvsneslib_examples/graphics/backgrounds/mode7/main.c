/*
 * Mode 7 - PVSnesLib example ported to OpenSNES
 *
 * Demonstrates Mode 7 rotation and scaling:
 *   A button:  Rotate clockwise
 *   B button:  Rotate counter-clockwise
 *   UP:        Zoom in (increase scale)
 *   DOWN:      Zoom out (decrease scale)
 *
 * Mode 7 uses interleaved VRAM format with tilemap in low bytes
 * and tile pixels in high bytes. Loading is handled by an assembly
 * helper function in data.asm.
 */

#include <snes.h>

/* Assembly helper to load Mode 7 data with proper VRAM interleaving */
extern void asm_loadMode7Data(void);

int main(void) {
    u16 pad0;
    u8 angle = 0;
    u16 zscale = 0x0100; /* 1.0 in 8.8 fixed point */

    consoleInit();

    /* Force blank for VRAM loading */
    REG_INIDISP = 0x80;

    /* Load Mode 7 tile data, tilemap, and palette via assembly helper */
    asm_loadMode7Data();

    /* Set Mode 7 and initialize transformation */
    setMode(BG_MODE7, 0);
    mode7Init();
    mode7SetAngle(0);
    mode7SetScale(0x0100, 0x0100);

    /* Turn on display with BG1 */
    REG_TM = TM_BG1;
    setScreenOn();

    while (1) {
        pad0 = padHeld(0);

        /* Rotate clockwise with A */
        if (pad0 & KEY_A) {
            angle++;
            mode7SetAngle(angle);
        }

        /* Rotate counter-clockwise with B */
        if (pad0 & KEY_B) {
            angle--;
            mode7SetAngle(angle);
        }

        /* Zoom in with UP (scale up = smaller value = magnify) */
        if (pad0 & KEY_UP) {
            if (zscale > 0x0010)
                zscale -= 16;
            mode7SetScale(zscale, zscale);
        }

        /* Zoom out with DOWN (scale down = larger value = shrink) */
        if (pad0 & KEY_DOWN) {
            if (zscale < 0x0F00)
                zscale += 16;
            mode7SetScale(zscale, zscale);
        }

        WaitForVBlank();
    }
    return 0;
}
