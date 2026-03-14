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

/* Mouse state — PVSnesLib-compatible indexed layout.
 * All 2-byte arrays: [0] = port 1, [1] = port 2.
 * NMI handler detects connection per-frame and handles sensitivity sync. */
extern u8  mouse_con;          /* Bitmask: bit 0 = port 1, bit 1 = port 2 */
extern u8  mouse_x[2];        /* X displacement per port (sign-magnitude) */
extern u8  mouse_y[2];        /* Y displacement per port (sign-magnitude) */
extern u8  mouseConnect[2];   /* Per-port connection flag (set by NMI handler) */
extern u8  mousePressed[2];   /* Currently held buttons per port */
extern u8  mousePreviousPressed[2]; /* Previous frame buttons per port */
extern u8  mouseButton[2];    /* Newly pressed buttons per port */
extern u8  mouseSensitivity[2]; /* Current sensitivity per port (0-2) */
extern u8  mouseRequestChangeSensitivity[2]; /* Deferred sensitivity command */

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

    /* Enable mouse reading in NMI handler.
     * The NMI handler will detect the connection per-frame, automatically
     * sync sensitivity on first connection (fixing the Nintendo mouse
     * power-on bug), and process deferred sensitivity change commands. */
    if (port == 0) {
        mouse_con |= 0x01;
    } else {
        mouse_con |= 0x02;
    }

    return 1;
}

u8 mouseIsConnected(u8 port) {
    if (port > 1) return 0;
    return mouseConnect[port] ? 1 : 0;
}

s16 mouseGetX(u8 port) {
    if (port > 1) return 0;
    return sign_magnitude_to_signed(mouse_x[port]);
}

s16 mouseGetY(u8 port) {
    if (port > 1) return 0;
    return sign_magnitude_to_signed(mouse_y[port]);
}

u8 mouseButtonsHeld(u8 port) {
    if (port > 1) return 0;
    return mousePressed[port];
}

u8 mouseButtonsPressed(u8 port) {
    if (port > 1) return 0;
    return mouseButton[port];
}

void mouseSetSensitivity(u8 port, u8 sensitivity) {
    if (port > 1) return;
    sensitivity &= 0x03;
    if (sensitivity > 2) sensitivity = 2;

    /* Deferred command: NMI handler will cycle hardware sensitivity.
     * High bit set = "set to specific sensitivity (value & 3)".
     * Protocol: $01=cycle once, $02+=cycle twice, $80+=set to (value & 3). */
    mouseRequestChangeSensitivity[port] = 0x80 | sensitivity;
}

u8 mouseGetSensitivity(u8 port) {
    if (port > 1) return 0;
    return mouseSensitivity[port];
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
