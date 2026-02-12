/*
 * window - Ported from PVSnesLib to OpenSNES
 *
 * Simple window effect in mode 1
 * -- alekmaul (original PVSnesLib example)
 *
 * Demonstrates HDMA-driven triangle window masking on two backgrounds.
 * Press A to apply window to BG1 only, X for BG2 only, B for both.
 *
 * API differences from PVSnesLib:
 *  - setModeHdmaWindow(mask, w12, left, right)
 *      -> hdmaSetup(ch, MODE_1REG, WH0, left)
 *         hdmaSetup(ch, MODE_1REG, WH1, right)
 *         + manual REG_W12SEL / REG_TMW setup
 *  - padsCurrent(0) -> padHeld(0)
 *  - bgInitMapSet()  -> dmaCopyVram()
 *  - bgSetEnable/bgSetDisable -> REG_TM
 *
 * The triangle shape is defined by two HDMA tables (left and right edges)
 * that use repeat mode (bit 7 set) for per-scanline window boundary writes.
 * HDMA channel 6 drives WH0 ($2126 = Window 1 Left position),
 * HDMA channel 7 drives WH1 ($2127 = Window 1 Right position).
 *
 * When left > right on a scanline, the window region is empty (disabled).
 * REG_W12SEL enables Window 1 on BG1 and/or BG2 with inversion (mask outside).
 */

#include <snes.h>
#include <snes/hdma.h>

extern u8 bg1_tiles[], bg1_tiles_end[];
extern u8 bg1_map[], bg1_map_end[];
extern u8 bg1_pal[], bg1_pal_end[];
extern u8 bg2_tiles[], bg2_tiles_end[];
extern u8 bg2_map[], bg2_map_end[];
extern u8 bg2_pal[], bg2_pal_end[];

/*
 * HDMA tables for triangle window shape.
 *
 * Format: [count] [data_byte] ... [0]=end
 * Bit 7 of count = repeat mode (write data every scanline for count lines).
 *
 * The left table starts at 0xFF (left > right = window disabled) for 60
 * scanlines, then sweeps inward from 0x7F to 0x60 and back over 64 lines
 * (forming the triangle), then 0xFF again (disabled) for the remainder.
 *
 * The right table mirrors: starts at 0x00 for 60 lines, sweeps from 0x81
 * to 0xA0 and back over 64 lines, then 0x00 for the remainder.
 */
static const u8 table_left[] = {
    60, 0xFF,       /* 60 lines: left=255 (window disabled, left > right) */
    0x80 | 64,      /* 64 lines of per-scanline data (repeat mode) */
    0x7F,0x7E,0x7D,0x7C,0x7B,0x7A,0x79,0x78,0x77,0x76,0x75,0x74,0x73,0x72,0x71,0x70,
    0x6F,0x6E,0x6D,0x6C,0x6B,0x6A,0x69,0x68,0x67,0x66,0x65,0x64,0x63,0x62,0x61,0x60,
    0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,
    0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
    0xFF, 0         /* remaining: left=255 (disabled), end */
};

static const u8 table_right[] = {
    60, 0x00,       /* 60 lines: right=0 (window disabled) */
    0x80 | 64,      /* 64 lines of per-scanline data */
    0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,
    0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,0xA0,
    0x9F,0x9E,0x9D,0x9C,0x9B,0x9A,0x99,0x98,0x97,0x96,0x95,0x94,0x93,0x92,0x91,0x90,
    0x8F,0x8E,0x8D,0x8C,0x8B,0x8A,0x89,0x88,0x87,0x86,0x85,0x84,0x83,0x82,0x81,
    0x00, 0         /* remaining: right=0, end */
};

static u8 pada, padb, padx;

/*
 * setup_window - Configure HDMA window effect on the specified BG layers.
 *
 * w12sel controls which layers have Window 1 enabled and whether it inverts.
 * REG_W12SEL bits:
 *   1:0 = BG1 Window 1 (bit 0=invert, bit 1=enable)
 *   5:4 = BG2 Window 1 (bit 4=invert, bit 5=enable)
 *
 * REG_TMW enables window masking on the main screen layers.
 */
static void setup_window(u8 w12sel) {
    /* Disable previous HDMA */
    hdmaDisable((1 << HDMA_CHANNEL_6) | (1 << HDMA_CHANNEL_7));

    /* Set window 1 enable/invert mask */
    REG_W12SEL = w12sel;

    /* Enable window masking on main screen for active layers */
    {
        u8 tmw = 0;
        if (w12sel & 0x02) tmw |= TM_BG1;
        if (w12sel & 0x20) tmw |= TM_BG2;
        REG_TMW = tmw;
    }

    /* HDMA ch6 -> WH0 ($2126 = Window 1 Left position) */
    /* HDMA ch7 -> WH1 ($2127 = Window 1 Right position) */
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, HDMA_DEST_WH0, table_left);
    hdmaSetup(HDMA_CHANNEL_7, HDMA_MODE_1REG, HDMA_DEST_WH1, table_right);
    hdmaEnable((1 << HDMA_CHANNEL_6) | (1 << HDMA_CHANNEL_7));
}

int main(void) {
    consoleInit();

    /* Load BG1 tiles at VRAM $4000, palette slot 0 */
    bgInitTileSet(0, bg1_tiles, bg1_pal, 0,
                  bg1_tiles_end - bg1_tiles,
                  bg1_pal_end - bg1_pal,
                  BG_16COLORS, 0x4000);

    /* Load BG2 tiles at VRAM $6000, palette slot 1 */
    bgInitTileSet(1, bg2_tiles, bg2_pal, 1,
                  bg2_tiles_end - bg2_tiles,
                  bg2_pal_end - bg2_pal,
                  BG_16COLORS, 0x6000);

    /* Load BG1 tilemap at VRAM $0000 */
    bgSetMapPtr(0, 0x0000, BG_MAP_32x32);
    dmaCopyVram(bg1_map, 0x0000, bg1_map_end - bg1_map);

    /* Load BG2 tilemap at VRAM $1000 */
    bgSetMapPtr(1, 0x1000, BG_MAP_32x32);
    dmaCopyVram(bg2_map, 0x1000, bg2_map_end - bg2_map);

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1 | TM_BG2;
    setScreenOn();

    /* Initial window: both BG1 and BG2, Window 1 enabled + inverted (mask outside) */
    /* BG1W1: enable=1, invert=1 -> bits 1:0 = 0b11 = 0x03 */
    /* BG2W1: enable=1, invert=1 -> bits 5:4 = 0b11 = 0x30 */
    setup_window(0x33);

    pada = 0;
    padb = 0;
    padx = 0;

    while (1) {
        u16 pad0 = padHeld(0);

        /* A = BG1 only window */
        if (pad0 & KEY_A) {
            if (!pada) {
                pada = 1;
                setup_window(0x03); /* BG1W1 enable+invert only */
            }
        } else {
            pada = 0;
        }

        /* X = BG2 only window */
        if (pad0 & KEY_X) {
            if (!padx) {
                padx = 1;
                setup_window(0x30); /* BG2W1 enable+invert only */
            }
        } else {
            padx = 0;
        }

        /* B = both BG1+BG2 window */
        if (pad0 & KEY_B) {
            if (!padb) {
                padb = 1;
                setup_window(0x33); /* Both BG1W1 + BG2W1 */
            }
        } else {
            padb = 0;
        }

        WaitForVBlank();
    }

    return 0;
}
