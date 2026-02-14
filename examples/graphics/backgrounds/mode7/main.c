/**
 * @file main.c
 * @brief Mode 7 Rotation/Scaling Demo
 *
 * Demonstrates SNES Mode 7 - the hardware rotation and scaling mode
 * made famous by F-Zero, Super Mario Kart, and Pilotwings.
 *
 * Controls:
 *   A/B      - Rotate left/right
 *   Up/Down  - Zoom in/out
 */

#include <snes.h>

/* External function from data.asm */
extern void load_mode7_vram(void);

int main(void) {
    u8 angle = 0;
    u16 scale = 0x0100;  /* 1.0 scale */
    u16 pad;

    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Load Mode 7 graphics */
    load_mode7_vram();

    /* Initialize Mode 7 (from library) */
    mode7Init();

    /* Set Mode 7 */
    REG_BGMODE = BGMODE_MODE7;

    /* Mode 7 settings: no flip, wrap around */
    REG_M7SEL = 0x00;

    /* Enable BG1 on main screen (Mode 7 only uses BG1) */
    REG_TM = TM_BG1;

    /* Set initial scale and rotation */
    mode7SetScale(scale, scale);
    mode7SetAngle(angle);

    /* Enable display at full brightness */
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Wait for auto-joypad read to complete */
        while (REG_HVBJOY & 0x01) { }

        /* Read controller - use 16-bit read directly */
        pad = *((vu16*)0x4218);

        /* Skip if disconnected (reads as $FFFF) or no buttons */
        if (pad == 0xFFFF || pad == 0) {
            continue;
        }

        /* A button: rotate clockwise */
        if (pad & KEY_A) {
            angle++;
            mode7SetAngle(angle);
        }

        /* B button: rotate counter-clockwise */
        if (pad & KEY_B) {
            angle--;
            mode7SetAngle(angle);
        }

        /* Up: zoom in (smaller scale = closer view) */
        if (pad & KEY_UP) {
            if (scale > 0x40) {
                scale -= 8;
                mode7SetScale(scale, scale);
                mode7SetAngle(angle);
            }
        }

        /* Down: zoom out (larger scale = farther view) */
        if (pad & KEY_DOWN) {
            if (scale < 0x400) {
                scale += 8;
                mode7SetScale(scale, scale);
                mode7SetAngle(angle);
            }
        }
    }

    return 0;
}
