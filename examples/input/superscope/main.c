/**
 * @file main.c
 * @brief SNES Super Scope light gun detection, calibration, and firing
 * @ingroup examples
 *
 * Demonstrates the SNES Super Scope (light gun) peripheral connected to
 * controller port 2. The Super Scope works by detecting the CRT beam
 * position when the trigger is pulled, returning X/Y screen coordinates
 * via the auto-joypad registers. Because the sensor relies on CRT timing,
 * a calibration step is needed to correct for aiming offset.
 *
 * The example uses a three-state machine: DETECT waits for the Super Scope
 * to be connected, CALIBRATE asks the player to shoot the screen center
 * so the library can compute the aim offset, and READY tracks fire events
 * and marks the hit position with a red dot sprite.
 *
 * Video setup: Mode 0 with BG1 (text overlay) and BG2 (crosshair target
 * background for calibration), plus OBJ sprites for the red dot marker.
 *
 * @par SNES Concepts
 * - Super Scope peripheral detection and calibration flow
 * - Light gun coordinate reading (scopeGetX / scopeGetY)
 * - Button polling for FIRE, PAUSE, and CURSOR buttons
 * - Multi-layer Mode 0 with text on BG1 and graphics on BG2
 * - Sprite positioning from light gun coordinates
 *
 * @par What to Observe
 * - "Connect SuperScope to Port 2" message on startup
 * - After connection, a crosshair target for calibration
 * - Shooting places a red dot at the hit position
 * - PAUSE button returns to calibration; CURSOR transitions to play
 *
 * @par Modules Used
 * console, input, sprite, dma, text, background
 *
 * @see input.h, sprite.h, dma.h, text.h
 */

#include <snes.h>
#include <snes/input.h>
#include <snes/text.h>
#include <snes/dma.h>

/** @brief Aim target background tile data (2bpp, crosshair pattern) from data.asm */
extern u8 aim_target_tiles[], aim_target_tiles_end[];
/** @brief Aim target tilemap (32x32 BG2 layout) from data.asm */
extern u8 aim_target_map[], aim_target_map_end[];
/** @brief Aim target palette (BGR555) from data.asm */
extern u8 aim_target_pal[], aim_target_pal_end[];
/** @brief Sprite tile data (red dot marker, 4bpp) from data.asm */
extern u8 sprites_tiles[], sprites_tiles_end[];
/** @brief Sprite palette (BGR555) from data.asm */
extern u8 sprites_pal[], sprites_pal_end[];

/* oamMemory[] and oam_update_flag declared in <snes/system.h> (via <snes.h>) */

/** @brief Waiting for Super Scope to be connected to port 2 */
#define STATE_DETECT    0
/** @brief Asking player to shoot screen center for aim offset calibration */
#define STATE_CALIBRATE 1
/** @brief Calibrated and tracking fire events; placing red dot markers */
#define STATE_READY     2

/**
 * @brief Hide the red dot marker sprite by moving it off-screen.
 *
 * Y coordinate 0xE0 (224) is below the NTSC visible area, so the
 * SNES PPU will not render this sprite. This is cheaper than disabling
 * the sprite via the OAM high table.
 */
static void hideDot(void) {
    oamMemory[1] = 0xE0;
    oam_update_flag = 1;
}

/**
 * @brief Show the red dot marker at the given screen coordinates.
 *
 * Updates sprite 0's X/Y in the OAM low table and signals the NMI
 * handler to DMA the OAM buffer to the PPU next VBlank.
 *
 * @param x Screen X coordinate (0-255)
 * @param y Screen Y coordinate (0-223 for visible area)
 */
static void showDot(u16 x, u16 y) {
    oamMemory[0] = (u8)x;
    oamMemory[1] = (u8)y;
    oam_update_flag = 1;
}

/**
 * @brief Clear the bottom 4 rows of the text overlay (rows 24-27).
 *
 * Used to erase status messages when transitioning between states
 * (detect, calibrate, ready).
 */
static void clearBottom(void) {
    textClearRect(0, 24, 32, 4);
}

/**
 * @brief Main entry point -- Super Scope detection, calibration, and fire tracking.
 *
 * Implements a three-state machine:
 * - STATE_DETECT: polls scopeInit() / scopeIsConnected() each frame until
 *   the Super Scope is detected on controller port 2.
 * - STATE_CALIBRATE: displays a crosshair target on BG2 and waits for the
 *   player to shoot the center. scopeCalibrate() records the aim offset
 *   so subsequent coordinate readings are accurate.
 * - STATE_READY: tracks FIRE events, reads calibrated X/Y via scopeGetX()
 *   / scopeGetY(), and places a red dot sprite at the hit position. The
 *   PAUSE button returns to calibration for re-aiming.
 *
 * The fire_armed flag prevents stale FIRE events from the previous state
 * from triggering immediately on state entry.
 *
 * @return 0 (never reached -- infinite game loop)
 */
int main(void) {
    u8 state;
    u8 fire_armed;
    u16 pressed, sx, sy;

    textModeInit();

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
