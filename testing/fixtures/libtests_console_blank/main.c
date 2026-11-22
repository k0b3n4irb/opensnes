/**
 * @file main.c
 * @brief Fixture: consoleInit() tells console_force_blanked the screen is off.
 *
 * consoleInit() writes INIDISP_FORCE_BLANK. Until 2026-11-22 it left the
 * console_force_blanked shadow as it was: after a setScreenOn(), a second
 * screen set up by textModeInit() (which calls consoleInit()) had the PPU in
 * forced blank and the shadow saying "on". The DMA screen check reads the
 * shadow, so every transfer of that screen outside VBlank was counted in
 * dma_unsafe_count and fired WDM $02, with nothing wrong.
 *
 * The fixture turns the screen on, sets up a text screen with
 * textModeInit(), then sends a DMA to VRAM in active display, outside
 * VBlank: the screen is off, so nothing may be counted.
 */
#include <snes.h>

/** @brief a tile's worth of bytes for the DMA: their content does not matter */
static const u8 probe_tile[32] = { 0x5A };

extern u8 console_force_blanked;

/** @brief the shadow right after textModeInit(): 1, the screen is off */
u16 r_shadow;
/** @brief dma_unsafe_count moved by the DMA in active display: 0 */
u16 r_delta;
/** @brief 1 when that DMA started outside VBlank (the case under test) */
u16 r_outside;
/** @brief marker: the run reached its end */
u16 r_done;

int main(void) {
    u16 before;

    consoleInit();
    setMode(BG_MODE0, 0);
    setMainScreen(LAYER_BG1);
    setScreenOn();
    WaitForVBlank();
    WaitForVBlank();

    /* the second screen: the screen goes off inside consoleInit() */
    textModeInit();
    r_shadow = console_force_blanked;

    WaitForVBlank();
    while (REG_HVBJOY & 0x80) { }       /* out of VBlank: active display */
    before = dma_unsafe_count;
    r_outside = (REG_HVBJOY & 0x80) ? 0 : 1;
    dmaCopyVram(probe_tile, 0x1000, sizeof(probe_tile));
    r_delta = dma_unsafe_count - before;

    r_done = 0xC0DE;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
