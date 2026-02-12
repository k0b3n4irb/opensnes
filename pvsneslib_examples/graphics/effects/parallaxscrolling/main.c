/*---------------------------------------------------------------------------------

    Simple parallax scrolling effect demo
    Ported from PVSnesLib (alekmaul) to OpenSNES

    3-region parallax via HDMA scroll offsets on BG1:
      Region 1: 72 lines at speed 1 (sky / distant)
      Region 2: 88 lines at speed 2 (middle)
      Region 3: 64 lines at speed 4 (ground / near)

    HDMA writes to BG1HOFS ($210D) using mode 1REG_2X (write-twice register).
    Table format: [count] [scroll_lo] [scroll_hi] ... [0]=end

---------------------------------------------------------------------------------*/
#include <snes.h>

extern u8 tiles[], tiles_end[];
extern u8 tilemap[];
extern u8 palette[];

/* HDMA scroll table in RAM (updated each frame) */
/* 3 regions * 3 bytes each + 1 byte terminator = 10 bytes */
static u8 scroll_table[16];

static u16 scroll1 = 0;
static u16 scroll2 = 0;
static u16 scroll3 = 0;

int main(void) {
    consoleInit();

    /* Load tiles at VRAM $1000, 2 palette slots (32 colors = 16*2*2 bytes) */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles, 16 * 2 * 2,
                  BG_16COLORS, 0x1000);

    /* Load 64x32 tilemap at VRAM $0000 */
    bgSetMapPtr(0, 0x0000, BG_MAP_64x32);
    dmaCopyVram(tilemap, 0x0000, 64 * 32 * 2);

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;
    setScreenOn();

    /* Initialize HDMA scroll table with region line counts */
    scroll_table[0] = 72;     /* Region 1: 72 scanlines (sky) */
    scroll_table[1] = 0;      /* scroll_lo */
    scroll_table[2] = 0;      /* scroll_hi */
    scroll_table[3] = 88;     /* Region 2: 88 scanlines (middle) */
    scroll_table[4] = 0;
    scroll_table[5] = 0;
    scroll_table[6] = 64;     /* Region 3: 64 scanlines (ground) */
    scroll_table[7] = 0;
    scroll_table[8] = 0;
    scroll_table[9] = 0x00;   /* End of HDMA table */

    /* Setup HDMA channel 6 for BG1 horizontal scroll */
    /* Mode 1REG_2X: writes 2 bytes to same register (write-twice $210D) */
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG_2X, HDMA_DEST_BG1HOFS, scroll_table);
    hdmaEnable(1 << HDMA_CHANNEL_6);

    while (1) {
        /* Increment scroll at different speeds per region */
        scroll1 += 1;   /* Slow (distant sky) */
        scroll2 += 2;   /* Medium (middle ground) */
        scroll3 += 4;   /* Fast (near ground) */

        /* Update HDMA table with new scroll values */
        scroll_table[1] = scroll1 & 0xFF;
        scroll_table[2] = (scroll1 >> 8) & 0xFF;
        scroll_table[4] = scroll2 & 0xFF;
        scroll_table[5] = (scroll2 >> 8) & 0xFF;
        scroll_table[7] = scroll3 & 0xFF;
        scroll_table[8] = (scroll3 >> 8) & 0xFF;

        WaitForVBlank();
    }
    return 0;
}
