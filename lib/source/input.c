/**
 * @file input.c
 * @brief OpenSNES Input Handling Implementation
 *
 * Controller reading with edge detection.
 * Input is read in the VBlank ISR (crt0.asm) for reliable, glitch-free input.
 * This file provides C wrappers to access the ISR-populated globals.
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
 * External Variables (populated by VBlank ISR in crt0.asm)
 *============================================================================*/

/* These are read in the NMI handler for reliable, glitch-free input */
extern u16 pad_keys[5];      /* Current button state (5 pads × 16 bits) */
extern u16 pad_keysold[5];   /* Previous frame button state */
extern u16 pad_keysdown[5];  /* Buttons pressed this frame (edge detection) */

/*============================================================================
 * Input Functions
 *============================================================================*/

void padUpdate(void) {
    /* Input is read in VBlank ISR - nothing to do here */
    /* This function exists for API compatibility */
}

u16 padPressed(u8 pad) {
    if (pad >= 5) return 0;
    u16 state = pad_keysdown[pad];
    /* Disconnected controller reads as $FFFF - treat as no input */
    if (pad_keys[pad] == 0xFFFF) return 0;
    return state;
}

u16 padHeld(u8 pad) {
    if (pad >= 5) return 0;
    u16 state = pad_keys[pad];
    /* Disconnected controller reads as $FFFF - treat as no input */
    if (state == 0xFFFF) return 0;
    return state;
}

u16 padReleased(u8 pad) {
    if (pad >= 5) return 0;
    u16 current = pad_keys[pad];
    u16 previous = pad_keysold[pad];
    /* Disconnected controller reads as $FFFF - treat as no input */
    if (previous == 0xFFFF) return 0;
    /* Buttons that were down last frame but aren't now */
    return previous & ~current;
}

u16 padRaw(u8 pad) {
    if (pad >= 5) return 0;
    return pad_keys[pad];
}

u8 padIsConnected(u8 pad) {
    if (pad >= 5) return FALSE;
    /* Check if any buttons are valid (bit 0 of high byte is always 0 for valid controller) */
    /* A disconnected controller reads as $FFFF or $0000 depending on pull-ups */
    u16 state = pad_keys[pad];
    return (state != 0xFFFF && state != 0x0000) ? TRUE : FALSE;
}
