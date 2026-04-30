/**
 * @file main.c
 * @brief SRAM battery-backed save with multiple save slots
 * @ingroup examples
 *
 * Demonstrates SRAM (Static RAM) persistence on the SNES. SRAM is a
 * small battery-backed memory region (typically 2-32KB) mapped at
 * $70:0000-$7D:FFFF (LoROM) that retains data when the console is
 * powered off. This is how SNES cartridges implement save games.
 *
 * The example uses two save slots at different SRAM offsets, each
 * storing a SaveState struct (player position and camera coordinates).
 * sramSaveOffset() copies RAM data to SRAM, and sramLoadOffset()
 * reads it back. The loaded values are displayed as hex on screen
 * to verify round-trip integrity.
 *
 * The ROM header must declare SRAM support (USE_SRAM := 1) and size
 * (SRAM_SIZE := 3 for 8KB) so emulators and flash carts allocate
 * the battery-backed region.
 *
 * @par SNES Concepts
 * - SRAM battery-backed persistence for save games
 * - Multiple save slots via byte offset addressing
 * - ROM header SRAM size declaration (affects emulator behavior)
 * - Struct serialization to/from SRAM
 *
 * @par What to Observe
 * - Press A to write test data to Slot 1, B to read it back
 * - Press X to write different data to Slot 2, Y to read it back
 * - Loaded values (camX, camY, posX, posY) appear as hex numbers
 * - Data persists across emulator reset if SRAM saving is enabled
 *
 * @par Modules Used
 * console, dma, text, background, sprite, input
 *
 * @see sram.h, console.h, text.h
 */

#include <snes.h>
#include <snes/sram.h>

/**
 * @brief Data structure representing one save slot's persistent state.
 *
 * This struct is serialized directly to/from SRAM as a raw byte block.
 * The SNES has no filesystem -- SRAM is a flat byte array, so structs
 * are written at calculated byte offsets. Total size must match SAVE_SIZE.
 */
typedef struct {
    s16 posX;   /**< Player X position (signed for negative coordinates) */
    s16 posY;   /**< Player Y position */
    u16 camX;   /**< Camera X scroll offset */
    u16 camY;   /**< Camera Y scroll offset */
} SaveState;

/** @brief Write buffer: filled with test data before saving to SRAM */
SaveState vts;
/** @brief Read buffer: populated by sramLoadOffset() when loading from SRAM */
SaveState vtl;
/** @brief Current joypad state (read each frame in the main loop) */
u16 pad0;

/** @brief SRAM byte offset for save slot 1 (starts at byte 0) */
#define SLOT1 0
/** @brief SRAM byte offset multiplier for save slot 2 (starts at byte 8) */
#define SLOT2 1
/** @brief Size in bytes of one save slot (must equal sizeof(SaveState)) */
#define SAVE_SIZE 8

/**
 * @brief Main entry point -- SRAM save/load demo with two slots.
 *
 * Sets up a text display with instructions and enters a loop where
 * each button performs a different SRAM operation:
 * - A: write test values to Slot 1
 * - B: read Slot 1 and display values as hex
 * - X: write different test values to Slot 2
 * - Y: read Slot 2 and display values as hex
 *
 * The SRAM region is battery-backed, so saved data persists across
 * power cycles on real hardware and across emulator sessions (if SRAM
 * saving is enabled in the emulator settings).
 *
 * @return 0 (never reached -- infinite game loop)
 */
int main(void) {
    textModeInit();

    textPrintAt(12, 1, "SRAM TEST");
    textPrintAt(3, 5, "USE A TO WRITE Slot1");
    textPrintAt(3, 7, "USE B TO READ Slot1");
    textPrintAt(3, 9, "USE X TO WRITE Slot2");
    textPrintAt(3, 11, "USE Y TO READ Slot2");

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

        WaitForVBlank();
    }

    return 0;
}
