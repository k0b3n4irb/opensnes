/*
 * luna stress probe — open-bus / MDR read behaviour (bank-controlled).
 *
 * The actual reads are in ob.asm, because a C pointer dereference on this
 * target always addresses bank $00 (structural limitation) and so cannot
 * set the open-bus MDR to a chosen bank byte. ob_probe() reads the $2100
 * MMIO mirror through several banks via `lda.l bb:2100`; on hardware the
 * open-bus read returns the bank byte `bb` (the MDR). See ob.asm.
 *
 * Expected (if luna models the MDR): res = 3F 01 20 10 00 00 …
 * If luna returns 0 for open bus:     res = 00 00 00 00 00 00 …
 * Arbiter: fullsnes / anomie open-bus rules.
 *
 * Transitory stress prototype (see .claude/rules/luna_tooling.md).
 */
#include <snes.h>

/** @brief Captured open-bus bytes; written by ob_probe (ob.asm). */
u8 res[16];

/** @brief Absolute-long open-bus reads (asm — C can't leave bank $00). */
extern void ob_probe(void);

int main(void)
{
    consoleInit();
    ob_probe();
    while (1) { }
    return 0;
}
