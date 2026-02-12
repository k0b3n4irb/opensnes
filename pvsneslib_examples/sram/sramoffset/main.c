/*
 * sramoffset - Ported from PVSnesLib to OpenSNES
 *
 * Multi-slot SRAM demo with two save slots.
 * Each slot stores a SaveState struct (posX, posY, camX, camY).
 *
 * A = Save slot 1    B = Load slot 1
 * X = Save slot 2    Y = Load slot 2
 *
 * -- alekmaul (original PVSnesLib example)
 */

#include <snes.h>
#include <snes/sram.h>

typedef struct {
    u16 posX;
    u16 posY;
    u16 camX;
    u16 camY;
} SaveState;

#define SLOT1_OFFSET  0
#define SLOT2_OFFSET  sizeof(SaveState)

SaveState state;
SaveState loaded;

void displaySlot(u8 y, const char *label, u16 px, u16 py, u16 cx, u16 cy) {
    textPrintAt(2, y, label);

    textPrintAt(4, y + 1, "POS  X=0x");
    textPrintHex(px, 4);
    textPrint(" Y=0x");
    textPrintHex(py, 4);

    textPrintAt(4, y + 2, "CAM  X=0x");
    textPrintHex(cx, 4);
    textPrint(" Y=0x");
    textPrintHex(cy, 4);
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

    /* Initialize save state with test values */
    state.posX = 0x0100;
    state.posY = 0x0080;
    state.camX = 0x0040;
    state.camY = 0x0020;

    /* Draw static UI */
    textPrintAt(5, 1, "SRAM OFFSET DEMO");

    textPrintAt(2, 3, "A=SAVE SLOT1  B=LOAD SLOT1");
    textPrintAt(2, 4, "X=SAVE SLOT2  Y=LOAD SLOT2");
    textPrintAt(2, 5, "UP/DN = CHANGE VALUES");

    textPrintAt(2, 7, "CURRENT STATE:");
    displaySlot(8, "STATE TO SAVE:", state.posX, state.posY, state.camX, state.camY);

    displaySlot(12, "SLOT 1 (LOADED):", 0, 0, 0, 0);
    displaySlot(16, "SLOT 2 (LOADED):", 0, 0, 0, 0);

    textFlush();
    WaitForVBlank();
    setScreenOn();

    while (1) {
        pressed = padPressed(0);

        if (pressed & KEY_UP) {
            /* Increment values to make saves distinguishable */
            state.posX = state.posX + 0x10;
            state.posY = state.posY + 0x08;
            state.camX = state.camX + 0x04;
            state.camY = state.camY + 0x02;

            displaySlot(8, "STATE TO SAVE:", state.posX, state.posY, state.camX, state.camY);
            textFlush();
        }

        if (pressed & KEY_DOWN) {
            /* Decrement values */
            state.posX = state.posX - 0x10;
            state.posY = state.posY - 0x08;
            state.camX = state.camX - 0x04;
            state.camY = state.camY - 0x02;

            displaySlot(8, "STATE TO SAVE:", state.posX, state.posY, state.camX, state.camY);
            textFlush();
        }

        if (pressed & KEY_A) {
            /* Save to slot 1 */
            sramSaveOffset((u8 *)&state, sizeof(SaveState), SLOT1_OFFSET);
            textPrintAt(2, 20, "SLOT 1 SAVED!  ");
            textFlush();
        }

        if (pressed & KEY_B) {
            /* Load from slot 1 */
            sramLoadOffset((u8 *)&loaded, sizeof(SaveState), SLOT1_OFFSET);
            displaySlot(12, "SLOT 1 (LOADED):", loaded.posX, loaded.posY, loaded.camX, loaded.camY);
            textPrintAt(2, 20, "SLOT 1 LOADED! ");
            textFlush();
        }

        if (pressed & KEY_X) {
            /* Save to slot 2 */
            sramSaveOffset((u8 *)&state, sizeof(SaveState), SLOT2_OFFSET);
            textPrintAt(2, 20, "SLOT 2 SAVED!  ");
            textFlush();
        }

        if (pressed & KEY_Y) {
            /* Load from slot 2 */
            sramLoadOffset((u8 *)&loaded, sizeof(SaveState), SLOT2_OFFSET);
            displaySlot(16, "SLOT 2 (LOADED):", loaded.posX, loaded.posY, loaded.camX, loaded.camY);
            textPrintAt(2, 20, "SLOT 2 LOADED! ");
            textFlush();
        }

        WaitForVBlank();
    }

    return 0;
}
