/**
 * @file main.c
 * @brief SA-1 boot diagnostic (Phase 1)
 * @ingroup examples
 */

#include <snes.h>
#include <snes/sa1.h>

extern u8 sa1_status;

int main(void) {
    u8 status;

    consoleInit();
    setMode(BG_MODE0, 0);

    setColor(0, 0x0000);
    setColor(1, RGB(31, 31, 31));

    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    textPrintAt(3, 3, "SA-1 BOOT DIAGNOSTIC");

    status = sa1_status;

    textPrintAt(3, 6, "SA1 STATUS BYTE:");

    if (status == 0xA5) {
        textPrintAt(3, 7, "$A5 = SA-1 BOOTED OK!");
    } else if (status == 0xFF) {
        textPrintAt(3, 7, "$FF = IRAM ACCESS FAIL");
        textPrintAt(3, 8, "SNES CPU CANT RW IRAM");
    } else if (status == 0x00) {
        textPrintAt(3, 7, "$00 = SA-1 TIMEOUT");
        textPrintAt(3, 8, "SA-1 NEVER WROTE $A5");
    } else if (status == 0x42) {
        textPrintAt(3, 7, "$42 = IRAM NOT CLEARED");
        textPrintAt(3, 8, "SELF-TEST VALUE STUCK");
    } else {
        textPrintAt(3, 7, "UNKNOWN STATUS VALUE");
    }

    setMainScreen(LAYER_BG1);
    textFlush();
    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
