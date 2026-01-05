// Test: Can C code do VRAM operations?
// This file does the graphics setup that was previously in assembly.

// SNES PPU registers
#define REG_INIDISP  (*(volatile unsigned char*)0x2100)
#define REG_BGMODE   (*(volatile unsigned char*)0x2105)
#define REG_BG1SC    (*(volatile unsigned char*)0x2107)
#define REG_BG12NBA  (*(volatile unsigned char*)0x210B)
#define REG_VMAIN    (*(volatile unsigned char*)0x2115)
#define REG_VMADDL   (*(volatile unsigned char*)0x2116)
#define REG_VMADDH   (*(volatile unsigned char*)0x2117)
#define REG_VMDATAL  (*(volatile unsigned char*)0x2118)
#define REG_VMDATAH  (*(volatile unsigned char*)0x2119)
#define REG_CGADD    (*(volatile unsigned char*)0x2121)
#define REG_CGDATA   (*(volatile unsigned char*)0x2122)
#define REG_TM       (*(volatile unsigned char*)0x212C)

// Font tiles (8x8 2bpp - 8 bytes per row, but we only use 1 bitplane)
// D, E, H, L, O, R, W, !, space
static const unsigned char font_D[] = {0x7C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7C, 0x00};
static const unsigned char font_E[] = {0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x7E, 0x00};
static const unsigned char font_H[] = {0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00};
static const unsigned char font_L[] = {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E, 0x00};
static const unsigned char font_O[] = {0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00};
static const unsigned char font_R[] = {0x7C, 0x42, 0x42, 0x7C, 0x48, 0x44, 0x42, 0x00};
static const unsigned char font_W[] = {0x42, 0x42, 0x42, 0x42, 0x5A, 0x66, 0x42, 0x00};
static const unsigned char font_X[] = {0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00}; // !
static const unsigned char font_SP[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // space

void write_tile(const unsigned char* data) {
    int i;
    for (i = 0; i < 8; i++) {
        REG_VMDATAL = data[i];  // Bitplane 0
        REG_VMDATAH = 0;        // Bitplane 1 (empty)
    }
}

int main(void) {
    int i;

    // VMAIN: increment after high byte write
    REG_VMAIN = 0x80;

    // Write font tiles to VRAM $0000
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    write_tile(font_D);  // Tile 0
    write_tile(font_E);  // Tile 1
    write_tile(font_H);  // Tile 2
    write_tile(font_L);  // Tile 3
    write_tile(font_O);  // Tile 4
    write_tile(font_R);  // Tile 5
    write_tile(font_W);  // Tile 6
    write_tile(font_X);  // Tile 7 (!)
    write_tile(font_SP); // Tile 8 (space)

    // Clear tilemap at $0800 with spaces
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x08;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 8;  // Tile 8 = space
        REG_VMDATAH = 0;  // Attributes
    }

    // Write "HELLO WORLD!" at row 14, col 10
    // Address = $0800 + (14*32) + 10 = $0800 + $1CA = $09CA
    REG_VMADDL = 0xCA;
    REG_VMADDH = 0x09;

    // H=2, E=1, L=3, L=3, O=4, space=8, W=6, O=4, R=5, L=3, D=0, !=7
    REG_VMDATAL = 2; REG_VMDATAH = 0;  // H
    REG_VMDATAL = 1; REG_VMDATAH = 0;  // E
    REG_VMDATAL = 3; REG_VMDATAH = 0;  // L
    REG_VMDATAL = 3; REG_VMDATAH = 0;  // L
    REG_VMDATAL = 4; REG_VMDATAH = 0;  // O
    REG_VMDATAL = 8; REG_VMDATAH = 0;  // (space)
    REG_VMDATAL = 6; REG_VMDATAH = 0;  // W
    REG_VMDATAL = 4; REG_VMDATAH = 0;  // O
    REG_VMDATAL = 5; REG_VMDATAH = 0;  // R
    REG_VMDATAL = 3; REG_VMDATAH = 0;  // L
    REG_VMDATAL = 0; REG_VMDATAH = 0;  // D
    REG_VMDATAL = 7; REG_VMDATAH = 0;  // !

    // Set palette
    REG_CGADD = 0;
    REG_CGDATA = 0x00;  // Color 0 low (dark blue)
    REG_CGDATA = 0x28;  // Color 0 high
    REG_CGDATA = 0xFF;  // Color 1 low (white)
    REG_CGDATA = 0x7F;  // Color 1 high

    // Enable BG1
    REG_TM = 0x01;

    // Screen on
    REG_INIDISP = 0x0F;

    // Infinite loop
    while (1) {}

    return 0;
}
