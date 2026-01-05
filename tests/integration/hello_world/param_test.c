/**
 * Test function with parameter and return value (like char_to_tile)
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

/* Two different tile patterns */
static const u8 tiles[] = {
    /* Tile 0: solid */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* Tile 1: checkered */
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
};

/* Function that takes parameter and returns value (like char_to_tile) */
static u8 get_tile(u8 input) {
    if (input == 0) {
        return 0;  /* Solid tile */
    } else {
        return 1;  /* Checkered tile */
    }
}

int main(void) {
    u16 i, j;

    REG_BGMODE = 0x00;
    REG_BG1SC = 0x08;
    REG_BG12NBA = 0x00;

    /* Load tiles */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 8; j++) {
            REG_VMDATAL = tiles[i * 8 + j];
            REG_VMDATAH = 0x00;
        }
    }

    /* Palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;

    /* Fill tilemap using get_tile function (like hello_world uses char_to_tile) */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x08;
    for (i = 0; i < 1024; i++) {
        u8 tile = get_tile(i & 1);  /* Call function with param, use return value */
        REG_VMDATAL = tile;
        REG_VMDATAH = 0x00;
    }

    REG_TM = 0x01;
    REG_INIDISP = 0x0F;

    while (1) {}
    return 0;
}
