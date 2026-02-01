/**
 * @file main.c
 * @brief TADA Sound Example - Press A to play sound
 */

typedef unsigned char u8;
typedef unsigned short u16;

/* Hardware registers */
#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_BGMODE   (*(volatile u8*)0x2105)
#define REG_BG1SC    (*(volatile u8*)0x2107)
#define REG_BG12NBA  (*(volatile u8*)0x210B)
#define REG_TM       (*(volatile u8*)0x212C)
#define REG_CGADD    (*(volatile u8*)0x2121)
#define REG_CGDATA   (*(volatile u8*)0x2122)
#define REG_NMITIMEN (*(volatile u8*)0x4200)
#define REG_HVBJOY   (*(volatile u8*)0x4212)
#define REG_JOY1H    (*(volatile u8*)0x4219)

/* Assembly functions */
extern void spc_init(void);
extern void spc_play(void);
extern void vram_init_text(void);

int main(void) {
    u8 prev_a = 0;

    /* Force blank during setup */
    REG_INIDISP = 0x80;

    /* Mode 0: 4 BG layers, all 2bpp */
    REG_BGMODE = 0x00;

    /* BG1 tilemap at $0400 (word address $0200), 32x32 */
    REG_BG1SC = 0x04;

    /* BG1 chr at $0000 */
    REG_BG12NBA = 0x00;

    /* Set up palette: color 0 = dark blue (bg), color 3 = white (text) */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;  /* Color 0: dark blue */
    REG_CGDATA = 0x40;
    REG_CGADD = 3;
    REG_CGDATA = 0xFF;  /* Color 3: white */
    REG_CGDATA = 0x7F;

    /* Initialize SPC and upload driver + sample */
    spc_init();

    /* Upload font and "TADA" text to VRAM */
    vram_init_text();

    /* Enable BG1 */
    REG_TM = 0x01;

    /* Screen on, full brightness */
    REG_INIDISP = 0x0F;

    /* Enable joypad auto-read */
    REG_NMITIMEN = 0x01;

    /* Main loop */
    while (1) {
        u8 joy;

        /* Wait for auto-read complete */
        while (REG_HVBJOY & 0x01) {}
        joy = REG_JOY1H;

        /* A button pressed (new press only)? */
        if ((joy & 0x80) && !prev_a) {
            spc_play();
        }
        prev_a = joy & 0x80;

        /* VBlank wait */
        while (!(REG_HVBJOY & 0x80)) {}
        while (REG_HVBJOY & 0x80) {}
    }

    return 0;
}
