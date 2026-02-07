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

/*============================================================================
 * HDMA Wave Effect
 *
 * Uses double-buffered RAM tables with 4-line chunks for smooth wave
 * distortion effects. Tables are updated during VBlank while HDMA reads
 * from the other buffer.
 *
 * IMPORTANT: HDMA tables live in bank $7E above $2000 (outside the WRAM
 * mirror). The cc65816 compiler generates sta.l $0000,x for pointer stores,
 * which writes to bank $00. Bank $00 addresses above $1FFF are I/O
 * registers, NOT RAM. So we use the WRAM data port ($2180-$2183) to write
 * to bank $7E directly.
 *
 * Table format: 56 entries of [0x84][scroll_lo][scroll_hi] + [0x00] = 169 bytes
 *============================================================================*/

#include <snes/math.h>

/* External symbols from hdma.asm */
extern u8 hdma_table_a[169];
extern u8 hdma_table_b[169];
extern u8 hdma_active_buffer;
extern u8 hdma_wave_frame;
extern u8 hdma_wave_amplitude;
extern u8 hdma_wave_frequency;
extern u8 hdma_wave_channel;
extern u8 hdma_wave_enabled;
extern u8 hdma_wave_speed;
extern u8 hdma_wave_dest_reg;

/* Number of 4-line chunks: 224 / 4 = 56 */
#define WAVE_CHUNKS 56

/*
 * Channel number to bitmask lookup.
 * The cc65816 compiler does not support variable-count left shifts
 * (1 << variable), so we use a lookup table instead.
 */
static const u8 channel_mask[] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};

/*
 * WRAM data port registers for writing to bank $7E above $2000.
 * $2181/$2182/$2183 = address, $2180 = data (auto-increment).
 */
#define WRAM_DATA   (*(vu8*)0x2180)
#define WRAM_ADDRL  (*(vu8*)0x2181)
#define WRAM_ADDRM  (*(vu8*)0x2182)
#define WRAM_ADDRH  (*(vu8*)0x2183)

/**
 * Fill a wave table in bank $7E via the WRAM data port.
 * Cannot use C pointers because they access bank $00 via sta.l $0000,x.
 */
static void fillWaveTable(u16 tableAddr, u8 frame, u8 amplitude, u8 frequency) {
    u16 i;

    /* Set WRAM write address to bank $7E + tableAddr */
    WRAM_ADDRL = (u8)(tableAddr & 0xFF);
    WRAM_ADDRM = (u8)((tableAddr >> 8) & 0xFF);
    WRAM_ADDRH = 0;  /* Bank $7E (bit 0 = 0) */

    for (i = 0; i < WAVE_CHUNKS; i++) {
        /* Calculate sine value for this chunk (use center of chunk) */
        u8 line = (u8)((i << 2) + 2);  /* i * 4 + 2 = center of chunk */
        u8 angle = (u8)(((u16)line * frequency) + frame);
        s16 sine_val = fixSin(angle);  /* -256 to +256 (8.8 fixed) */

        /* Scale by amplitude: offset = (sine_val * amplitude) >> 8 */
        s16 offset = (s16)(((s32)sine_val * amplitude) >> 8);

        WRAM_DATA = 0x84;                 /* Repeat mode, 4 lines */
        WRAM_DATA = (u8)(offset & 0xFF);  /* Scroll low byte */
        WRAM_DATA = (u8)((offset >> 8) & 0xFF);  /* Scroll high byte */
    }
    WRAM_DATA = 0x00;  /* End marker */
}

void hdmaWaveInit(void) {
    /* Ensure all HDMA is disabled first */
    hdmaDisableAll();

    hdma_active_buffer = 0;
    hdma_wave_frame = 0;
    hdma_wave_amplitude = 8;
    hdma_wave_frequency = 4;
    hdma_wave_channel = 6;
    hdma_wave_enabled = 0;
    hdma_wave_speed = 2;
    hdma_wave_dest_reg = HDMA_DEST_BG1HOFS;

    /* Initialize both buffers with flat scroll (no wave) */
    fillWaveTable((u16)hdma_table_a, 0, 0, 0);
    fillWaveTable((u16)hdma_table_b, 0, 0, 0);
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

    /* Store parameters */
    hdma_wave_channel = channel;
    hdma_wave_amplitude = amplitude;
    hdma_wave_frequency = frequency;
    hdma_wave_dest_reg = destReg;
    hdma_wave_enabled = 1;

    /* Fill both buffers with initial wave */
    fillWaveTable((u16)hdma_table_a, hdma_wave_frame, amplitude, frequency);
    fillWaveTable((u16)hdma_table_b, hdma_wave_frame, amplitude, frequency);

    /* Start with buffer A active */
    hdma_active_buffer = 0;

    /* Setup and enable HDMA */
    hdmaSetup(channel, HDMA_MODE_1REG_2X, destReg, hdma_table_a);
    hdmaEnable(channel_mask[channel]);
}

void hdmaWaveUpdate(void) {
    u16 update_addr;

    if (!hdma_wave_enabled) return;

    /* Advance animation frame based on speed */
    hdma_wave_frame = (u8)(hdma_wave_frame + hdma_wave_speed);

    /* Determine which buffer to update (the one NOT being read by HDMA) */
    if (hdma_active_buffer == 0) {
        /* Buffer A is active, update buffer B */
        update_addr = (u16)hdma_table_b;
    } else {
        /* Buffer B is active, update buffer A */
        update_addr = (u16)hdma_table_a;
    }

    /* Fill the inactive buffer with new wave values via WRAM port */
    fillWaveTable(update_addr, hdma_wave_frame,
                  hdma_wave_amplitude, hdma_wave_frequency);

    /* Swap active buffer by updating the HDMA table pointer */
    if (hdma_active_buffer == 0) {
        hdmaSetTable(hdma_wave_channel, hdma_table_b);
    } else {
        hdmaSetTable(hdma_wave_channel, hdma_table_a);
    }

    /* Toggle active buffer flag */
    hdma_active_buffer = (u8)(hdma_active_buffer ^ 1);
}

void hdmaWaveStop(void) {
    if (!hdma_wave_enabled) return;

    /* Disable HDMA channel */
    hdmaDisable(channel_mask[hdma_wave_channel]);
    hdma_wave_enabled = 0;
}

void hdmaWaveSetSpeed(u8 speed) {
    hdma_wave_speed = speed;
}
