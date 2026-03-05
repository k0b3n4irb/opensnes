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

/* Mouse state (populated by VBlank ISR when mouse_con != 0) */
extern u8  mouse_con;        /* Bitmask: bit 0 = port 1, bit 1 = port 2 */
extern u16 mouse_x;          /* Port 1 X displacement (sign-magnitude in low byte) */
extern u16 mouse_y;          /* Port 1 Y displacement (sign-magnitude in low byte) */
extern u16 mouse_x2;         /* Port 2 X displacement */
extern u16 mouse_y2;         /* Port 2 Y displacement */
extern u8  mouse_buttons;    /* Port 1 buttons held */
extern u8  mouse_buttons2;   /* Port 2 buttons held */
extern u8  mouse_btnsold;    /* Port 1 previous buttons */
extern u8  mouse_btnsold2;   /* Port 2 previous buttons */
extern u8  mouse_btnsdown;   /* Port 1 newly pressed */
extern u8  mouse_btnsdown2;  /* Port 2 newly pressed */
extern u8  mouse_sens;       /* Port 1 sensitivity */
extern u8  mouse_sens2;      /* Port 2 sensitivity */

/*============================================================================
 * Input Functions
 *============================================================================*/

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

/*============================================================================
 * Mouse Functions
 *============================================================================*/

/**
 * Convert sign-magnitude byte to signed value.
 * Bit 7 = sign (1 = negative), bits 6-0 = magnitude.
 */
static s16 sign_magnitude_to_signed(u8 raw) {
    s16 magnitude = (s16)(raw & 0x7F);
    if (raw & 0x80) {
        return -magnitude;
    }
    return magnitude;
}

u8 mouseInit(u8 port) {
    u8 signature;

    if (port > 1) return 0;

    /* Wait for auto-joypad to complete */
    while (REG_HVBJOY & 0x01) {}

    /* Read device signature from auto-joypad result (low nibble) */
    if (port == 0) {
        signature = REG_JOY1L & 0x0F;
    } else {
        signature = REG_JOY2L & 0x0F;
    }

    /* Mouse signature is $01 (joypad is $00) */
    if (signature != 0x01) return 0;

    /* Enable mouse reading in NMI handler */
    if (port == 0) {
        mouse_con |= 0x01;
    } else {
        mouse_con |= 0x02;
    }

    /*
     * Fix Nintendo mouse power-on bug:
     * The internal sensitivity state may not match what the mouse reports.
     * Cycling sensitivity 2 times via strobe resets it to a known state.
     * Note: We must actually read the port register (not just cast to void)
     * because some compilers may eliminate a (void) volatile read.
     */
    {
        volatile u8 dummy;
        u8 i;
        for (i = 0; i < 2; i++) {
            REG_JOYA = 0x01;   /* Strobe ON */
            if (port == 0) {
                dummy = REG_JOYA; /* Clock one bit (triggers sensitivity cycle) */
            } else {
                dummy = REG_JOYB;
            }
            REG_JOYA = 0x00;   /* Strobe OFF */
        }
        (void)dummy;           /* Suppress unused warning */
    }

    return 1;
}

u8 mouseIsConnected(u8 port) {
    if (port > 1) return 0;
    if (port == 0) {
        return (mouse_con & 0x01) ? 1 : 0;
    }
    return (mouse_con & 0x02) ? 1 : 0;
}

s16 mouseGetX(u8 port) {
    if (port == 0) {
        return sign_magnitude_to_signed((u8)mouse_x);
    } else if (port == 1) {
        return sign_magnitude_to_signed((u8)mouse_x2);
    }
    return 0;
}

s16 mouseGetY(u8 port) {
    if (port == 0) {
        return sign_magnitude_to_signed((u8)mouse_y);
    } else if (port == 1) {
        return sign_magnitude_to_signed((u8)mouse_y2);
    }
    return 0;
}

u8 mouseButtonsHeld(u8 port) {
    if (port == 0) return mouse_buttons;
    if (port == 1) return mouse_buttons2;
    return 0;
}

u8 mouseButtonsPressed(u8 port) {
    if (port == 0) return mouse_btnsdown;
    if (port == 1) return mouse_btnsdown2;
    return 0;
}

void mouseSetSensitivity(u8 port, u8 sensitivity) {
    volatile u8 dummy;
    u8 read_sens;
    u8 max_cycles;
    u8 i;

    if (port > 1) return;
    sensitivity &= 0x03;
    if (sensitivity > 2) sensitivity = 2;

    /*
     * PVSnesLib-compatible sensitivity cycling protocol:
     * Each cycle: strobe ON, read response, strobe OFF, skip 10 bits,
     * read 2 sensitivity bits, compare with target.
     * Max 4 cycles to wrap around and find target.
     */
    max_cycles = 4;
    while (max_cycles > 0) {
        /* Strobe: latch data, clock one bit, release */
        REG_JOYA = 0x01;   /* Strobe ON */
        if (port == 0) {
            dummy = REG_JOYA; /* Read response (clock) */
        } else {
            dummy = REG_JOYB;
        }
        REG_JOYA = 0x00;   /* Strobe OFF */

        /* Skip 10 bits of serial data */
        for (i = 0; i < 10; i++) {
            if (port == 0) {
                dummy = REG_JOYA;
            } else {
                dummy = REG_JOYB;
            }
        }

        /* Read 2 sensitivity bits */
        read_sens = 0;
        if (port == 0) {
            dummy = REG_JOYA;
        } else {
            dummy = REG_JOYB;
        }
        read_sens = (dummy & 0x01) << 1;

        if (port == 0) {
            dummy = REG_JOYA;
        } else {
            dummy = REG_JOYB;
        }
        read_sens |= (dummy & 0x01);

        /* Check if we reached the target */
        if (read_sens == sensitivity) break;

        max_cycles--;
    }

    (void)dummy;  /* Suppress unused warning */

    /* Update cached value */
    if (port == 0) {
        mouse_sens = sensitivity;
    } else {
        mouse_sens2 = sensitivity;
    }
}

u8 mouseGetSensitivity(u8 port) {
    if (port == 0) return mouse_sens;
    if (port == 1) return mouse_sens2;
    return 0;
}

/*============================================================================
 * Super Scope Functions
 *============================================================================*/

/* Super Scope state (populated by VBlank ISR when scope_con != 0) */
extern u8  scope_con;
extern u16 scope_sinceshot;
extern u16 scope_shoth, scope_shotv;
extern u16 scope_shothraw, scope_shotvraw;
extern u16 scope_centerh, scope_centerv;
extern u16 scope_down, scope_now, scope_held;
extern u16 scope_last;
extern u16 scope_holddelay, scope_repdelay;
extern u16 scope_tohold;

u8 scopeInit(void) {
    u16 val;

    /* Wait for auto-joypad to complete */
    while (REG_HVBJOY & 0x01) {}

    /* Read port 2 auto-joypad result (16-bit) */
    val = (u16)REG_JOY2H << 8 | (u16)REG_JOY2L;

    /* Super Scope signature: bits 0-7 all 1, bits 10-11 both 0 */
    if ((val & 0x0CFF) != 0x00FF) return 0;

    /* Enable Super Scope reading in NMI handler */
    scope_con = 1;

    /* Set default delays */
    scope_holddelay = 60;
    scope_repdelay = 20;
    scope_tohold = 60;

    /* Clear calibration */
    scope_centerh = 0;
    scope_centerv = 0;

    return 1;
}

u8 scopeIsConnected(void) {
    return scope_con;
}

u16 scopeGetX(void) {
    return scope_shoth;
}

u16 scopeGetY(void) {
    return scope_shotv;
}

u16 scopeGetRawX(void) {
    return scope_shothraw;
}

u16 scopeGetRawY(void) {
    return scope_shotvraw;
}

u16 scopeButtonsDown(void) {
    return scope_down;
}

u16 scopeButtonsPressed(void) {
    return scope_now;
}

u16 scopeButtonsHeld(void) {
    return scope_held;
}

void scopeCalibrate(void) {
    scope_centerh = 0x80 - scope_shothraw;
    scope_centerv = 0x70 - scope_shotvraw;
}

void scopeSetHoldDelay(u16 frames) {
    scope_holddelay = frames;
}

void scopeSetRepeatDelay(u16 frames) {
    scope_repdelay = frames;
}

u16 scopeSinceShot(void) {
    return scope_sinceshot;
}
