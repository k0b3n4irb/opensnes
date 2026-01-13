/**
 * @file main.c
 * @brief Minimal OpenSNES Project Template
 *
 * A starting point for your SNES game. This template shows:
 * - Basic hardware initialization
 * - Main loop structure
 * - Joypad reading
 * - Background color control
 *
 * Controls:
 *   D-pad: Change background color
 *   Start: Reset to default color
 *
 * @author Your Name
 * @license MIT (or your preferred license)
 */

typedef unsigned char u8;
typedef unsigned short u16;

/*============================================================================
 * Hardware Registers
 *============================================================================*/

#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_CGADD    (*(volatile u8*)0x2121)
#define REG_CGDATA   (*(volatile u8*)0x2122)
#define REG_NMITIMEN (*(volatile u8*)0x4200)
#define REG_RDNMI    (*(volatile u8*)0x4210)
#define REG_HVBJOY   (*(volatile u8*)0x4212)
#define REG_JOY1L    (*(volatile u8*)0x4218)
#define REG_JOY1H    (*(volatile u8*)0x4219)

/* Joypad masks */
#define JOY_UP      0x0800
#define JOY_DOWN    0x0400
#define JOY_LEFT    0x0200
#define JOY_RIGHT   0x0100
#define JOY_START   0x1000

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void wait_vblank(void) {
    /* Wait for VBlank interrupt flag */
    while (!(REG_RDNMI & 0x80)) {}
}

static u16 read_joypad(void) {
    /* Wait for auto-read to complete */
    while (REG_HVBJOY & 0x01) {}
    return REG_JOY1L | (REG_JOY1H << 8);
}

static void set_bg_color(u8 r, u8 g, u8 b) {
    /* SNES uses 5-bit color components (0-31) */
    u16 color;
    color = (r & 0x1F) | ((g & 0x1F) << 5) | ((b & 0x1F) << 10);

    REG_CGADD = 0;  /* Color 0 = backdrop */
    REG_CGDATA = color & 0xFF;
    REG_CGDATA = (color >> 8) & 0xFF;
}

/*============================================================================
 * Main Program
 *============================================================================*/

int main(void) {
    u16 joy;
    u8 red, green, blue;

    /* Initialize color values */
    red = 0;
    green = 0;
    blue = 16;  /* Start with blue */

    /* Set initial background color */
    set_bg_color(red, green, blue);

    /* Enable NMI and auto-joypad read */
    REG_NMITIMEN = 0x81;

    /* Turn on screen at full brightness */
    REG_INIDISP = 0x0F;

    /* Main game loop */
    while (1) {
        wait_vblank();

        joy = read_joypad();

        /* Modify color with D-pad */
        if (joy & JOY_UP) {
            if (green < 31) green++;
        }
        if (joy & JOY_DOWN) {
            if (green > 0) green--;
        }
        if (joy & JOY_RIGHT) {
            if (red < 31) red++;
        }
        if (joy & JOY_LEFT) {
            if (red > 0) red--;
        }

        /* Reset with Start */
        if (joy & JOY_START) {
            red = 0;
            green = 0;
            blue = 16;
        }

        /* Update background color */
        set_bg_color(red, green, blue);
    }

    return 0;
}
