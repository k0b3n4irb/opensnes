/**
 * @file main.c
 * @brief SNES mouse input with cursor sprite, buttons, and sensitivity
 * @ingroup examples
 *
 * Demonstrates the SNES Mouse peripheral connected to controller port 1.
 * The SNES mouse is a standard serial device that reports relative X/Y
 * deltas and two buttons each frame via the auto-joypad read registers.
 * This example accumulates the deltas into absolute screen coordinates
 * and drives a 16x16 hardware sprite as a cursor.
 *
 * The mouse supports three sensitivity levels (low, medium, high) that
 * scale the delta values. Sensitivity is toggled by right-clicking.
 * If no mouse is detected at startup, a diagnostic message is shown.
 *
 * @par SNES Concepts
 * - Mouse peripheral detection and initialization via auto-joypad
 * - Relative-to-absolute coordinate accumulation from mouse deltas
 * - Sensitivity control (hardware-level scaling of mouse movement)
 * - Sprite used as a cursor overlay on a text background
 *
 * @par What to Observe
 * - A 16x16 cursor sprite follows mouse movement on screen
 * - Left click changes the background color to blue
 * - Right click cycles sensitivity (LOW / MED / HIGH)
 * - Button states ("PRESSED") appear next to L-click / R-click labels
 * - Without a mouse connected, a "No mouse detected" message appears
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

/** @brief Cursor sprite tile data (16x16, 4bpp) defined in data.asm */
extern u8 cursor_tiles[], cursor_tiles_end[];
/** @brief Cursor sprite palette (SNES BGR555 format) defined in data.asm */
extern u8 cursor_pal[], cursor_pal_end[];

/* oamMemory[] and oam_update_flag declared in <snes/system.h> (via <snes.h>) */

/**
 * @brief Absolute cursor X position on screen.
 *
 * Accumulated from relative mouse X deltas each frame. The SNES mouse
 * reports displacement, not position, so we must integrate the deltas
 * ourselves and clamp to the visible screen range (0-255).
 */
static s16 pos_x;

/**
 * @brief Absolute cursor Y position on screen.
 *
 * Same accumulation strategy as pos_x, clamped to 0-223 (NTSC visible).
 */
static s16 pos_y;

/** @brief Sensitivity label: low speed (smallest deltas per physical movement) */
static const char sens_low[] = "LOW ";
/** @brief Sensitivity label: medium speed */
static const char sens_med[] = "MED ";
/** @brief Sensitivity label: high speed (largest deltas per physical movement) */
static const char sens_hi[] = "HIGH";

/**
 * @brief Main entry point -- mouse detection, initialization, and input loop.
 *
 * Sets up Mode 0 with a text background for status display and a 16x16
 * hardware sprite as a mouse cursor. The mouse is detected via mouseInit()
 * on port 1. If absent, the program halts with a diagnostic message.
 * Otherwise, the main loop reads relative mouse deltas each frame,
 * accumulates them into absolute screen coordinates, and updates the
 * cursor sprite position and button/sensitivity status on screen.
 *
 * @return 0 (never reached -- infinite game loop)
 */
int main(void) {
    u8 detected;

    textModeInit();

    textPrintAt(2, 1, "SNES MOUSE DEMO");
    textPrintAt(2, 2, "---------------");

    /* Ensure at least one auto-joypad cycle has completed before detection */
    WaitForVBlank();

    /* Try to detect mouse on port 1 */
    detected = mouseInit(0);

    if (!detected) {
        textPrintAt(2, 5, "No mouse detected.");
        textPrintAt(2, 7, "Connect SNES mouse");
        textPrintAt(2, 8, "to port 1 and reset.");
        setScreenOn();
        while (1) {
            WaitForVBlank();
        }
    }

    textPrintAt(2, 4, "Mouse detected!");
    textPrintAt(2, 6, "L-click:");
    textPrintAt(2, 7, "R-click:");
    textPrintAt(2, 9, "Sens:");

    /* Load cursor sprite tiles to VRAM.
     * A 16x16 sprite uses tiles 0,1 (top) and 16,17 (bottom) in the
     * sprite character page. gfx4snes outputs tiles sequentially (0,1,2,3)
     * so we split the DMA: top half → tile 0, bottom half → tile 16. */
    dmaCopyVram(cursor_tiles, 0x4000, 64);          /* tiles 0-1 (top) */
    dmaCopyVram(cursor_tiles + 64, 0x4100, 64);     /* tiles 16-17 (bottom) */

    /* Load cursor palette to CGRAM (sprite palette 0 = color 128) */
    dmaCopyCGram(cursor_pal, OBJ_CGRAM_BASE,
                 (u16)(cursor_pal_end - cursor_pal));

    /* OBJSEL: base address $4000 word addr = N*$2000, N=2 */
    REG_OBJSEL = OBJSEL(OBJ_SIZE8_L16, 0x4000);

    /* Enable BG1 (text) and sprites on main screen */
    setMainScreen(TM_BG1 | TM_OBJ);

    /* Clear all sprites (hide garbage) then set up sprite 0 as cursor */
    oamClear();

    /* Initialize cursor position at screen center */
    pos_x = 128;
    pos_y = 112;
    oamMemory[0] = (u8)pos_x;
    oamMemory[1] = (u8)(pos_y - 1);  /* PPU +1 scanline quirk: write Y-1 */
    oamMemory[2] = 0x00;         /* Tile number low */
    oamMemory[3] = 0x30;         /* priority 3, palette 0, no flip */
    oamMemory[512] = 0x02;       /* Large sprite (16x16), X high = 0 */
    oam_update_flag = 1;

    setScreenOn();

    while (1) {
        WaitForVBlank();

        /* Read mouse displacement and accumulate */
        pos_x += mouseGetX(0);
        pos_y += mouseGetY(0);

        /* Clamp to screen bounds */
        if (pos_x < 0) pos_x = 0;
        if (pos_x > 255) pos_x = 255;
        if (pos_y < 0) pos_y = 0;
        if (pos_y > 223) pos_y = 223;

        /* Update cursor sprite position (PPU +1 quirk: write Y-1) */
        oamMemory[0] = (u8)pos_x;
        oamMemory[1] = (u8)(pos_y - 1);
        oam_update_flag = 1;

        /* Show button state: HELD or blank */
        {
            u8 btns = mouseButtonsHeld(0);
            if (btns & MOUSE_BUTTON_LEFT)
                textPrintAt(11, 6, "PRESSED");
            else
                textPrintAt(11, 6, "       ");

            if (btns & MOUSE_BUTTON_RIGHT)
                textPrintAt(11, 7, "PRESSED");
            else
                textPrintAt(11, 7, "       ");
        }

        /* Left click: change BG color to blue */
        if (mouseButtonsPressed(0) & MOUSE_BUTTON_LEFT) {
            setColor(0, RGB(0, 0, 31));  /* Blue */
        }

        /* Right click: cycle sensitivity */
        if (mouseButtonsPressed(0) & MOUSE_BUTTON_RIGHT) {
            u8 sens = mouseGetSensitivity(0);
            sens++;
            if (sens > 2) sens = 0;
            mouseSetSensitivity(0, sens);
        }

        /* Display sensitivity */
        {
            u8 s = mouseGetSensitivity(0);
            if (s == 0) textPrintAt(8, 9, sens_low);
            else if (s == 1) textPrintAt(8, 9, sens_med);
            else textPrintAt(8, 9, sens_hi);
        }
    }

    return 0;
}
