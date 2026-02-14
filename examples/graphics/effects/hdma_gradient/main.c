/**
 * @file main.c
 * @brief HDMA Gradient Example
 *
 * Demonstrates HDMA to create a vertical color gradient on BG1.
 * The gradient goes from dark blue at top to bright cyan at bottom.
 */

#include <snes.h>
#include <snes/hdma.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];
extern u8 hdma_gradient_table[];

int main(void) {
    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /*------------------------------------------------------------------------
     * Configure Background
     *------------------------------------------------------------------------*/

    bgSetMapPtr(0, 0x1000, SC_32x32);
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x4000);
    dmaCopyVram(tilemap, 0x1000, tilemap_end - tilemap);

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;
    bgSetScroll(0, 0, 0);

    /*------------------------------------------------------------------------
     * Set up Color Math for BG1
     *------------------------------------------------------------------------*/

    REG_CGWSEL = 0x02;      /* Use fixed color */
    REG_CGADSUB = 0x01;     /* Add mode, enable for BG1 */

    /* Initialize ALL color channels to 0 first */
    /* COLDATA format: bits 7-5 select channels, bits 4-0 = intensity */
    REG_COLDATA = 0x20 | 0;  /* Red = 0 */
    REG_COLDATA = 0x40 | 0;  /* Green = 0 */
    REG_COLDATA = 0x80 | 0;  /* Blue = 0 */

    /*------------------------------------------------------------------------
     * Set up HDMA for gradient
     *------------------------------------------------------------------------
     * The HDMA table now sets ALL channels (RGB) for each gradient step.
     *------------------------------------------------------------------------*/

    hdmaSetup(HDMA_CHANNEL_7, HDMA_MODE_1REG, HDMA_DEST_COLDATA, hdma_gradient_table);
    hdmaEnable(1 << HDMA_CHANNEL_7);

    /*------------------------------------------------------------------------
     * Turn on screen
     *------------------------------------------------------------------------*/

    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /*------------------------------------------------------------------------
     * Main loop
     *------------------------------------------------------------------------*/

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
