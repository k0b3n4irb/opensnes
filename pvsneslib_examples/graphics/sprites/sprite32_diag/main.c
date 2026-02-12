/*
 * sprite32_diag - OpenSNES diagnostic test for 32x32 sprites
 *
 * Tests 32x32 sprite rendering with FOUR methods and displays
 * OAM buffer contents on screen for runtime verification.
 *
 *   Sprite 0 (X=100, Y=80):  PURE ASSEMBLY — direct PPU OAM + DMA
 *   Sprite 1 (X=80, Y=140):  Direct OAM buffer writes
 *   Sprite 2 (X=140, Y=140): Library oamSet + oamSetEx
 *   Sprite 3 (X=200, Y=140): Direct buffer + library oamSetSize
 *
 * Bottom of screen shows OAM buffer readback (hex):
 *   SPR0: XX YY TT AA   (bytes 0-3)
 *   SPR1: XX YY TT AA   (bytes 4-7)
 *   SPR2: XX YY TT AA   (bytes 8-11)
 *   SPR3: XX YY TT AA   (bytes 12-15)
 *   EXT0: HH             (byte 512 = sprites 0-3 extension)
 *
 * Expected EXT0 = 0A (sprites 0+1 large, 2+3 also large)
 * = binary: 10_10_10_10 => 0xAA... no wait:
 *   Sprite 0: bits 0-1 = 10 (size=1, xhi=0)
 *   Sprite 1: bits 2-3 = 10 (size=1, xhi=0)
 *   Sprite 2: bits 4-5 = 10 (size=1, xhi=0)
 *   Sprite 3: bits 6-7 = 10 (size=1, xhi=0)
 *   = 0b10101010 = 0xAA
 */

#include <snes.h>

extern u8 sprite32[], sprite32_end[], palsprite32[];

/* Direct access to OAM buffer (defined in crt0.asm) */
extern u8 oamMemory[];

/* Pure assembly function - writes directly to PPU (data.asm) */
extern void asm_setupSpriteDirectPPU(void);

/* Print one byte as 2-digit hex at position (x,y) */
void printHexByte(u8 x, u8 y, u8 val) {
    textSetPos(x, y);
    textPrintHex(val, 2);
}

/* Display OAM buffer state for sprites 0-3 and extension byte */
void showBufferState(void) {
    u8 i;

    for (i = 0; i < 4; i++) {
        u16 off = i * 4;  /* Use u16 to avoid issues */
        u8 row = 22 + i;

        /* Label */
        textSetPos(1, row);
        textPrintU16(i);
        textPutChar(':');

        /* X position */
        printHexByte(4, row, oamMemory[off]);
        /* Y position */
        printHexByte(7, row, oamMemory[off + 1]);
        /* Tile */
        printHexByte(10, row, oamMemory[off + 2]);
        /* Attributes */
        printHexByte(13, row, oamMemory[off + 3]);
    }

    /* Extension byte for sprites 0-3 */
    textPrintAt(1, 27, "EXT:");
    printHexByte(6, 27, oamMemory[512]);

    textFlush();
}

int main(void) {
    consoleInit();

    /* ================================================================
     * Text setup on BG3 (2bpp in Mode 1)
     * ================================================================ */
    textInitEx(0xD000, 0, 0);
    textLoadFont(0x3000);
    bgSetGfxPtr(2, 0x3000);
    bgSetMapPtr(2, 0x6800, BG_MAP_32x32);

    /* ================================================================
     * SPRITE 0: PURE ASSEMBLY — direct PPU OAM + DMA
     * Loads tile data to VRAM $2100, palette to CGRAM 128,
     * sets OBSEL = 0x21, writes sprite 0 directly to PPU OAM.
     * Position: (100, 80), tile $10, priority 3, large 32x32.
     * ================================================================ */
    asm_setupSpriteDirectPPU();

    /* Mirror sprite 0 in the buffer so NMI DMA doesn't destroy it. */
    oamMemory[0] = 100;       /* X low = 100 */
    oamMemory[1] = 80;        /* Y = 80 */
    oamMemory[2] = 0x10;      /* Tile $10 */
    oamMemory[3] = 0x30;      /* Attr: priority 3, palette 0 */
    oamMemory[512] = (oamMemory[512] & 0xFC) | 0x02;  /* size=large, X high=0 */

    /* ================================================================
     * SPRITE 1: Direct buffer writes (NO library functions)
     * Position: (80, 140), tile $10, palette 0, priority 3, large
     * ================================================================ */
    oamMemory[4] = 80;        /* X low */
    oamMemory[5] = 140;       /* Y */
    oamMemory[6] = 0x10;      /* Tile $10 */
    oamMemory[7] = 0x30;      /* Attr: priority 3 */
    oamMemory[512] = (oamMemory[512] & 0xF3) | 0x08;  /* size=large, X high=0 */

    /* ================================================================
     * SPRITE 2: Library oamSet + oamSetEx
     * Position: (140, 140), tile $10, palette 0, priority 3, large
     * ================================================================ */
    oamSet(2, 140, 140, 0x0010, 0, 3, 0);
    oamSetEx(2, OBJ_LARGE, OBJ_SHOW);

    /* ================================================================
     * SPRITE 3: Direct buffer + library oamSetSize
     * Position: (200, 140), tile $10, palette 0, priority 3, large
     * ================================================================ */
    oamMemory[12] = 200;      /* X low */
    oamMemory[13] = 140;      /* Y */
    oamMemory[14] = 0x10;     /* Tile $10 */
    oamMemory[15] = 0x30;     /* Attr: priority 3 */
    oamSetSize(3, OBJ_LARGE);
    oamMemory[512] &= 0xBF;   /* Clear bit 6 (X high for sprite 3) */

    /* ================================================================
     * Display titles and expected values
     * ================================================================ */
    textPrintAt(1, 0, "SPRITE32 DIAG TEST");
    textPrintAt(1, 2, "4 sprites should show.");
    textPrintAt(1, 3, "All identical 32x32.");
    textPrintAt(1, 5, "0:ASM 1:BUF 2:LIB 3:MIX");

    /* Column headers for buffer dump */
    textPrintAt(1, 20, "OAM BUFFER READBACK:");
    textPrintAt(4, 21, "X  Y  T  A");

    /* Show expected extension byte */
    textPrintAt(1, 28, "EXP:AA");

    /* Show actual buffer state BEFORE turning screen on */
    showBufferState();

    /* Enable Mode 1 with BG3 (text) + sprites */
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG3 | TM_OBJ;

    /* Backdrop = dark blue so text and sprites are visible */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x40;   /* Dark blue */

    setScreenOn();

    while (1) {
        WaitForVBlank();

        /* Refresh buffer state display every frame */
        showBufferState();
    }

    return 0;
}
