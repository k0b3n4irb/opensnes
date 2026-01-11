/**
 * @file main.c
 * @brief Tone Generator Example
 *
 * Generates a simple square wave tone using the SNES audio DSP.
 * Demonstrates SPC700 communication and basic audio setup.
 *
 * Press A to change pitch, B to toggle sound on/off.
 *
 * Technical notes:
 *   - SNES audio uses the SPC700 coprocessor
 *   - Communication via ports $2140-$2143 (APUIO0-3)
 *   - This example uploads a minimal driver that generates a tone
 *
 * @author OpenSNES Team
 * @license CC0 (Public Domain)
 */

typedef unsigned char u8;
typedef unsigned short u16;

/*============================================================================
 * Hardware Registers
 *============================================================================*/

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
#define REG_NMITIMEN (*(volatile u8*)0x4200)
#define REG_HVBJOY   (*(volatile u8*)0x4212)
#define REG_JOY1L    (*(volatile u8*)0x4218)
#define REG_JOY1H    (*(volatile u8*)0x4219)

/* APU Communication Ports */
#define REG_APUIO0   (*(volatile u8*)0x2140)
#define REG_APUIO1   (*(volatile u8*)0x2141)
#define REG_APUIO2   (*(volatile u8*)0x2142)
#define REG_APUIO3   (*(volatile u8*)0x2143)

/* Joypad buttons */
#define JOY_A       0x0080
#define JOY_B       0x8000

/*============================================================================
 * SPC700 Driver (assembled bytecode)
 *============================================================================
 * This minimal driver:
 *   1. Initializes the DSP to play a square wave on channel 0
 *   2. Loops, reading port 0 for pitch control
 *
 * The driver is loaded at SPC RAM $0200
 *============================================================================*/

static const u8 spc_driver[] = {
    /* $0200: Initialize DSP for square wave tone */
    0x8F, 0x6C, 0xF2,       /* mov $F2, #$6C      ; DSP register = FLG */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00      ; FLG = 0 (unmute, enable) */

    0x8F, 0x5D, 0xF2,       /* mov $F2, #$5D      ; DSP register = DIR */
    0x8F, 0x03, 0xF3,       /* mov $F3, #$03      ; Sample dir at $0300 */

    0x8F, 0x4C, 0xF2,       /* mov $F2, #$4C      ; DSP register = KON */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00      ; Key off all */

    /* Set up channel 0 voice */
    0x8F, 0x00, 0xF2,       /* mov $F2, #$00      ; V0VOLL */
    0x8F, 0x7F, 0xF3,       /* mov $F3, #$7F      ; Left volume max */

    0x8F, 0x01, 0xF2,       /* mov $F2, #$01      ; V0VOLR */
    0x8F, 0x7F, 0xF3,       /* mov $F3, #$7F      ; Right volume max */

    0x8F, 0x02, 0xF2,       /* mov $F2, #$02      ; V0PITCHL */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00      ; Pitch low = 0 */

    0x8F, 0x03, 0xF2,       /* mov $F2, #$03      ; V0PITCHH */
    0x8F, 0x10, 0xF3,       /* mov $F3, #$10      ; Pitch high = $10 */

    0x8F, 0x04, 0xF2,       /* mov $F2, #$04      ; V0SRCN */
    0x8F, 0x00, 0xF3,       /* mov $F3, #$00      ; Source = sample 0 */

    0x8F, 0x05, 0xF2,       /* mov $F2, #$05      ; V0ADSR1 */
    0x8F, 0x8F, 0xF3,       /* mov $F3, #$8F      ; ADSR on, A=max, D=7 */

    0x8F, 0x06, 0xF2,       /* mov $F2, #$06      ; V0ADSR2 */
    0x8F, 0xF0, 0xF3,       /* mov $F3, #$F0      ; S=max, R=0 */

    0x8F, 0x07, 0xF2,       /* mov $F2, #$07      ; V0GAIN */
    0x8F, 0x7F, 0xF3,       /* mov $F3, #$7F      ; GAIN (unused with ADSR) */

    /* Master volume */
    0x8F, 0x0C, 0xF2,       /* mov $F2, #$0C      ; MVOLL */
    0x8F, 0x7F, 0xF3,       /* mov $F3, #$7F      ; Left master = max */

    0x8F, 0x1C, 0xF2,       /* mov $F2, #$1C      ; MVOLR */
    0x8F, 0x7F, 0xF3,       /* mov $F3, #$7F      ; Right master = max */

    /* Key on channel 0 */
    0x8F, 0x4C, 0xF2,       /* mov $F2, #$4C      ; KON */
    0x8F, 0x01, 0xF3,       /* mov $F3, #$01      ; Key on voice 0 */

    /* Main loop: read port 0 for pitch control */
    /* $0257: loop */
    0xE4, 0xF4,             /* mov A, $F4         ; Read APUIO0 */
    0xC4, 0x00,             /* mov $00, A         ; Store pitch */
    0x8F, 0x03, 0xF2,       /* mov $F2, #$03      ; V0PITCHH */
    0xE4, 0x00,             /* mov A, $00         ; Get pitch */
    0xC4, 0xF3,             /* mov $F3, A         ; Set pitch high */
    0x2F, 0xF3,             /* bra loop (-13)     ; Loop forever */
};

/* Square wave sample (BRR format) at $0300 */
/* Sample directory: start=$0310, loop=$0310 */
static const u8 sample_dir[] = {
    0x10, 0x03,             /* Sample 0 start: $0310 */
    0x10, 0x03,             /* Sample 0 loop: $0310 */
};

/* Simple square wave BRR sample (1 block, looping) */
static const u8 sample_brr[] = {
    0xB3,                   /* Header: loop, filter 0, range 12 */
    0x77, 0x77, 0x77, 0x77, /* +7,+7,+7,+7,+7,+7,+7,+7 */
    0x88, 0x88, 0x88, 0x88, /* -8,-8,-8,-8,-8,-8,-8,-8 */
};

/*============================================================================
 * Functions
 *============================================================================*/

static void wait_vblank(void) {
    while (REG_HVBJOY & 0x80) {}
    while (!(REG_HVBJOY & 0x80)) {}
}

static u16 read_joypad(void) {
    while (REG_HVBJOY & 0x01) {}
    return REG_JOY1L | (REG_JOY1H << 8);
}

/**
 * Wait for SPC700 ready signal (with timeout)
 */
static u8 spc_wait_ready(void) {
    u16 timeout;

    /* Wait for $AA in port 0 */
    timeout = 0;
    while (REG_APUIO0 != 0xAA) {
        timeout++;
        if (timeout == 0) return 0;  /* Timeout after 65536 iterations */
    }

    /* Wait for $BB in port 1 */
    timeout = 0;
    while (REG_APUIO1 != 0xBB) {
        timeout++;
        if (timeout == 0) return 0;
    }

    return 1;  /* Success */
}

/**
 * Upload data to SPC700 RAM using IPL protocol
 */
static void spc_upload(u16 addr, const u8 *data, u16 size) {
    u16 i;
    u8 count;

    /* Set destination address */
    REG_APUIO2 = addr & 0xFF;
    REG_APUIO3 = addr >> 8;

    /* Start transfer (write $CC to port 0) */
    REG_APUIO1 = 0x01;      /* Non-zero = transfer */
    REG_APUIO0 = 0xCC;

    /* Wait for acknowledgment */
    while (REG_APUIO0 != 0xCC) {}

    /* Transfer data */
    count = 0;
    for (i = 0; i < size; i++) {
        REG_APUIO1 = data[i];
        REG_APUIO0 = count;
        while (REG_APUIO0 != count) {}
        count++;
    }
}

/**
 * Start SPC700 execution at address
 */
static void spc_execute(u16 addr) {
    u8 count;

    /* Read current counter */
    count = REG_APUIO0;
    count = count + 2;
    if (count == 0) count = 2;

    /* Set jump address */
    REG_APUIO2 = addr & 0xFF;
    REG_APUIO3 = addr >> 8;

    /* Signal execution (port1 = 0) */
    REG_APUIO1 = 0x00;
    REG_APUIO0 = count;

    /* Wait for driver to start (it won't respond with IPL protocol anymore) */
    /* Small delay */
    {
        volatile u16 delay;
        for (delay = 0; delay < 1000; delay++) {}
    }
}

/*============================================================================
 * Display Functions (minimal text)
 *============================================================================*/

static const u8 font_tiles[] = {
    /* Space */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* T */
    0x7E, 0x00, 0x18, 0x00, 0x18, 0x00, 0x18, 0x00,
    0x18, 0x00, 0x18, 0x00, 0x18, 0x00, 0x00, 0x00,
    /* O */
    0x3C, 0x00, 0x66, 0x00, 0x66, 0x00, 0x66, 0x00,
    0x66, 0x00, 0x66, 0x00, 0x3C, 0x00, 0x00, 0x00,
    /* N */
    0x66, 0x00, 0x76, 0x00, 0x7E, 0x00, 0x7E, 0x00,
    0x6E, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00,
    /* E */
    0x7E, 0x00, 0x60, 0x00, 0x60, 0x00, 0x7C, 0x00,
    0x60, 0x00, 0x60, 0x00, 0x7E, 0x00, 0x00, 0x00,
};

static const u8 message[] = { 1, 2, 3, 4, 0xFF }; /* "TONE" */

static void setup_display(void) {
    u16 i;

    /* Mode 0 */
    REG_BGMODE = 0x00;
    REG_BG1SC = 0x04;
    REG_BG12NBA = 0x00;

    /* Load font */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;
    for (i = 0; i < 80; i += 2) {
        REG_VMDATAL = font_tiles[i];
        REG_VMDATAH = font_tiles[i + 1];
    }

    /* Palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x28; /* Dark blue */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F; /* White */

    /* Clear tilemap */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0;
    }

    /* Write "TONE" at center */
    REG_VMADDL = 0xCE;  /* Row 14, col 14 */
    REG_VMADDH = 0x05;
    for (i = 0; message[i] != 0xFF; i++) {
        REG_VMDATAL = message[i];
        REG_VMDATAH = 0;
    }

    /* Enable BG1 */
    REG_TM = 0x01;

    /* Turn on screen so text is visible during SPC init */
    REG_INIDISP = 0x0F;
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u8 pitch;
    u8 sound_on;
    u16 joy;
    u16 prev_joy;

    /* Set up display */
    setup_display();

    /* SPC700 audio disabled - needs debugging
    if (spc_wait_ready()) {
        spc_upload(0x0200, spc_driver, sizeof(spc_driver));
        spc_upload(0x0300, sample_dir, sizeof(sample_dir));
        spc_upload(0x0310, sample_brr, sizeof(sample_brr));
        spc_execute(0x0200);
    }
    */

    /* Initial state */
    pitch = 0x10;
    sound_on = 1;
    prev_joy = 0;

    /* Set initial pitch */
    REG_APUIO0 = pitch;

    /* Enable NMI */
    REG_NMITIMEN = 0x81;

    /* Turn on screen */
    REG_INIDISP = 0x0F;

    /* Main loop */
    while (1) {
        wait_vblank();

        joy = read_joypad();

        /* A button: increase pitch */
        if ((joy & JOY_A) && !(prev_joy & JOY_A)) {
            pitch = pitch + 4;
            if (pitch > 0x3F) pitch = 0x04;
            if (sound_on) {
                REG_APUIO0 = pitch;
            }
            /* Visual feedback - flash background green */
            REG_CGADD = 0;
            REG_CGDATA = 0xE0;  /* Green */
            REG_CGDATA = 0x03;
        }

        /* B button: toggle sound */
        if ((joy & JOY_B) && !(prev_joy & JOY_B)) {
            sound_on = !sound_on;
            if (sound_on) {
                REG_APUIO0 = pitch;
            } else {
                REG_APUIO0 = 0;
            }
            /* Visual feedback - flash background red */
            REG_CGADD = 0;
            REG_CGDATA = 0x1F;  /* Red */
            REG_CGDATA = 0x00;
        }

        prev_joy = joy;
    }

    return 0;
}
