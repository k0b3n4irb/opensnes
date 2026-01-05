/**
 * Test nested loop with i*8+j indexing (same as hello_world font loading)
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

/* 2 tiles of test pattern (like 2 font characters) */
static const u8 test_font[] = {
    /* Tile 0: solid pattern */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* Tile 1: checkered pattern */
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
};

int main(void) {
    u16 i, j;

    REG_BGMODE = 0x00;
    REG_BG1SC = 0x08;
    REG_BG12NBA = 0x00;

    /* Load 2 tiles using nested loop (like hello_world) */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    for (i = 0; i < 2; i++) {          /* 2 tiles */
        for (j = 0; j < 8; j++) {      /* 8 rows per tile */
            REG_VMDATAL = test_font[i * 8 + j];  /* Same indexing as hello_world! */
            REG_VMDATAH = 0x00;
        }
    }

    /* Palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Color 0: black */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* Color 1: white */

    /* Fill tilemap - alternating tile 0 and 1 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x08;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = (i & 1);  /* Alternate between tile 0 and 1 */
        REG_VMDATAH = 0x00;
    }

    REG_TM = 0x01;
    REG_INIDISP = 0x0F;

    while (1) {}
    return 0;
}
