/**
 * @file main.c
 * @brief SA-1 save: a boot counter that survives the power switch, in BW-RAM
 * @ingroup examples
 *
 * On an SA-1 cartridge the battery-backed memory is not the LoROM SRAM at
 * $70:0000 but the SA-1's BW-RAM, seen by the SNES CPU at $40:0000 and
 * writable only once SBWE ($2226) is set — which crt0 does on an SA-1 build.
 * The sram module addresses it for you: this example is the same call
 * sequence as memory/save_game, on the other memory.
 *
 * It keeps one number: how many times the console has been switched on.
 * At boot it reads the counter and a magic word; a matching magic means a
 * previous run saved here (a fresh cart reads whatever the chip powered up
 * with, so the magic decides, not a zero). It adds one, saves, and prints
 * both the value it found and the value it wrote. Power-cycle: the screen
 * shows one more. That is the whole test — and the one a console session
 * needs (docs/HARDWARE_VERIFICATION.md, row 25).
 *
 * @par SNES Concepts
 * - SA-1 BW-RAM as the save memory: $40:0000 for the SNES CPU, SBWE first
 * - A magic word tells a saved block from power-on garbage
 * - The sram module is the same API on LoROM, HiROM, SA-1 and Super FX carts
 *
 * @par What to Observe
 * - "FOUND" is the saved counter (or "NONE" on a fresh cart), "NOW" is
 *   FOUND + 1; both printed as decimal
 * - Switch the console off and on: NOW increments by one each time
 * - "SA-1 READY" confirms the coprocessor booted (sa1IsReady)
 *
 * @par Modules Used
 * console, dma, background, text, sa1, sram
 *
 * @see sram.h, sa1.h, memory/save_game, chips/superfx_save
 */

#include <snes.h>
#include <snes/sram.h>
#include <snes/sa1.h>

/** @brief Marks a block this program wrote (any value a powered-up chip is unlikely to hold) */
#define SAVE_MAGIC 0x5A1E
/** @brief BW-RAM byte offset of the save block */
#define SAVE_AT    0x0000

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
/** @brief sa1IsReady() at boot: 1 on a working SA-1 cartridge */
u16 sa1_ready;

int main(void) {
    consoleInit();
    textModeInit();

    sa1_ready = sa1IsReady();

    /* Read first: on a fresh cartridge this is power-on garbage, which the
     * magic word tells apart from a block we wrote. */
    sramLoadOffset((u8 *)&found, sizeof(SaveBlock), SAVE_AT);
    had_save = (found.magic == SAVE_MAGIC) ? 1 : 0;

    now.magic = SAVE_MAGIC;
    now.boots = had_save ? (u16)(found.boots + 1) : 1;
    sramSaveOffset((u8 *)&now, sizeof(SaveBlock), SAVE_AT);

    textPrintAt(5, 3, "SA-1 BW-RAM SAVE");
    textPrintAt(3, 7, had_save ? "FOUND:" : "FOUND: NONE");
    if (had_save) {
        textSetPos(10, 7);
        textPrintU16(found.boots);
    }
    textPrintAt(3, 9, "NOW:");
    textSetPos(10, 9);
    textPrintU16(now.boots);
    textPrintAt(3, 13, sa1_ready ? "SA-1 READY" : "SA-1 NOT READY");
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
