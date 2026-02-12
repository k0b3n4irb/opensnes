/*
 * waves - Ported from PVSnesLib to OpenSNES
 *
 * Simple gradient color effect in mode 1 with wave distortion
 * -- alekmaul (original PVSnesLib example)
 *
 * Demonstrates HDMA-driven horizontal wave effect on BG1.
 * Press A to stop the wave, B to restart it.
 *
 * API differences from PVSnesLib:
 *  - setModeHdmaWaves(0)     -> hdmaWaveInit() + hdmaWaveH(ch, bg, amp, freq)
 *  - setModeHdmaWavesMove()  -> hdmaWaveUpdate()
 *  - setModeHdmaReset(0x00)  -> hdmaWaveStop()
 *  - padsCurrent(0)          -> padHeld(0)
 *  - bgInitMapSet()          -> dmaCopyVram()
 *  - bgSetDisable()          -> REG_TM
 *
 * The wave effect uses HDMA to modify the BG1 horizontal scroll register
 * (BG1HOFS $210D) every scanline with a sinusoidal offset that animates
 * over time. hdmaWaveUpdate() advances the wave phase each frame.
 */

#include <snes.h>
#include <snes/hdma.h>

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

int main(void) {
    u8 pada = 0, padb = 0;
    u16 pad0;

    consoleInit();

    /* Load BG1 tiles at VRAM $4000, palette slot 0 */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x4000);

    /* Load tilemap at VRAM $0000 */
    bgSetMapPtr(0, 0x0000, BG_MAP_32x32);
    dmaCopyVram(tilemap, 0x0000, tilemap_end - tilemap);

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;
    setScreenOn();

    /* Start wave effect: channel 6, BG1 (bg=0), amplitude 8, frequency 4 */
    hdmaWaveInit();
    hdmaWaveH(HDMA_CHANNEL_6, 0, 8, 4);

    while (1) {
        pad0 = padHeld(0);

        /* A = stop waves */
        if (pad0 & KEY_A) {
            if (!pada) {
                pada = 1;
                hdmaWaveStop();
            }
        } else {
            pada = 0;
        }

        /* B = restart waves */
        if (pad0 & KEY_B) {
            if (!padb) {
                padb = 1;
                hdmaWaveInit();
                hdmaWaveH(HDMA_CHANNEL_6, 0, 8, 4);
            }
        } else {
            padb = 0;
        }

        /* Update wave animation (internally throttled, safe to call every frame) */
        hdmaWaveUpdate();

        WaitForVBlank();
    }

    return 0;
}
