/*
 * hdmagradient - Ported from PVSnesLib to OpenSNES
 *
 * Simple tile mode 3 demo with more than 32k tiles
 * -- alekmaul (original PVSnesLib example)
 *
 * Demonstrates Mode 3 (256-color / 8bpp) background with >32KB tile data
 * split across two DMA transfers, plus HDMA brightness gradient effect.
 * Press A to cycle through gradient levels (15 down to 2, then wrap).
 *
 * API differences from PVSnesLib:
 *  - setModeHdmaGradient(level)
 *      -> build_gradient_table(level) + hdmaSetup(ch, MODE_1REG, 0x00, table)
 *         (HDMA to $2100 = INIDISP = screen brightness register)
 *  - padsDown(0) -> padPressed(0)
 *  - bgInitMapSet() -> dmaCopyVram()
 *  - bgSetDisable() -> REG_TM
 *
 * The gradient effect works by using HDMA to write different brightness
 * levels to INIDISP ($2100) at different vertical positions on screen.
 * 224 scanlines / 16 steps = 14 lines per step. Each step computes:
 *   brightness = maxLevels - (step / (32 / (maxLevels + 1)))
 * This creates a vertical brightness gradient from bright to dim.
 */

#include <snes.h>
#include <snes/hdma.h>

extern u8 tiles_part1[], tiles_part1_end[];
extern u8 tiles_part2[], tiles_part2_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

/*
 * HDMA brightness gradient table.
 * 16 entries of 2 bytes each (line count + brightness), plus terminator.
 * Format for HDMA_MODE_1REG to $2100 (INIDISP):
 *   [count] [brightness 0-15]
 * Written to by build_gradient_table().
 */
static u8 gradient_table[16 * 2 + 1];

/*
 * Build a brightness gradient table.
 *
 * Replicates PVSnesLib setModeHdmaGradient logic:
 * 224 lines / 16 steps = 14 lines per step.
 * brightness[i] = maxLevels - (i / (32 / (maxLevels + 1)))
 *
 * This produces a gradient from full brightness at the top to dimmer
 * at the bottom, with the steepness controlled by maxLevels.
 */
static void build_gradient_table(u8 max_levels) {
    u8 i;
    u8 idx = 0;
    u16 divisor;

    /* 32 / (maxLevels + 1) */
    divisor = 32 / (max_levels + 1);
    if (divisor == 0) divisor = 1;

    for (i = 0; i < 16; i++) {
        u8 brightness;
        u8 step;

        gradient_table[idx] = 14;       /* 14 scanlines per step */
        idx++;

        step = i / divisor;
        if (step > max_levels) step = max_levels;
        brightness = max_levels - step;

        gradient_table[idx] = brightness;
        idx++;
    }

    gradient_table[idx] = 0;            /* End of table */
}

static void enable_gradient(u8 level) {
    build_gradient_table(level);
    /* HDMA to register $2100 (INIDISP = screen brightness) */
    /* Destination byte = $00 (low byte of $2100) */
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, 0x00, gradient_table);
    hdmaEnable(1 << HDMA_CHANNEL_6);
}

int main(void) {
    u8 gradient = 15;

    consoleInit();

    /* Load tiles in 2 phases (>32KB tile data) */
    /* First 32KB to VRAM $1000 */
    bgInitTileSet(0, tiles_part1, palette, 0,
                  tiles_part1_end - tiles_part1,
                  palette_end - palette,
                  BG_256COLORS, 0x1000);

    /* Remaining tiles to VRAM $5000 (continues after first 32KB) */
    dmaCopyVram(tiles_part2, 0x5000, tiles_part2_end - tiles_part2);

    /* Load tilemap at VRAM $0000 */
    bgSetMapPtr(0, 0x0000, BG_MAP_32x32);
    dmaCopyVram(tilemap, 0x0000, tilemap_end - tilemap);

    /* Mode 3 = 256-color (8bpp) mode */
    setMode(BG_MODE3, 0);
    REG_TM = TM_BG1;
    setScreenOn();

    while (1) {
        /* Press A to cycle gradient levels */
        if (padPressed(0) & KEY_A) {
            enable_gradient(gradient);
            gradient = (gradient - 1) & 15;
            if (gradient == 1) {
                gradient = 15;
            }
        }

        WaitForVBlank();
    }

    return 0;
}
