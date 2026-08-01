/**
 * @file main.c
 * @brief SA-1 boot diagnostic (Phase 1)
 * @ingroup examples
 *
 * Minimal SA-1 readiness check. The SA-1 coprocessor (Super Accelerator 1)
 * runs the 65816 ISA at 10.74 MHz alongside the main CPU's 3.58 MHz. This
 * example boots the SA-1, has it write a status byte to shared I-RAM
 * (`$3000-$37FF`), then reports the byte's value on screen — proving that
 * the SA-1 coprocessor is running and the main CPU can read its output.
 *
 * @par SNES Concepts
 * - SA-1 vs 65816 dual-CPU: same ISA, different clock, separate memory map
 * - Shared I-RAM as the dispatch / status channel
 * - The `sa1_boot.asm` per-example entry point (each SA-1 ROM brings its own)
 * - Pre-A6+A7 ABI quirks: the SA-1 wrapper API is minimal, all coordination
 *   happens in assembly today
 *
 * @par What to Observe
 * - On boot you see "SA1 status: NN" where NN is the byte the SA-1
 *   wrote to its shared status slot. Any non-zero value confirms the
 *   coprocessor ran. The expected value here is `0xA5` (set by the
 *   bundled `sa1_boot.asm`).
 * - If status is 0x00, the SA-1 didn't run — check that the emulator
 *   recognises SA-1 from the ROM header (Mesen2 is reliable; some
 *   snes9x builds miss it).
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input, sa1
 *
 * @see sa1.h, .claude/notes/tech/sa1_phase1_diagnostic.md
 */

#include <snes.h>
#include <snes/sa1.h>

extern u8 sa1_status;

int main(void) {
    u8 status;

    textModeInit();

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
    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
