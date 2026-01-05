/**
 * Test function calls (JSL/RTL) - like hello_world's load_font/set_palette
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

static const u8 tile_data[] = {
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
};

/* Separate function like hello_world's load_font */
static void load_tile(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;
    for (i = 0; i < 8; i++) {
        REG_VMDATAL = tile_data[i];
        REG_VMDATAH = 0x00;
    }
}

/* Separate function like hello_world's set_palette */
static void setup_palette(void) {
    REG_CGADD = 0;
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x00;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;
}

/* Separate function like hello_world's clearing tilemap */
static void fill_tilemap(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x08;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0x00;
    }
}

int main(void) {
    REG_BGMODE = 0x00;
    REG_BG1SC = 0x08;
    REG_BG12NBA = 0x00;

    /* Call functions like hello_world does */
    load_tile();
    setup_palette();
    fill_tilemap();

    REG_TM = 0x01;
    REG_INIDISP = 0x0F;

    while (1) {}
    return 0;
}
