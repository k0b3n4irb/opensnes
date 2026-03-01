/**
 * @file main.c
 * @brief SNES Mouse Demo
 *
 * Demonstrates SNES mouse input: cursor movement, button detection,
 * and sensitivity control.
 *
 * Controls (mouse on port 1):
 *   - Move mouse: cursor position displayed as text
 *   - Left click: cycle background color
 *   - Right click: cycle sensitivity
 *
 * If no mouse is detected, a message is displayed.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/input.h>
#include <snes/text.h>
#include <snes/dma.h>

/* Graphics data defined in data.asm */
extern u8 cursor_tiles[], cursor_tiles_end[];
extern u8 cursor_pal[], cursor_pal_end[];

/* Direct OAM buffer access for performance */
extern u8 oamMemory[];
extern volatile u8 oam_update_flag;

/* Cursor position (accumulated from mouse deltas) */
static s16 pos_x;
static s16 pos_y;

/* Sensitivity names as individual strings (avoid pointer array stride issue) */
static const char sens_low[] = "LOW ";
static const char sens_med[] = "MED ";
static const char sens_hi[] = "HIGH";

int main(void) {
    u8 detected;

    consoleInit();
    setMode(BG_MODE0, 0);

    /* Text system: load font, set BG pointers, set palette */
    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* BG color = black */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* Text color = white */

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
        textFlush();
        setScreenOn();
        while (1) {
            WaitForVBlank();
        }
    }

    textPrintAt(2, 4, "Mouse detected!");
    textPrintAt(2, 6, "L-click:");
    textPrintAt(2, 7, "R-click:");
    textPrintAt(2, 9, "Sens:");
    textFlush();

    /* Load cursor sprite tiles to VRAM.
     * A 16x16 sprite uses tiles 0,1 (top) and 16,17 (bottom) in the
     * sprite character page. gfx4snes outputs tiles sequentially (0,1,2,3)
     * so we split the DMA: top half → tile 0, bottom half → tile 16. */
    dmaCopyVram(cursor_tiles, 0x4000, 64);          /* tiles 0-1 (top) */
    dmaCopyVram(cursor_tiles + 64, 0x4100, 64);     /* tiles 16-17 (bottom) */

    /* Load cursor palette to CGRAM (sprite palette 0 = color 128) */
    dmaCopyCGram(cursor_pal, 128,
                 (u16)(cursor_pal_end - cursor_pal));

    /* OBJSEL: base address $4000 word addr = N*$2000, N=2 */
    REG_OBJSEL = OBJSEL(OBJ_SIZE8_L16, 0x4000);

    /* Enable BG1 (text) and sprites on main screen */
    REG_TM = TM_BG1 | TM_OBJ;

    /* Clear all sprites (hide garbage) then set up sprite 0 as cursor */
    oamClear();

    /* Initialize cursor position at screen center */
    pos_x = 128;
    pos_y = 112;
    oamMemory[0] = (u8)pos_x;
    oamMemory[1] = (u8)pos_y;
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

        /* Update cursor sprite position */
        oamMemory[0] = (u8)pos_x;
        oamMemory[1] = (u8)pos_y;
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
            REG_CGADD = 0;
            REG_CGDATA = 0x00;  /* Blue low (RRRRR GGG) */
            REG_CGDATA = 0x7C;  /* Blue high (0BBBBB GG) = B=31 */
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
        textFlush();
    }

    return 0;
}
