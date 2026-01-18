/**
 * @file main.c
 * @brief Audio test - diagnose where audioInit hangs
 */

typedef unsigned char u8;
typedef unsigned short u16;

/* Hardware registers */
#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_BGMODE   (*(volatile u8*)0x2105)
#define REG_TM       (*(volatile u8*)0x212C)
#define REG_CGADD    (*(volatile u8*)0x2121)
#define REG_CGDATA   (*(volatile u8*)0x2122)
#define REG_NMITIMEN (*(volatile u8*)0x4200)
#define REG_HVBJOY   (*(volatile u8*)0x4212)

/* Audio functions */
extern void audioInit(void);
extern void audioPlaySample(u8 id);
extern void audioSetVolume(u8 vol);

int main(void) {
    u8 prev_a = 0;

    /* Force blank */
    REG_INIDISP = 0x80;

    /* Disable all layers */
    REG_TM = 0x00;

    /* Set Mode 0 */
    REG_BGMODE = 0x00;

    /* Set backdrop to GREEN (audio ready) */
    REG_CGADD = 0;
    REG_CGDATA = 0xE0;
    REG_CGDATA = 0x03;

    /* Initialize audio */
    audioInit();

    /* Enable joypad auto-read */
    REG_NMITIMEN = 0x01;

    /* Screen on */
    REG_INIDISP = 0x0F;

    /* Main loop - press A to play sound */
    while (1) {
        u8 joy;

        /* Wait for auto-read complete */
        while (REG_HVBJOY & 0x01) { }
        joy = *((volatile u8*)0x4219);  /* JOY1H: A is bit 7 */

        /* A button pressed (new press only) */
        if ((joy & 0x80) && !prev_a) {
            audioPlaySample(0);
            /* Flash blue when playing */
            REG_CGADD = 0;
            REG_CGDATA = 0x00;
            REG_CGDATA = 0x7C;
        }
        prev_a = joy & 0x80;

        /* VBlank wait */
        while (!(REG_HVBJOY & 0x80)) { }
        while (REG_HVBJOY & 0x80) { }
    }

    return 0;
}
