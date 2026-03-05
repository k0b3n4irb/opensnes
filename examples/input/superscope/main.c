/**
 * @file main.c
 * @brief SNES Super Scope Demo
 *
 * Blue calibration background with crosshair target, white text overlay,
 * and red dot sprite marking fire position.
 *
 * States: DETECT -> CALIBRATE -> READY
 */

#include <snes.h>
#include <snes/input.h>
#include <snes/text.h>
#include <snes/dma.h>

/* Graphics data from data.asm */
extern u8 aim_target_tiles[], aim_target_tiles_end[];
extern u8 aim_target_map[], aim_target_map_end[];
extern u8 aim_target_pal[], aim_target_pal_end[];
extern u8 sprites_tiles[], sprites_tiles_end[];
extern u8 sprites_pal[], sprites_pal_end[];

/* oamMemory[] and oam_update_flag declared in <snes/system.h> (via <snes.h>) */

#define STATE_DETECT    0
#define STATE_CALIBRATE 1
#define STATE_READY     2

static void hideDot(void) {
    oamMemory[1] = 0xE0;
    oam_update_flag = 1;
}

static void showDot(u16 x, u16 y) {
    oamMemory[0] = (u8)x;
    oamMemory[1] = (u8)y;
    oam_update_flag = 1;
}

static void clearBottom(void) {
    textClearRect(0, 24, 32, 4);
}

int main(void) {
    u8 state;
    u8 fire_armed;
    u16 pressed, sx, sy;

    consoleInit();
    setMode(BG_MODE0, 0);

    /* --- Text system (BG1) --- */
    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* --- BG2: aim calibration target --- */
    dmaCopyVram(aim_target_tiles, 0x1000,
                (u16)(aim_target_tiles_end - aim_target_tiles));
    dmaCopyVram(aim_target_map, 0x2000,
                (u16)(aim_target_map_end - aim_target_map));
    bgSetGfxPtr(1, 0x1000);
    bgSetMapPtr(1, 0x2000, BG_MAP_32x32);

    /* --- Sprites --- */
    dmaCopyVram(sprites_tiles, 0x4000,
                (u16)(sprites_tiles_end - sprites_tiles));
    REG_OBJSEL = OBJSEL(OBJ_SIZE16_L32, 0x4000);

    /* --- Palettes --- */
    REG_CGADD = 0;
    REG_CGDATA = 0x66; REG_CGDATA = 0x7D;  /* Backdrop = blue $7D66 */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* Text = white $7FFF */

    /* BG2: all 4 sub-palettes (CGRAM 32-47) */
    dmaCopyCGram(aim_target_pal, 32, 8);
    dmaCopyCGram(aim_target_pal, 36, 8);
    dmaCopyCGram(aim_target_pal, 40, 8);
    dmaCopyCGram(aim_target_pal, 44, 8);

    /* Sprite palette (CGRAM 128+) */
    dmaCopyCGram(sprites_pal, 128,
                 (u16)(sprites_pal_end - sprites_pal));

    /* --- Scroll --- */
    bgSetScroll(0, 0, 0xFFFF);
    bgSetScroll(1, 0, 0xFFFF);

    /* --- OAM: sprite 0 = red dot, hidden --- */
    oamClear();
    oamMemory[0] = 0;
    oamMemory[1] = 0xE0;     /* Off-screen */
    oamMemory[2] = 0x80;     /* Tile 0x80 (red dot) */
    oamMemory[3] = 0x34;     /* Priority 3, palette 2 */
    oamMemory[512] = 0x00;   /* Small (16x16), X high = 0 */
    oam_update_flag = 1;

    /* --- Enable layers --- */
    REG_TM = TM_BG1 | TM_BG2 | TM_OBJ;

    /* Title (persistent across states) */
    textPrintAt(8, 1, "SUPERSCOPE Test");

    /* Initial state: detect */
    state = STATE_DETECT;
    fire_armed = 0;
    textPrintAt(7, 25, "Connect SuperScope");
    textPrintAt(11, 26, "to Port 2");
    textFlush();

    WaitForVBlank();
    setScreenOn();

    /* Main loop */
    while (1) {
        WaitForVBlank();

        switch (state) {
        case STATE_DETECT:
            scopeInit();
            if (scopeIsConnected()) {
                clearBottom();
                textPrintAt(5, 25, "Shoot center of screen");
                textPrintAt(9, 26, "to adjust aim");
                fire_armed = 0;
                state = STATE_CALIBRATE;
            }
            break;

        case STATE_CALIBRATE:
            if (!fire_armed) {
                if (!(scopeButtonsDown() & SSC_FIRE))
                    fire_armed = 1;
            } else {
                pressed = scopeButtonsPressed();
                if (pressed & SSC_FIRE) {
                    scopeCalibrate();
                    showDot(120, 104);
                    clearBottom();
                    textPrintAt(9, 24, "Are you ready?");
                    textPrintAt(3, 25, "Press PAUSE to adjust aim");
                    textPrintAt(6, 26, "Press CURSOR to play");
                    state = STATE_READY;
                }
            }
            break;

        case STATE_READY:
            pressed = scopeButtonsPressed();
            if (pressed & SSC_FIRE) {
                sx = scopeGetX();
                sy = scopeGetY();
                if (sx >= 8) sx -= 8;
                if (sy >= 8) sy -= 8;
                showDot(sx, sy);
            }
            if (pressed & SSC_PAUSE) {
                hideDot();
                clearBottom();
                textPrintAt(5, 25, "Shoot center of screen");
                textPrintAt(9, 26, "to adjust aim");
                fire_armed = 0;
                state = STATE_CALIBRATE;
            }
            break;
        }

        textFlush();
    }

    return 0;
}
