/**
 * @file console.c
 * @brief OpenSNES Console Functions Implementation
 *
 * Core console initialization and VBlank synchronization.
 *
 * Attribution:
 *   Based on PVSnesLib console.c by Alekmaul
 *   License: zlib (compatible with MIT)
 *   Modifications:
 *     - Simplified implementation
 *     - Added documentation
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>

/*============================================================================
 * External Variables (defined in crt0.asm)
 *============================================================================*/

extern volatile u8 vblank_flag;
extern volatile u16 frame_count;
extern volatile u8 nmi_callback[4];     /* 24-bit function pointer + padding (PVSnesLib compatible) */
extern void DefaultNmiCallback(void);  /* Default callback in crt0.asm */

/*============================================================================
 * Static Variables
 *============================================================================*/

/** Current screen brightness (0-15) */
static u8 current_brightness;

/** PAL/NTSC flag */
static u8 is_pal_system;

/** Random seed */
static u16 rand_seed;

/*============================================================================
 * Console Initialization
 *============================================================================*/

void consoleInit(void) {
    /* Detect PAL/NTSC */
    is_pal_system = (REG_STAT78 & 0x10) ? TRUE : FALSE;

    /* Set default brightness (screen still blanked) */
    current_brightness = 15;

    /* Initialize random seed from hardware */
    rand_seed = REG_OPHCT | (REG_OPVCT << 8);
    if (rand_seed == 0) rand_seed = 0xACE1;

    /* Set up Mode 1 as default */
    REG_BGMODE = BGMODE_MODE1;

    /* Clear palettes to black */
    REG_CGADD = 0;
    u16 i;
    for (i = 0; i < 256; i++) {
        REG_CGDATA = 0;
        REG_CGDATA = 0;
    }

    /* Enable NMI (VBlank interrupt) and auto-joypad read */
    REG_NMITIMEN = NMITIMEN_NMI_ENABLE | NMITIMEN_JOY_ENABLE;
}

void consoleInitEx(u16 options) {
    /* For now, just call standard init */
    consoleInit();
    (void)options;  /* Reserved for future use */
}

/*============================================================================
 * Screen Control
 *============================================================================*/

void setScreenOn(void) {
    /* Set brightness, disable force blank */
    REG_INIDISP = current_brightness & 0x0F;
}

void setScreenOff(void) {
    /* Force blank */
    REG_INIDISP = INIDISP_FORCE_BLANK;
}

void setBrightness(u8 brightness) {
    current_brightness = brightness & 0x0F;
    /* Only update if screen is on (not force blanked) */
    if (!(REG_INIDISP & 0x80)) {
        REG_INIDISP = current_brightness;
    }
}

u8 getBrightness(void) {
    return current_brightness;
}

/*============================================================================
 * VBlank Synchronization
 *============================================================================*/

void WaitForVBlank(void) {
    /* Wait for VBlank flag to be set by NMI handler */
    while (!vblank_flag) {
        /* Idle - could use WAI instruction for power saving */
    }

    /* Clear flag for next frame */
    vblank_flag = 0;
}

u8 isInVBlank(void) {
    return (REG_HVBJOY & 0x80) ? TRUE : FALSE;
}

/*============================================================================
 * Frame Counter
 *============================================================================*/

u16 getFrameCount(void) {
    return frame_count;
}

void resetFrameCount(void) {
    frame_count = 0;
}

/*============================================================================
 * System Information
 *============================================================================*/

u8 isPAL(void) {
    return is_pal_system;
}

u8 getRegion(void) {
    return is_pal_system ? 1 : 0;
}

/*============================================================================
 * Random Number Generation
 *============================================================================*/

u16 rand(void) {
    /* 16-bit LFSR (Linear Feedback Shift Register) */
    /* Polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
    u16 bit = ((rand_seed >> 0) ^ (rand_seed >> 2) ^
               (rand_seed >> 3) ^ (rand_seed >> 5)) & 1;
    rand_seed = (rand_seed >> 1) | (bit << 15);
    return rand_seed;
}

void srand(u16 seed) {
    rand_seed = seed;
    if (rand_seed == 0) rand_seed = 0xACE1;  /* Avoid zero state */
}

/*============================================================================
 * Video Mode
 *============================================================================*/

void setMode(u8 mode) {
    REG_BGMODE = mode & 0x07;
}

/*============================================================================
 * VBlank Callback
 *============================================================================*/

void nmiSetBank(VBlankCallback callback, u8 bank) {
    /* Disable NMI during pointer write to prevent partial reads */
    REG_NMITIMEN = 0;

    /* Store 24-bit callback pointer (little-endian: addr_lo, addr_hi, bank) */
    nmi_callback[0] = (u16)callback & 0xFF;
    nmi_callback[1] = ((u16)callback >> 8) & 0xFF;
    nmi_callback[2] = bank;
    nmi_callback[3] = 0x00;  /* Padding */

    /* Clear NMI flag to prevent spurious interrupt */
    (void)REG_RDNMI;

    /* Re-enable NMI and auto-joypad */
    REG_NMITIMEN = NMITIMEN_NMI_ENABLE | NMITIMEN_JOY_ENABLE;
}

void nmiSet(VBlankCallback callback) {
    /* Default to bank 0 - works for most small/medium projects */
    nmiSetBank(callback, 0);
}

void nmiClear(void) {
    /* Restore default callback */
    nmiSetBank((VBlankCallback)DefaultNmiCallback, 0);
}
