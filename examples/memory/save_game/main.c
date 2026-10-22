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
 * The screen is a save terminal of the Rue basse: two slot cards on a
 * terminal's glass (BG2), the text on BG1 in front. The example uses two
 * save slots at different SRAM offsets, each storing a SaveState struct
 * (player position and camera coordinates). sramSaveOffset() copies RAM
 * data to SRAM, and sramLoadOffset() reads it back. The loaded values are
 * displayed as hex in the slot's card to verify round-trip integrity, and
 * Kit's icon appears in a card once the slot has been written or read.
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
 * - Mode 0: the text on BG1 and a 2bpp picture on BG2, each with its own
 *   palettes (BG2's start at CGRAM 32)
 *
 * @par What to Observe
 * - Press A to write test data to Slot 1, B to read it back
 * - Press X to write different data to Slot 2, Y to read it back
 * - Loaded values (camX, camY, posX, posY) appear as hex numbers in the
 *   slot's card, with Kit's icon
 * - Data persists across emulator reset if SRAM saving is enabled
 *
 * @par Modules Used
 * console, dma, text, background, sprite, input
 *
 * @see sram.h, console.h, text.h
 */

#include <snes.h>
#include <snes/sram.h>

#include "res/terminal.inc"
#include "res/icon.inc"
/* generated: terminal_tiles/_pal/_map, icon_tiles/_pal (each with _end) */

/** @name VRAM (word addresses): textModeInit() keeps $0000 (font) and $3800 (text map)
 *  @{ */
#define TERM_TILES 0x1000
#define TERM_MAP   0x3C00
#define OBJ_VRAM   0x4000
/** @} */

/** @brief The cards' first text row: slot 1's card is rows 13-16, slot 2's 19-22 */
#define CARD1_ROW 13
#define CARD2_ROW 19

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
/**
 * @brief Write a slot's status in its card, and show Kit's icon there
 *
 * @param row The card's first text row (CARD1_ROW or CARD2_ROW)
 * @param id The slot's sprite (0 or 1): its icon in the card's well
 * @param what "SAVED" or "LOADED"
 * @param s The values to print, or NULL after a save
 */
static void show_slot(u8 row, u8 id, const char *what, const SaveState *s) {
    textPrintAt(8, (u8)(row + 2), "                  ");
    textPrintAt(8, (u8)(row + 3), "                  ");
    textPrintAt(15, (u8)(row + 1), what);
    if (s) {
        textPrintAt(8, (u8)(row + 2), "POS ");
        textPrintHex(s->posX, 4);
        textPrint(" ");
        textPrintHex(s->posY, 4);
        textPrintAt(8, (u8)(row + 3), "CAM ");
        textPrintHex(s->camX, 4);
        textPrint(" ");
        textPrintHex(s->camY, 4);
    }
    oamSet(id, 22, (u16)(row * 8), 0, 0, 3, 0);
}

int main(void) {
    textModeInit();                 /* Mode 0, the text on BG1, font at $0000 */

    /* BG2: the terminal, four colours at CGRAM 32 (Mode 0 gives each BG its
     * own palettes: BG_4COLORS0 puts palette 0 of BG2 there) */
    bgInitTileSet(1, terminal_tiles, terminal_pal, 0,
                  terminal_tiles_end - terminal_tiles,
                  terminal_pal_end - terminal_pal,
                  BG_4COLORS0, TERM_TILES);
    bgSetMapPtr(1, TERM_MAP, SC_32x32);
    dmaCopyVram(terminal_map, TERM_MAP, terminal_map_end - terminal_map);
    setColor(0, RGB(1, 2, 3));      /* the backdrop, round the bezel */

    /* Kit's icon, 32x32 (the LARGE size of OBJ_SIZE16_L32): one sprite a
     * card, off screen until its slot is used */
    oamInitGfxSet(icon_tiles, icon_tiles_end - icon_tiles,
                  icon_pal, icon_pal_end - icon_pal,
                  0, OBJ_VRAM, OBJ_SIZE16_L32);
    oamSetSize(0, OBJ_LARGE);
    oamSetSize(1, OBJ_LARGE);
    oamHide(0);
    oamHide(1);

    setMainScreen(LAYER_BG1 | LAYER_BG2 | LAYER_OBJ);

    textPrintAt(5, 1, "SAVE TERMINAL  RUE BASSE");
    textPrintAt(4, 5, "A  SAVE SLOT 1");
    textPrintAt(4, 6, "B  LOAD SLOT 1");
    textPrintAt(4, 8, "X  SAVE SLOT 2");
    textPrintAt(4, 9, "Y  LOAD SLOT 2");
    textPrintAt(8, CARD1_ROW + 1, "SLOT 1");
    textPrintAt(8, CARD2_ROW + 1, "SLOT 2");

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
            show_slot(CARD1_ROW, 0, "SAVED ", 0);
        }

        if (pad0 == KEY_B) {
            sramLoadOffset((u8 *)&vtl, SAVE_SIZE, SAVE_SIZE * SLOT1);
            show_slot(CARD1_ROW, 0, "LOADED", &vtl);
        }

        if (pad0 == KEY_X) {
            vts.camX = 0xA987;
            vts.camY = 0x6543;
            vts.posX = 0x0002;
            vts.posY = 0x0001;
            sramSaveOffset((u8 *)&vts, SAVE_SIZE, SAVE_SIZE * SLOT2);
            show_slot(CARD2_ROW, 1, "SAVED ", 0);
        }

        if (pad0 == KEY_Y) {
            sramLoadOffset((u8 *)&vtl, SAVE_SIZE, SAVE_SIZE * SLOT2);
            show_slot(CARD2_ROW, 1, "LOADED", &vtl);
        }

        WaitForVBlank();
    }

    return 0;
}
