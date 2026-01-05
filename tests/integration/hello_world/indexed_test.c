/**
 * Test indexed memory load (same pattern as hello_world font loading)
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

/* Test pattern - same as minimal_test but stored in array */
static const u8 test_pattern[] = {
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
};

int main(void) {
    u16 i;

    /* Mode 0, 8x8 tiles */
    REG_BGMODE = 0x00;
    REG_BG1SC = 0x08;
    REG_BG12NBA = 0x00;

    /* Set VRAM to word $0000 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    /* Write tile 0 using indexed array access (like hello_world) */
    for (i = 0; i < 8; i++) {
        REG_VMDATAL = test_pattern[i];  /* This uses indexed load! */
        REG_VMDATAH = 0x00;
    }

    /* Set palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x00;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    /* Fill tilemap with tile 0 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x08;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0x00;
    }

    REG_TM = 0x01;
    REG_INIDISP = 0x0F;

    while (1) {}
    return 0;
}
