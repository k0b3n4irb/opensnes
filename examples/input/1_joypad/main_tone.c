/**
 * @file main.c
 * @brief Tada Sound Demo - Press A to Play
 *
 * Demonstrates SPC700 audio playback with button control.
 * Press the A button to play the "tada" BRR sample.
 *
 * SPC Communication Protocol:
 * - SPC continuously echoes port0 values back
 * - Write 0x55 to trigger sound playback
 * - Wait for echo, then clear to 0x00
 *
 * @author OpenSNES Team
 * @license CC0 (Public Domain)
 */

typedef unsigned char u8;
typedef unsigned short u16;

/*============================================================================
 * Hardware Registers
 *============================================================================*/

/* PPU Registers */
#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_BGMODE   (*(volatile u8*)0x2105)
#define REG_BG1SC    (*(volatile u8*)0x2107)
#define REG_BG12NBA  (*(volatile u8*)0x210B)
#define REG_VMAIN    (*(volatile u8*)0x2115)
#define REG_VMADDL   (*(volatile u8*)0x2116)
#define REG_VMADDH   (*(volatile u8*)0x2117)
#define REG_VMDATAL  (*(volatile u8*)0x2118)
#define REG_VMDATAH  (*(volatile u8*)0x2119)
#define REG_CGADD    (*(volatile u8*)0x2121)
#define REG_CGDATA   (*(volatile u8*)0x2122)
#define REG_TM       (*(volatile u8*)0x212C)

/* APU Communication */
#define REG_APUIO0   (*(volatile u8*)0x2140)

/* System Registers */
#define REG_NMITIMEN (*(volatile u8*)0x4200)
#define REG_HVBJOY   (*(volatile u8*)0x4212)
#define REG_JOY1L    (*(volatile u8*)0x4218)
#define REG_JOY1H    (*(volatile u8*)0x4219)

/* Joypad Button Masks */
#define JOY_A        0x0080

/* SPC Commands */
#define SPC_CMD_PLAY 0x55

/*============================================================================
 * SPC Driver
 *
 * Memory Map:
 *   $0200 - Driver code
 *   $0300 - Sample directory
 *   $0400 - BRR sample data
 *
 * Protocol:
 *   - Driver echoes all port0 values back to port0
 *   - When port0 == 0x55, triggers voice 0 key-on
 *   - Small delay after key-on prevents re-triggering
 *============================================================================*/

/* Sample directory: points to BRR data at $0400 */
static const u8 sample_dir[] = {
    0x00, 0x04,  /* Sample 0 start: $0400 */
    0x00, 0x04,  /* Sample 0 loop:  $0400 */
};

static const u8 spc_driver[] = {
    /*--------------------------------------------------
     * DSP Initialization
     *--------------------------------------------------*/

    /* FLG: Disable mute, enable echo write */
    0x8F, 0x6C, 0xF2,       /* mov $F2, #$6C */
    0x8F, 0x20, 0xF3,       /* mov $F3, #$20 */

    /* DIR: Sample directory at $0300 */
    0x8F, 0x5D, 0xF2,       /* mov $F2, #$5D */
    0x8F, 0x03, 0xF3,       /* mov $F3, #$03 */

    /* KON: All voices off initially */
    0x8F, 0x4C, 0xF2,       /* mov $F2, #$4C */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00 */

    /* Voice 0: Left volume */
    0x8F, 0x00, 0xF2,       /* mov $F2, #$00 (V0VOLL) */
    0x8F, 0x7F, 0xF3,       /* mov $F3, #$7F */

    /* Voice 0: Right volume */
    0x8F, 0x01, 0xF2,       /* mov $F2, #$01 (V0VOLR) */
    0x8F, 0x7F, 0xF3,       /* mov $F3, #$7F */

    /* Voice 0: Pitch $0440 (tuned for tada.brr at 32kHz) */
    0x8F, 0x02, 0xF2,       /* mov $F2, #$02 (V0PITCHL) */
    0x8F, 0x40, 0xF3,       /* mov $F3, #$40 */
    0x8F, 0x03, 0xF2,       /* mov $F2, #$03 (V0PITCHH) */
    0x8F, 0x04, 0xF3,       /* mov $F3, #$04 */

    /* Voice 0: Sample source */
    0x8F, 0x04, 0xF2,       /* mov $F2, #$04 (V0SRCN) */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00 */

    /* Voice 0: ADSR envelope */
    0x8F, 0x05, 0xF2,       /* mov $F2, #$05 (V0ADSR1) */
    0x8F, 0xFF, 0xF3,       /* mov $F3, #$FF */
    0x8F, 0x06, 0xF2,       /* mov $F2, #$06 (V0ADSR2) */
    0x8F, 0xE0, 0xF3,       /* mov $F3, #$E0 */
    0x8F, 0x07, 0xF2,       /* mov $F2, #$07 (V0GAIN) */
    0x8F, 0x7F, 0xF3,       /* mov $F3, #$7F */

    /* Master volume */
    0x8F, 0x0C, 0xF2,       /* mov $F2, #$0C (MVOLL) */
    0x8F, 0x7F, 0xF3,       /* mov $F3, #$7F */
    0x8F, 0x1C, 0xF2,       /* mov $F2, #$1C (MVOLR) */
    0x8F, 0x7F, 0xF3,       /* mov $F3, #$7F */

    /* Disable pitch modulation, noise, echo */
    0x8F, 0x2D, 0xF2,       /* mov $F2, #$2D (PMON) */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00 */
    0x8F, 0x3D, 0xF2,       /* mov $F2, #$3D (NON) */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00 */
    0x8F, 0x4D, 0xF2,       /* mov $F2, #$4D (EON) */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00 */

    /* Echo configuration (disabled) */
    0x8F, 0x6D, 0xF2,       /* mov $F2, #$6D (ESA) */
    0x8F, 0x60, 0xF3,       /* mov $F3, #$60 */
    0x8F, 0x7D, 0xF2,       /* mov $F2, #$7D (EDL) */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00 */
    0x8F, 0x2C, 0xF2,       /* mov $F2, #$2C (EVOLL) */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00 */
    0x8F, 0x3C, 0xF2,       /* mov $F2, #$3C (EVOLR) */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00 */

    /* Key off all voices */
    0x8F, 0x5C, 0xF2,       /* mov $F2, #$5C (KOF) */
    0x8F, 0xFF, 0xF3,       /* mov $F3, #$FF */

    /* Small delay for key-off to take effect */
    0xCD, 0x10,             /* mov X, #$10 */
    0x8D, 0x00,             /* mov Y, #$00 */
    0xDC,                   /* dec Y */
    0xD0, 0xFD,             /* bne -3 */
    0x1D,                   /* dec X */
    0xD0, 0xF8,             /* bne -8 */

    /* Clear key-off */
    0x8F, 0x5C, 0xF2,       /* mov $F2, #$5C (KOF) */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00 */

    /*--------------------------------------------------
     * Main Loop: Echo port0, play on 0x55
     *--------------------------------------------------*/

    /* @loop: */
    0xE4, 0xF4,             /* mov A, $F4       ; read port0 */
    0xC4, 0xF4,             /* mov $F4, A       ; echo it back */
    0x68, 0x55,             /* cmp A, #$55      ; is it play command? */
    0xD0, 0xF8,             /* bne @loop (-8)   ; no, keep looping */

    /* Play command received - key on voice 0 */
    0x8F, 0x4C, 0xF2,       /* mov $F2, #$4C (KON) */
    0x8F, 0x01, 0xF3,       /* mov $F3, #$01 */

    /* Delay to prevent immediate re-trigger */
    0xCD, 0xFF,             /* mov X, #$FF */
    0x1D,                   /* dec X */
    0xD0, 0xFD,             /* bne -3 */

    /* Return to main loop */
    0x2F, 0xEB,             /* bra @loop (-21) */
};

/*============================================================================
 * External Data (from data.asm)
 *============================================================================*/

extern u8 tada_brr_start;

/*============================================================================
 * External SPC Functions (from crt0.asm)
 *============================================================================*/

extern void spc_wait_ready(void);
extern void spc_upload(u16 addr, const u8 *data, u16 size);
extern void spc_execute(u16 addr);

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void wait_vblank(void) {
    while (REG_HVBJOY & 0x80) {}
    while (!(REG_HVBJOY & 0x80)) {}
}

/*============================================================================
 * Display Setup
 *============================================================================*/

/* Simple font: space, !, T, A, D */
static const u8 font_tiles[] = {
    /* Tile 0: Space */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Tile 1: ! */
    0x7E, 0x00, 0x18, 0x00, 0x18, 0x00, 0x18, 0x00,
    0x18, 0x00, 0x18, 0x00, 0x18, 0x00, 0x00, 0x00,
    /* Tile 2: T */
    0x18, 0x00, 0x3C, 0x00, 0x66, 0x00, 0x7E, 0x00,
    0x66, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00,
    /* Tile 3: A */
    0x7C, 0x00, 0x66, 0x00, 0x66, 0x00, 0x66, 0x00,
    0x66, 0x00, 0x66, 0x00, 0x7C, 0x00, 0x00, 0x00,
    /* Tile 4: D */
    0x18, 0x00, 0x18, 0x00, 0x18, 0x00, 0x18, 0x00,
    0x18, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
};

/* Message: "TADA!" using tile indices */
static const u8 msg_tada[] = { 1, 2, 3, 2, 4, 0xFF };

static void setup_display(void) {
    u16 i;

    /* Mode 0: 4 BG layers, 4 colors each */
    REG_BGMODE = 0x00;

    /* BG1 tilemap at $0400, 32x32 */
    REG_BG1SC = 0x04;

    /* BG1 tiles at $0000 */
    REG_BG12NBA = 0x00;

    /* Upload font tiles to VRAM $0000 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;
    for (i = 0; i < 80; i += 2) {
        REG_VMDATAL = font_tiles[i];
        REG_VMDATAH = font_tiles[i + 1];
    }

    /* Set palette: color 0 = black, color 1 = white */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Black */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* White */

    /* Clear tilemap */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0;
    }

    /* Write "TADA!" at center of screen */
    REG_VMADDL = 0xCD;
    REG_VMADDH = 0x05;
    for (i = 0; msg_tada[i] != 0xFF; i++) {
        REG_VMDATAL = msg_tada[i];
        REG_VMDATAH = 0;
    }

    /* Enable BG1 */
    REG_TM = 0x01;
}

/*============================================================================
 * Main Program
 *============================================================================*/

int main(void) {
    u8 a_was_pressed = 0;
    u16 joy;

    /* Initialize SPC700 */
    spc_wait_ready();

    /* Upload driver code to $0200 */
    spc_upload(0x0200, spc_driver, sizeof(spc_driver));

    /* Upload sample directory to $0300 */
    spc_upload(0x0300, sample_dir, sizeof(sample_dir));

    /* Upload BRR sample to $0400 */
    spc_upload(0x0400, &tada_brr_start, 8739);

    /* Start driver execution */
    spc_execute(0x0200);

    /* Synchronize with SPC - wait for echo */
    REG_APUIO0 = 0x00;
    while (REG_APUIO0 != 0x00) {}

    /* Set up display */
    setup_display();

    /* Enable NMI and auto-joypad reading */
    REG_NMITIMEN = 0x81;

    /* Turn on screen at full brightness */
    REG_INIDISP = 0x0F;

    /* Main loop: press A to play sound */
    while (1) {
        wait_vblank();

        /* Wait for auto-joypad read to complete */
        while (REG_HVBJOY & 0x01) {}

        /* Read joypad 1 */
        joy = REG_JOY1L | (REG_JOY1H << 8);

        /* Detect A button press (rising edge only) */
        if ((joy & JOY_A) && !a_was_pressed) {
            a_was_pressed = 1;

            /* Send play command to SPC */
            REG_APUIO0 = SPC_CMD_PLAY;

            /* Wait for SPC to acknowledge */
            while (REG_APUIO0 != SPC_CMD_PLAY) {}

            /* Clear command */
            REG_APUIO0 = 0x00;
            while (REG_APUIO0 != 0x00) {}

        } else if (!(joy & JOY_A)) {
            a_was_pressed = 0;
        }
    }

    return 0;
}
