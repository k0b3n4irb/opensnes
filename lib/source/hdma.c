/**
 * @file hdma.c
 * @brief OpenSNES HDMA Helper Functions
 *
 * High-level HDMA effect helpers. Core functions are in hdma.asm
 * to properly handle 24-bit ROM addresses.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/hdma.h>

/*============================================================================
 * HDMA Effect Helpers
 *
 * These call the assembly core functions (hdmaSetup, hdmaEnable)
 *============================================================================*/

void hdmaParallax(u8 channel, u8 bg, const void *scrollTable) {
    u8 destReg;

    /* Map BG number to scroll register */
    switch (bg) {
        case 1: destReg = HDMA_DEST_BG1HOFS; break;
        case 2: destReg = HDMA_DEST_BG2HOFS; break;
        case 3: destReg = HDMA_DEST_BG3HOFS; break;
        default: return;
    }

    /* Use mode 2 for 2-byte scroll values (written twice to same register) */
    hdmaSetup(channel, HDMA_MODE_1REG_2X, destReg, scrollTable);
}

void hdmaGradient(u8 channel, const void *colorTable) {
    /* Fixed color register uses 1-byte writes */
    hdmaSetup(channel, HDMA_MODE_1REG, HDMA_DEST_COLDATA, colorTable);
}

void hdmaWindowShape(u8 channel, const void *windowTable) {
    /* Window uses 2 consecutive registers (WH0, WH1) */
    hdmaSetup(channel, HDMA_MODE_2REG, HDMA_DEST_WH0, windowTable);
}
