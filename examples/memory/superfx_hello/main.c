/**
 * @file main.c
 * @brief SuperFX (GSU) Hello World — boot diagnostic
 * @ingroup examples
 *
 * Detects the SuperFX coprocessor, launches a minimal GSU program
 * that sets R0 = $CAFE, and reads back the result to verify
 * two-way communication between the SNES CPU and the GSU.
 *
 * @par What to Observe
 * - GSU chip version displayed (VCR register)
 * - GSU R0 result: $CAFE confirms the GSU executed code correctly
 * - If no SuperFX hardware: "NOT DETECTED" message
 *
 * @par Modules Used
 * console, sprite, dma, background, text, superfx
 */

#include <snes.h>
#include <snes/superfx.h>

/** @brief Launch GSU program, wait for completion (defined in gsu_loader.asm) */
extern void launchGSU(void);

/** @brief GSU R0 result after launchGSU() (set by gsu_loader.asm) */
extern u16 gsu_result;

/** @brief Convert a nibble to hex ASCII */
static u8 hexchar(u8 n) {
    return (n < 10) ? ('0' + n) : ('A' + n - 10);
}

int main(void) {
    u8 version;
    u16 result;
    u8 buf[8];

    consoleInit();
    setMode(BG_MODE0, 0);

    setColor(0, 0x0000);
    setColor(1, RGB(31, 31, 31));

    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    textPrintAt(3, 2, "SUPERFX HELLO");
    textPrintAt(3, 3, "GSU COPROCESSOR");

    if (gsuInit()) {
        version = superfx_status;

        buf[0] = '$';
        buf[1] = hexchar(version >> 4);
        buf[2] = hexchar(version & 0x0F);
        buf[3] = 0;
        textPrintAt(3, 6, "GSU VERSION:");
        textPrintAt(16, 6, (char*)buf);

        textPrintAt(3, 8, "LAUNCHING GSU...");
        textFlush();
        WaitForVBlank();

        launchGSU();

        result = gsu_result;
        buf[0] = '$';
        buf[1] = hexchar((result >> 12) & 0x0F);
        buf[2] = hexchar((result >> 8) & 0x0F);
        buf[3] = hexchar((result >> 4) & 0x0F);
        buf[4] = hexchar(result & 0x0F);
        buf[5] = 0;

        textPrintAt(3, 10, "GSU R0 RESULT:");
        textPrintAt(18, 10, (char*)buf);

        if (result == 0xCAFE) {
            textPrintAt(3, 13, "GSU: BOOTED OK");
        } else {
            textPrintAt(3, 13, "GSU: WRONG VALUE!");
        }
    } else {
        textPrintAt(3, 6, "GSU: NOT DETECTED");
        textPrintAt(3, 8, "USE SUPERFX-CAPABLE");
        textPrintAt(3, 9, "EMULATOR (MESEN2)");
    }

    textFlush();

    setMainScreen(LAYER_BG1);
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
