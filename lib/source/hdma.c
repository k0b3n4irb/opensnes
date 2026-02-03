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
#include <snes/math.h>

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

/*============================================================================
 * HDMA Wave Effect Implementation
 *
 * Uses sine table to create wavy scroll distortion per scanline.
 * HDMA table format for scroll (mode 2 = 1REG_2X):
 *   [1 byte: line count] [2 bytes: scroll value] per entry
 *   [0] = end marker
 *
 * For 224 scanlines: 224 * 3 + 1 = 673 bytes
 *============================================================================*/

#define WAVE_TABLE_SIZE 673
#define VISIBLE_LINES   224

/* Wave state */
static u8 wave_table[WAVE_TABLE_SIZE];
static u8 wave_channel;
static u8 wave_amplitude;
static u8 wave_frequency;
static u8 wave_speed;
static u8 wave_phase;
static u8 wave_active;

void hdmaWaveInit(void) {
    wave_channel = 0;
    wave_amplitude = 0;
    wave_frequency = 1;
    wave_speed = 2;
    wave_phase = 0;
    wave_active = 0;
}

void hdmaWaveH(u8 channel, u8 bg, u8 amplitude, u8 frequency) {
    u8 destReg;

    /* Map BG number to scroll register */
    switch (bg) {
        case 1: destReg = HDMA_DEST_BG1HOFS; break;
        case 2: destReg = HDMA_DEST_BG2HOFS; break;
        case 3: destReg = HDMA_DEST_BG3HOFS; break;
        default: return;
    }

    /* Store wave parameters */
    wave_channel = channel;
    wave_amplitude = amplitude;
    wave_frequency = frequency;
    wave_phase = 0;
    wave_active = 1;

    /* Initialize table with zero scroll */
    hdmaWaveUpdate();

    /* Set up HDMA - table is in RAM (< $8000) so bank $7E will be used */
    hdmaSetup(channel, HDMA_MODE_1REG_2X, destReg, wave_table);
}

void hdmaWaveUpdate(void) {
    u16 i;
    u8 *ptr;
    u8 angle;
    s16 offset;

    if (!wave_active) return;

    ptr = wave_table;

    /* Build HDMA table: each scanline gets a different scroll offset */
    for (i = 0; i < VISIBLE_LINES; i++) {
        /* Calculate wave angle for this scanline */
        /* angle = phase + (scanline * frequency) */
        angle = wave_phase + (u8)(i * wave_frequency);

        /* Get sine value (-256 to 256 in 8.8 fixed) and scale by amplitude */
        /* fixSin returns 8.8 fixed, divide by 256 to get -1 to +1 range */
        /* then multiply by amplitude */
        offset = (fixSin(angle) * wave_amplitude) >> 8;

        /* Write table entry: [line_count=1] [scroll_low] [scroll_high] */
        *ptr++ = 1;                    /* 1 scanline */
        *ptr++ = (u8)(offset & 0xFF);  /* Scroll low byte */
        *ptr++ = (u8)(offset >> 8);    /* Scroll high byte (sign extend) */
    }

    /* End marker */
    *ptr = 0;

    /* Advance phase for animation */
    wave_phase = wave_phase + wave_speed;
}

void hdmaWaveStop(void) {
    if (wave_active) {
        hdmaDisable(1 << wave_channel);
        wave_active = 0;
    }
}

void hdmaWaveSetSpeed(u8 speed) {
    wave_speed = speed;
}
