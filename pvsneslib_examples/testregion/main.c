/*
 * testregion - Ported from PVSnesLib to OpenSNES
 *
 * Compares the SNES hardware region (NTSC/PAL from register $213F)
 * with the cartridge region (country byte in ROM header at $FFD9).
 * PVSnesLib original used consoleRegionIsOK() for this check.
 *
 * Country codes: $00=Japan(NTSC), $01=N.America(NTSC), $02+=Europe(PAL)
 */

#include <snes.h>

int main(void) {
    u8 cart_country;
    u8 cart_is_pal;
    u8 hw_is_pal;

    /* Initialize hardware */
    consoleInit();

    /* Set Mode 0 (2bpp BGs — matches built-in font) */
    setMode(BG_MODE0, 0);

    /* Initialize text system and load font */
    textInit();
    textLoadFont(0x0000);

    /* Configure BG1 to match text layout */
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* White text color */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    /* Enable BG1 */
    REG_TM = TM_BG1;

    /* Read cartridge country code from ROM header ($FFD9 in LoROM) */
    cart_country = *((volatile u8 *)0xFFD9);

    /* Country codes < 2 are NTSC (Japan, North America),
     * codes >= 2 are PAL (Europe and others) */
    cart_is_pal = (cart_country >= 2) ? 1 : 0;
    hw_is_pal = isPAL() ? 1 : 0;  /* Normalize: isPAL() returns 0xFF, not 1 */

    textPrintAt(9, 8, "CHECK REGIONS");
    textPrintAt(3, 12, "BETWEEN SNES AND CARTRIDGE");

    if (cart_is_pal == hw_is_pal) {
        textPrintAt(6, 16, "OK, THE SAME REGION!");
    } else {
        textPrintAt(5, 16, "NOT THE SAME REGION!");
    }

    textFlush();
    WaitForVBlank();

    /* Turn on screen */
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
