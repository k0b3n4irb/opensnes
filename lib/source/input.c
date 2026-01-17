/**
 * @file input.c
 * @brief OpenSNES Input Handling Implementation
 *
 * Controller reading with edge detection.
 *
 * Attribution:
 *   Based on PVSnesLib input handling by Alekmaul
 *   License: zlib (compatible with MIT)
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>

/*============================================================================
 * Controller State
 *============================================================================*/

/** Current button state for each controller */
static u16 pad_current[4];

/** Previous frame button state */
static u16 pad_previous[4];

/*============================================================================
 * Input Functions
 *============================================================================*/

void padUpdate(void) {
    /* Wait for auto-joypad read to complete */
    while (REG_HVBJOY & 0x01) {
        /* Busy wait */
    }

    /* Save previous state */
    pad_previous[0] = pad_current[0];
    pad_previous[1] = pad_current[1];
    pad_previous[2] = pad_current[2];
    pad_previous[3] = pad_current[3];

    /* Read current state from hardware registers */
    pad_current[0] = REG_JOY1L | (REG_JOY1H << 8);
    pad_current[1] = REG_JOY2L | (REG_JOY2H << 8);
    pad_current[2] = REG_JOY3L | (REG_JOY3H << 8);
    pad_current[3] = REG_JOY4L | (REG_JOY4H << 8);
}

u16 padPressed(u8 pad) {
    if (pad >= 4) return 0;
    u16 current = pad_current[pad];
    u16 previous = pad_previous[pad];
    /* Disconnected controller reads as $FFFF - treat as no input */
    if (current == 0xFFFF) return 0;
    /* Buttons that are down now but weren't last frame */
    return current & ~previous;
}

u16 padHeld(u8 pad) {
    if (pad >= 4) return 0;
    u16 state = pad_current[pad];
    /* Disconnected controller reads as $FFFF - treat as no input */
    if (state == 0xFFFF) return 0;
    return state;
}

u16 padReleased(u8 pad) {
    if (pad >= 4) return 0;
    u16 current = pad_current[pad];
    u16 previous = pad_previous[pad];
    /* Disconnected controller reads as $FFFF - treat as no input */
    if (previous == 0xFFFF) return 0;
    /* Buttons that were down last frame but aren't now */
    return previous & ~current;
}

u16 padRaw(u8 pad) {
    if (pad >= 4) return 0;
    return pad_current[pad];
}

u8 padIsConnected(u8 pad) {
    if (pad >= 4) return FALSE;
    /* Check if any buttons are valid (bit 0 of high byte is always 0 for valid controller) */
    /* A disconnected controller reads as $FFFF or $0000 depending on pull-ups */
    u16 state = pad_current[pad];
    return (state != 0xFFFF && state != 0x0000) ? TRUE : FALSE;
}
