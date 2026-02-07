/**
 * @file main.c
 * @brief SNES Button Tester - Direct VRAM approach
 *
 * Diagnostic: compares direct register read ($4218) with padHeld(0).
 * Uses direct VRAM writes — NO text library dependency (except font data).
 * Row addresses use (row << 5), no multiplication.
 *
 * SNES joypad bit layout:
 *   15:B  14:Y  13:Sel  12:Sta  11:Up  10:Dn  9:Lt  8:Rt
 *    7:A   6:X    5:L     4:R    3-0: controller ID
 */

#include <snes.h>

extern const unsigned char opensnes_font_2bpp[];

#define HW_JOY1 (*(volatile u16 *)0x4218)

#define TMAP 0x3800
#define T(c) ((u8)(c) - 32)
#define ADDR(x, y) (TMAP + ((u16)(y) << 5) + (x))

#define VRAM_SETADDR(a) do { \
    REG_VMAIN = 0x80; \
    REG_VMADDL = (a) & 0xFF; \
    REG_VMADDH = (a) >> 8; \
} while(0)

#define VRAM_TILE(t) do { \
    REG_VMDATAL = (t); \
    REG_VMDATAH = 0x00; \
} while(0)

static void vputs(u16 addr, const char *s) {
    REG_VMAIN = 0x80;
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;
    while (*s) {
        REG_VMDATAL = T(*s);
        REG_VMDATAH = 0x00;
        s++;
    }
}

#define TILE_O T('O')
#define TILE_N T('N')
#define TILE_DASH T('-')

#define BTN_IND(addr, raw, mask) do { \
    VRAM_SETADDR(addr); \
    if ((raw) & (mask)) { VRAM_TILE(TILE_O); VRAM_TILE(TILE_N); } \
    else { VRAM_TILE(TILE_DASH); VRAM_TILE(TILE_DASH); } \
} while(0)

static void vhex4(u16 addr, u16 val) {
    u8 d;
    VRAM_SETADDR(addr);

    d = (val >> 12) & 0xF;
    VRAM_TILE((d < 10) ? (d + T('0')) : (d - 10 + T('A')));

    d = (val >> 8) & 0xF;
    VRAM_TILE((d < 10) ? (d + T('0')) : (d - 10 + T('A')));

    d = (val >> 4) & 0xF;
    VRAM_TILE((d < 10) ? (d + T('0')) : (d - 10 + T('A')));

    d = val & 0xF;
    VRAM_TILE((d < 10) ? (d + T('0')) : (d - 10 + T('A')));
}

int main(void) {
    u16 raw;
    u16 lib;
    u16 i;

    consoleInit();
    setMode(BG_MODE0, 0);

    REG_BG1SC = 0x38;
    REG_BG12NBA = 0x00;

    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x38;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0x00;
        REG_VMDATAH = 0x00;
    }

    dmaCopyVram((u8 *)opensnes_font_2bpp, 0, 1536);

    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;

    REG_TM = TM_BG1;

    vputs(ADDR(5, 1), "SNES BUTTON TESTER");

    vputs(ADDR(1, 3), "RAW  $4218:");
    vputs(ADDR(1, 4), "padHeld(0):");

    vputs(ADDR(1, 7),  "UP:");
    vputs(ADDR(1, 8),  "DOWN:");
    vputs(ADDR(1, 9),  "LEFT:");
    vputs(ADDR(1, 10), "RIGHT:");

    vputs(ADDR(12, 7),  "A:");
    vputs(ADDR(12, 8),  "B:");
    vputs(ADDR(12, 9),  "X:");
    vputs(ADDR(12, 10), "Y:");

    vputs(ADDR(21, 7),  "L:");
    vputs(ADDR(21, 8),  "R:");
    vputs(ADDR(21, 9),  "SELECT:");
    vputs(ADDR(21, 10), "START:");

    setScreenOn();

    while (1) {
        WaitForVBlank();

        raw = HW_JOY1;
        lib = padHeld(0);

        /* Button indicators FIRST — all inline macros */

        BTN_IND(ADDR(4, 7),  raw, 0x0800);
        BTN_IND(ADDR(6, 8),  raw, 0x0400);
        BTN_IND(ADDR(6, 9),  raw, 0x0200);
        BTN_IND(ADDR(7, 10), raw, 0x0100);

        BTN_IND(ADDR(14, 7),  raw, 0x0080);
        BTN_IND(ADDR(14, 8),  raw, 0x8000);
        BTN_IND(ADDR(14, 9),  raw, 0x0040);
        BTN_IND(ADDR(14, 10), raw, 0x4000);

        BTN_IND(ADDR(23, 7), raw, 0x0020);
        BTN_IND(ADDR(23, 8), raw, 0x0010);

        BTN_IND(ADDR(28, 9),  raw, 0x2000);
        BTN_IND(ADDR(28, 10), raw, 0x1000);

        /* Hex values */
        vhex4(ADDR(13, 3), raw);
        vhex4(ADDR(13, 4), lib);
    }

    return 0;
}
