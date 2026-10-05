/**
 * @file main.c
 * @brief Super FX save: a boot counter that survives the power switch, in Game Pak RAM
 * @ingroup examples
 *
 * A Super FX cartridge has one RAM, the GSU's Game Pak RAM at $70:0000: the
 * chip draws into it, and when the board has a battery it is also the save
 * memory (header type $15, `USE_SRAM := 1`; its size is declared at $FFBD,
 * `GSU_RAM_KB`). The sram module addresses it like any other save memory —
 * this example is memory/save_game's call sequence on a Super FX board, with
 * the GSU stopped the whole time.
 *
 * It keeps one number: how many times the console has been switched on.
 * At boot it reads the counter and a magic word; a matching magic means a
 * previous run saved here (a fresh cart reads whatever the RAM powered up
 * with, so the magic decides, not a zero). It adds one, saves, and prints
 * both. Power-cycle: the screen shows one more. The block sits at $E000,
 * above the two 16 KB framebuffers a game would use (chips/superfx_game_skeleton)
 * — a game that presents frames must keep its save out of them.
 *
 * @par SNES Concepts
 * - Game Pak RAM is the GSU's work RAM and the battery-backed save memory
 * - The save block must not overlap the framebuffers the GSU draws into
 * - A magic word tells a saved block from power-on garbage
 *
 * @par What to Observe
 * - "FOUND" is the saved counter (or "NONE" on a fresh cart), "NOW" is
 *   FOUND + 1
 * - Switch the console off and on: NOW increments by one each time
 * - "GSU PRESENT" confirms the header and the chip agree (gsuIsPresent)
 *
 * @par Modules Used
 * console, dma, background, text, sram, superfx
 *
 * @see sram.h, superfx.h, memory/save_game, chips/sa1_save
 */

#include <snes.h>
#include <snes/sram.h>
#include <snes/superfx.h>

/** @brief Marks a block this program wrote */
#define SAVE_MAGIC 0x5F5A
/** @brief Game Pak RAM byte offset of the save block: above two 16 KB framebuffers */
#define SAVE_AT    0xE000

/** @brief The saved block: the magic word, then the counter */
typedef struct {
    u16 magic;  /**< SAVE_MAGIC once written */
    u16 boots;  /**< power-ons counted so far */
} SaveBlock;

/** @brief What the battery held at power-on (read before anything is written) */
SaveBlock found;
/** @brief What this run wrote back */
SaveBlock now;
/** @brief 1 when `found` carried the magic word (a previous run saved here) */
u16 had_save;
/** @brief gsuIsPresent() at boot: 1 on a Super FX cartridge */
u16 gsu_present;

int main(void) {
    consoleInit();
    textModeInit();

    gsu_present = gsuInit();

    sramLoadOffset((u8 *)&found, sizeof(SaveBlock), SAVE_AT);
    had_save = (found.magic == SAVE_MAGIC) ? 1 : 0;

    now.magic = SAVE_MAGIC;
    now.boots = had_save ? (u16)(found.boots + 1) : 1;
    sramSaveOffset((u8 *)&now, sizeof(SaveBlock), SAVE_AT);

    textPrintAt(4, 3, "SUPER FX RAM SAVE");
    textPrintAt(3, 7, had_save ? "FOUND:" : "FOUND: NONE");
    if (had_save) {
        textSetPos(10, 7);
        textPrintU16(found.boots);
    }
    textPrintAt(3, 9, "NOW:");
    textSetPos(10, 9);
    textPrintU16(now.boots);
    textPrintAt(3, 13, gsu_present ? "GSU PRESENT" : "GSU NOT PRESENT");
    textPrintAt(3, 17, "POWER OFF AND ON:");
    textPrintAt(3, 18, "NOW COUNTS ONE MORE");
    textFlush();

    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
