/**
 * SSA Phi-Node Runtime Verification
 *
 * Accumulates button presses in a loop variable (maxraw |= raw).
 * If the phi-node bug exists, maxraw would never accumulate
 * (each iteration would read from a wrong/stale slot).
 *
 * Expected behavior:
 * - Press buttons → hex values on screen grow (bits accumulate)
 * - Once a bit is set in maxraw, it stays set permanently
 * - After pressing all buttons, maxraw should reach 0xFFF0
 *
 * Display:
 *   Row 1: "PHI-NODE TEST"
 *   Row 3: "RAW:  xxxx"    (current $4218 value)
 *   Row 4: "MAX:  xxxx"    (accumulated OR of all frames)
 *   Row 5: "FRAME:xxxx"    (frame counter)
 *   Row 7: "OK=FFF0 AFTER ALL BUTTONS"
 */

#include <snes.h>

extern const unsigned char opensnes_font_2bpp[];

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
    u16 maxraw;
    u16 frame;
    u16 i;

    consoleInit();
    setMode(BG_MODE0, 0);

    REG_BG1SC = 0x38;
    REG_BG12NBA = 0x00;

    /* Clear tilemap */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x38;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0x00;
        REG_VMDATAH = 0x00;
    }

    dmaCopyVram((u8 *)opensnes_font_2bpp, 0, 1536);

    /* Palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;

    REG_TM = TM_BG1;

    /* Static labels */
    vputs(ADDR(5, 1), "PHI-NODE TEST");
    vputs(ADDR(1, 3), "RAW:  ");
    vputs(ADDR(1, 4), "MAX:  ");
    vputs(ADDR(1, 5), "FRAME:");
    vputs(ADDR(1, 7), "OK=FFF0 AFTER ALL BTNS");

    setScreenOn();

    /* Initialize accumulators */
    maxraw = 0;
    frame = 0;

    /* Main loop — the phi-node test */
    while (1) {
        WaitForVBlank();

        raw = padHeld(0);

        /* THE CRITICAL LINE: phi-node accumulator */
        maxraw = maxraw | raw;

        /* Frame counter */
        frame = frame + 1;

        /* Display values */
        vhex4(ADDR(7, 3), raw);
        vhex4(ADDR(7, 4), maxraw);
        vhex4(ADDR(7, 5), frame);
    }

    return 0;
}
