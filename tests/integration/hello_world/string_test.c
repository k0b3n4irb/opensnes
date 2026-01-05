/**
 * Test string pointer iteration (like print_at)
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

/* 4 different tiles */
static const u8 tiles[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* Tile 0: solid */
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,  /* Tile 1: checker */
    0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,  /* Tile 2: left half */
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,  /* Tile 3: right half */
};

/* Convert char to tile - simple version */
static u8 char_to_tile(char c) {
    if (c == 'A') return 0;
    if (c == 'B') return 1;
    if (c == 'C') return 2;
    if (c == 'D') return 3;
    return 0;
}

/* Print string at position - mimics hello_world print_at */
static void print_string(const char *str) {
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x08;

    while (*str) {
        u8 tile = char_to_tile(*str);
        REG_VMDATAL = tile;
        REG_VMDATAH = 0x00;
        str++;
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
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 8; j++) {
            REG_VMDATAL = tiles[i * 8 + j];
            REG_VMDATAH = 0x00;
        }
    }

    /* Palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;

    /* Clear tilemap with tile 0 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x08;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0x00;
    }

    /* Print string - this should show: solid, checker, left, right, solid, checker, left, right */
    print_string("ABCDABCD");

    REG_TM = 0x01;
    REG_INIDISP = 0x0F;

    while (1) {}
    return 0;
}
