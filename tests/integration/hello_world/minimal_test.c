/**
 * Minimal display test - just show one tile pattern
 */

typedef unsigned char u8;
typedef unsigned short u16;

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

int main(void) {
    u16 i;

    /* Mode 0, 8x8 tiles */
    REG_BGMODE = 0x00;

    /* BG1 tilemap at $0800 */
    REG_BG1SC = 0x08;

    /* BG1 tiles at $0000 */
    REG_BG12NBA = 0x00;

    /* Set VRAM to word $0000 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    /* Write a simple checkered pattern to tile 0 (8 words = 16 bytes for 2bpp) */
    /* Pattern: alternating lines of $AA and $55 */
    for (i = 0; i < 8; i++) {
        if (i & 1) {
            REG_VMDATAL = 0x55;  /* Bitplane 0: 01010101 */
        } else {
            REG_VMDATAL = 0xAA;  /* Bitplane 0: 10101010 */
        }
        REG_VMDATAH = 0x00;      /* Bitplane 1: all zeros */
    }

    /* Set palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;  /* Color 0: black */
    REG_CGDATA = 0x00;
    REG_CGDATA = 0xFF;  /* Color 1: white */
    REG_CGDATA = 0x7F;

    /* Clear tilemap at $0800, fill with tile 0 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x08;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0;    /* Tile 0 */
        REG_VMDATAH = 0x00; /* Attributes */
    }

    /* Enable BG1 */
    REG_TM = 0x01;

    /* Screen on */
    REG_INIDISP = 0x0F;

    /* Loop forever */
    while (1) {
    }

    return 0;
}
