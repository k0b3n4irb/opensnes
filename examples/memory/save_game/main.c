/* SRAM Test — Direct port of PVSnesLib sramoffset example
 *
 * A = Write Slot 1 (hardcoded values)
 * B = Read Slot 1
 * X = Write Slot 2 (hardcoded values)
 * Y = Read Slot 2
 */

#include <snes.h>
#include <snes/sram.h>

typedef struct {
    s16 posX, posY;
    u16 camX, camY;
} SaveState;

SaveState vts, vtl;
u16 pad0;

#define SLOT1 0
#define SLOT2 1
#define SAVE_SIZE 8

int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);

    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;
    REG_CGADD = 1;
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;
    REG_TM = TM_BG1;

    textPrintAt(12, 1, "SRAM TEST");
    textPrintAt(3, 5, "USE A TO WRITE Slot1");
    textPrintAt(3, 7, "USE B TO READ Slot1");
    textPrintAt(3, 9, "USE X TO WRITE Slot2");
    textPrintAt(3, 11, "USE Y TO READ Slot2");
    textFlush();

    WaitForVBlank();
    setScreenOn();

    while (1) {
        pad0 = padHeld(0);

        if (pad0 == KEY_A) {
            vts.camX = 0x1234;
            vts.camY = 0x5678;
            vts.posX = 0x0009;
            vts.posY = 0x000A;
            sramSaveOffset((u8 *)&vts, SAVE_SIZE, SAVE_SIZE * SLOT1);
            textPrintAt(2, 13, "SAVE SLOT1          ");
            textPrintAt(2, 14, "                   ");
            textPrintAt(2, 15, "                   ");
        }

        if (pad0 == KEY_B) {
            sramLoadOffset((u8 *)&vtl, SAVE_SIZE, SAVE_SIZE * SLOT1);
            textPrintAt(2, 13, "LOAD SLOT1          ");
            textPrintAt(2, 14, "camX=");
            textPrintHex(vtl.camX, 4);
            textPrintAt(12, 14, "camY=");
            textPrintHex(vtl.camY, 4);
            textPrintAt(2, 15, "posX=");
            textPrintHex(vtl.posX, 4);
            textPrintAt(12, 15, "posY=");
            textPrintHex(vtl.posY, 4);
        }

        if (pad0 == KEY_X) {
            vts.camX = 0xA987;
            vts.camY = 0x6543;
            vts.posX = 0x0002;
            vts.posY = 0x0001;
            sramSaveOffset((u8 *)&vts, SAVE_SIZE, SAVE_SIZE * SLOT2);
            textPrintAt(2, 13, "SAVE SLOT2          ");
            textPrintAt(2, 14, "                   ");
            textPrintAt(2, 15, "                   ");
        }

        if (pad0 == KEY_Y) {
            sramLoadOffset((u8 *)&vtl, SAVE_SIZE, SAVE_SIZE * SLOT2);
            textPrintAt(2, 13, "LOAD SLOT2          ");
            textPrintAt(2, 14, "camX=");
            textPrintHex(vtl.camX, 4);
            textPrintAt(12, 14, "camY=");
            textPrintHex(vtl.camY, 4);
            textPrintAt(2, 15, "posX=");
            textPrintHex(vtl.posX, 4);
            textPrintAt(12, 15, "posY=");
            textPrintHex(vtl.posY, 4);
        }

        textFlush();
        WaitForVBlank();
    }

    return 0;
}
