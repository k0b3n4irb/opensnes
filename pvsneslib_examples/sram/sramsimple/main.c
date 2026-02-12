/*
 * sramsimple - Ported from PVSnesLib to OpenSNES
 *
 * Basic SRAM save/load demo.
 * Press A to save 0xCAFE to SRAM.
 * Press B to load from SRAM and display the value.
 *
 * -- alekmaul (original PVSnesLib example)
 */

#include <snes.h>
#include <snes/sram.h>

u16 save_value;
u16 load_value;

void updateDisplay(void) {
    textPrintAt(2, 10, "SAVED: 0x");
    textPrintHex(save_value, 4);

    textPrintAt(2, 12, "LOADED: 0x");
    textPrintHex(load_value, 4);
}

int main(void) {
    u16 pressed;

    /* Initialize hardware */
    consoleInit();
    setMode(BG_MODE0, 0);

    /* Initialize text system and load font */
    textInit();
    textLoadFont(0x0000);

    /* Configure BG1 */
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* White text color */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    REG_TM = TM_BG1;

    /* Initialize values */
    save_value = 0xCAFE;
    load_value = 0x0000;

    /* Draw static UI */
    textPrintAt(6, 1, "SRAM SIMPLE DEMO");
    textPrintAt(2, 4, "A = SAVE 0xCAFE TO SRAM");
    textPrintAt(2, 5, "B = LOAD FROM SRAM");

    updateDisplay();

    textFlush();
    WaitForVBlank();
    setScreenOn();

    while (1) {
        pressed = padPressed(0);

        if (pressed & KEY_A) {
            /* Save 0xCAFE to SRAM */
            save_value = 0xCAFE;
            sramSave((u8 *)&save_value, 2);

            textPrintAt(2, 15, "SAVED!       ");
            updateDisplay();
            textFlush();
        }

        if (pressed & KEY_B) {
            /* Load from SRAM */
            load_value = 0x0000;
            sramLoad((u8 *)&load_value, 2);

            textPrintAt(2, 15, "LOADED!      ");
            updateDisplay();
            textFlush();
        }

        WaitForVBlank();
    }

    return 0;
}
