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

    /* Map BG number (0-indexed) to scroll register */
    switch (bg) {
        case 0: destReg = HDMA_DEST_BG1HOFS; break;
        case 1: destReg = HDMA_DEST_BG2HOFS; break;
        case 2: destReg = HDMA_DEST_BG3HOFS; break;
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
 * Wave tables: 224 entries of [0x81][scroll_lo][scroll_hi] + [0x00] = 673 bytes
 * Wave/ripple tables in bank $00 RAM (C pointer writes, no WRAM port needed)
 *============================================================================*/

/*
 * Local 64-entry quarter-wave sine table (8.8 fixed-point, 0-256).
 * Avoids dependency on the math module. Full period = 256 entries.
 * sine_quarter[i] = round(256 * sin(i * pi / 128)) for i in [0..63].
 */
static const u8 sine_quarter[64] = {
      0,   6,  13,  19,  25,  31,  37,  44,
     50,  56,  62,  68,  74,  80,  86,  92,
     98, 103, 109, 115, 120, 126, 131, 136,
    142, 147, 152, 157, 162, 167, 171, 176,
    181, 185, 189, 193, 197, 201, 205, 209,
    212, 216, 219, 222, 225, 228, 231, 234,
    236, 238, 241, 243, 244, 246, 248, 249,
    251, 252, 253, 254, 254, 255, 255, 256
};

/** Local sine function: angle 0-255, returns -256..+256 (8.8 fixed) */
static s16 hdmaSin(u8 angle) {
    u8 idx;
    s16 val;
    if (angle < 64) {
        val = (s16)sine_quarter[angle];
    } else if (angle < 128) {
        idx = (u8)(127 - angle);
        val = (s16)sine_quarter[idx];
    } else if (angle < 192) {
        idx = (u8)(angle - 128);
        val = (s16)(-(s16)sine_quarter[idx]);
    } else {
        idx = (u8)(255 - angle);
        val = (s16)(-(s16)sine_quarter[idx]);
    }
    return val;
}

/* External symbols from hdma.asm */
extern u8 hdma_table_a[673];
extern u8 hdma_table_b[673];
extern u8 hdma_brightness_table[113];
extern u8 hdma_color_table[281];
extern u8 hdma_iris_table_a[673];
extern u8 hdma_iris_table_b[673];
extern u8 hdma_iris_buffer;
extern u8 hdma_active_buffer;
extern u8 hdma_wave_frame;
extern u8 hdma_wave_amplitude;
extern u8 hdma_wave_frequency;
extern u8 hdma_wave_channel;
extern u8 hdma_wave_enabled;
extern u8 hdma_wave_speed;
extern u8 hdma_wave_dest_reg;
extern u8 hdma_wave_mode;

/* Number of 4-line chunks for gradient effects: 224 / 4 = 56 */
#define WAVE_CHUNKS 56

/* Number of 1-scanline entries for wave/ripple effects (full resolution) */
#define WAVE_LINES 224

/* Forward declaration for ripple mode */
static void fillRippleTableRAM(u8 *table, u8 frame, u8 maxAmplitude, u8 frequency);

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
 * Fill a wave table in bank $00 RAM via C pointer writes.
 * Tables live in bank $00 RAMSECTION (address < $2000), accessible by C.
 */
static void fillWaveTable(u8 *table, u8 frame, u8 amplitude, u8 frequency) {
    u16 i;
    u8 *p = table;
    u8 angle = frame;

    for (i = 0; i < WAVE_LINES; i++) {
        s16 sine_val = hdmaSin(angle);  /* -256 to +256 (8.8 fixed) */

        /* Scale by amplitude with rounding: (product + 128) >> 8
         * Without +128, truncation creates horizontal banding artifacts
         * because many adjacent scanlines get the same integer offset. */
        s16 offset = (s16)((sine_val * (s16)amplitude + 128) >> 8);

        *p++ = 0x81;                         /* Repeat mode, 1 line */
        *p++ = (u8)(offset & 0xFF);          /* Scroll low byte */
        *p++ = (u8)((offset >> 8) & 0xFF);   /* Scroll high byte */

        angle += frequency;
    }
    *p = 0x00;  /* End marker */
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
    hdma_wave_mode = 0;

    /* Initialize both buffers with flat scroll (no wave) */
    fillWaveTable((u8 *)hdma_table_a, 0, 0, 0);
    fillWaveTable((u8 *)hdma_table_b, 0, 0, 0);
}

void hdmaWaveH(u8 channel, u8 bg, u8 amplitude, u8 frequency) {
    u8 destReg;

    /* Map BG number (0-indexed) to scroll register */
    switch (bg) {
        case 0: destReg = HDMA_DEST_BG1HOFS; break;
        case 1: destReg = HDMA_DEST_BG2HOFS; break;
        case 2: destReg = HDMA_DEST_BG3HOFS; break;
        default: return;
    }

    /* Store parameters */
    hdma_wave_channel = channel;
    hdma_wave_amplitude = amplitude;
    hdma_wave_frequency = frequency;
    hdma_wave_dest_reg = destReg;
    hdma_wave_mode = 0;

    hdma_table_a[0] = 0x00;
    hdma_active_buffer = 0;
    hdma_wave_enabled = 1;

    hdmaSetupBank(channel, HDMA_MODE_1REG_2X, destReg, hdma_table_a, 0x00);
    hdmaEnable(channel_mask[channel]);
}

void hdmaWaveUpdate(void) {
    if (!hdma_wave_enabled) return;

    /* Advance animation frame based on speed */
    hdma_wave_frame = (u8)(hdma_wave_frame + hdma_wave_speed);

    {
        /* Both wave and ripple: double-buffered bank $00 RAM tables */
        u8 *update_buf;
        if (hdma_active_buffer == 0) {
            update_buf = (u8 *)hdma_table_b;
        } else {
            update_buf = (u8 *)hdma_table_a;
        }

        if (hdma_wave_mode == 1) {
            fillRippleTableRAM(update_buf, hdma_wave_frame,
                               hdma_wave_amplitude, hdma_wave_frequency);
        } else {
            fillWaveTable(update_buf, hdma_wave_frame,
                          hdma_wave_amplitude, hdma_wave_frequency);
        }

        /* Point HDMA to newly written buffer */
        if (hdma_active_buffer == 0) {
            hdmaSetTable(hdma_wave_channel, hdma_table_b);
        } else {
            hdmaSetTable(hdma_wave_channel, hdma_table_a);
        }
        hdma_active_buffer = (u8)(hdma_active_buffer ^ 1);
    }
}

void hdmaWaveStop(void) {
    if (!hdma_wave_enabled) return;

    /* Disable HDMA channel */
    hdmaDisable(channel_mask[hdma_wave_channel]);
    hdma_wave_enabled = 0;

    /* Reset BG scroll offset to 0 so the wave doesn't leave the
     * background shifted after stopping. Write both low and high bytes. */
    *(vu8*)(0x2100 + hdma_wave_dest_reg) = 0x00;
    *(vu8*)(0x2100 + hdma_wave_dest_reg) = 0x00;
}

void hdmaWaveSetSpeed(u8 speed) {
    hdma_wave_speed = speed;
}

/*============================================================================
 * HDMA Effect Helpers - Phase 1
 *
 * All table writes use the WRAM data port ($2180-$2183) because tables
 * live in bank $7E above $2000, unreachable by C pointers (sta.l $0000,x
 * writes to bank $00 where $2000+ is I/O, not RAM).
 *============================================================================*/

/** Set WRAM write address to bank $7E + addr */
static void wramSetAddr(u16 addr) {
    WRAM_ADDRL = (u8)(addr & 0xFF);
    WRAM_ADDRM = (u8)((addr >> 8) & 0xFF);
    WRAM_ADDRH = 0;  /* Bank $7E (bit 0 = 0) */
}

/*--------------------------------------------------------------------------
 * Brightness Gradient
 *--------------------------------------------------------------------------*/

void hdmaBrightnessGradient(u8 channel, u8 topBrightness, u8 bottomBrightness) {
    u16 i;
    u8 top, bot;
    u8 *p;

    top = topBrightness & 0x0F;
    bot = bottomBrightness & 0x0F;

    /* Write directly to bank $00 RAM table (below $2000) */
    p = (u8 *)hdma_brightness_table;

    /* 56 chunks of 4 scanlines, non-repeat mode.
     * Each entry: linecount (4), brightness value.
     * INIDISP is latched — value persists until next HDMA write. */
    for (i = 0; i < WAVE_CHUNKS; i++) {
        u8 line = (u8)((i << 2) + 2);
        u8 bright = (u8)(top + (((s16)bot - (s16)top) * line) / 223);
        *p++ = 4;       /* 4 scanlines per chunk */
        *p++ = bright;  /* INIDISP value (0-15) */
    }
    *p = 0x00;  /* End marker */

    hdmaSetupBank(channel, HDMA_MODE_1REG, HDMA_DEST_INIDISP, hdma_brightness_table, 0x00);
    hdmaEnable(channel_mask[channel]);
}

void hdmaBrightnessGradientStop(u8 channel) {
    hdmaDisable(channel_mask[channel]);
    /* Restore full brightness */
    *(vu8*)0x2100 = 0x0F;
}

/*--------------------------------------------------------------------------
 * Color Gradient (CGRAM per-scanline)
 *--------------------------------------------------------------------------*/

void hdmaColorGradient(u8 channel, u8 colorIndex, u16 topColor, u16 bottomColor) {
    u16 i;
    s16 topR, topG, topB;
    s16 botR, botG, botB;

    topR = (s16)(topColor & 0x1F);
    topG = (s16)((topColor >> 5) & 0x1F);
    topB = (s16)((topColor >> 10) & 0x1F);
    botR = (s16)(bottomColor & 0x1F);
    botG = (s16)((bottomColor >> 5) & 0x1F);
    botB = (s16)((bottomColor >> 10) & 0x1F);

    wramSetAddr((u16)hdma_color_table);

    /* 56 chunks of 4 scanlines each = 224 scanlines.
     * Mode 2REG_2X to CGADD ($2121): writes CGADD, CGADD, CGDATA, CGDATA.
     * Non-repeat: write once per group, color latches until next write. */
    for (i = 0; i < WAVE_CHUNKS; i++) {
        s16 line = (s16)((i << 2) + 2);
        u8 r = (u8)(topR + ((botR - topR) * line) / 223);
        u8 g = (u8)(topG + ((botG - topG) * line) / 223);
        u8 b = (u8)(topB + ((botB - topB) * line) / 223);
        u16 color = (u16)(((u16)b << 10) | ((u16)g << 5) | (u16)r);

        WRAM_DATA = 4;                          /* 4 scanlines, non-repeat */
        WRAM_DATA = colorIndex;                  /* CGADD low */
        WRAM_DATA = 0x00;                        /* CGADD high */
        WRAM_DATA = (u8)(color & 0xFF);          /* CGDATA low */
        WRAM_DATA = (u8)((color >> 8) & 0xFF);   /* CGDATA high */
    }
    WRAM_DATA = 0x00;  /* End marker */

    hdmaSetup(channel, HDMA_MODE_2REG_2X, HDMA_DEST_CGADD, hdma_color_table);
    hdmaEnable(channel_mask[channel]);
}

void hdmaColorGradientStop(u8 channel) {
    hdmaDisable(channel_mask[channel]);
    /* Note: CGRAM retains per-scanline gradient values after HDMA stops.
     * Caller must restore the original palette if needed — the library
     * cannot know what the original colors were. */
}

/*--------------------------------------------------------------------------
 * Iris Wipe (circular window)
 *--------------------------------------------------------------------------*/

static u16 isqrt(u16 n) {
    u16 result = 0;
    u16 bit = 0x4000;
    while (bit > n) bit >>= 2;
    while (bit != 0) {
        if (n >= result + bit) {
            n = (u16)(n - result - bit);
            result = (u16)((result >> 1) + bit);
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

/**
 * Fill an iris table buffer using C pointer writes.
 * Tables are in bank $00 RAM (< $2000), directly accessible from C.
 * Writes 224 entries of [0x81][WH0][WH1] + terminator.
 */
static void fillIrisTable(u8 *table, u8 centerX, u8 centerY, u16 r2) {
    u16 y;
    u8 *p = table;

    for (y = 0; y < 224; y++) {
        s16 dy = (s16)y - (s16)centerY;
        u16 dy2 = (u16)((s16)(dy * dy));
        u8 wh0, wh1;

        if (dy2 >= r2) {
            /* Outside circle: WH0 > WH1 = empty window = fully masked */
            wh0 = 0xFF;
            wh1 = 0x00;
        } else {
            u16 dx = isqrt(r2 - dy2);
            s16 left = (s16)centerX - (s16)dx;
            s16 right = (s16)centerX + (s16)dx;
            wh0 = (left < 0) ? 0 : (u8)left;
            wh1 = (right > 255) ? 255 : (u8)right;
        }

        *p++ = 0x81;  /* Repeat 1 line */
        *p++ = wh0;   /* WH0 (left edge) */
        *p++ = wh1;   /* WH1 (right edge) */
    }
    *p = 0x00;  /* End marker */
}

void hdmaIrisWipe(u8 channel, u8 layers, u8 centerX, u8 centerY, u8 radius) {
    u16 r2 = (u16)radius * (u16)radius;
    u8 *build_table;

    /* Configure window registers from layers bitmask.
     * Window 1 enable + invert for each requested layer:
     * inside circle = visible, outside = masked. */
    u8 w12sel = 0;
    u8 w34sel = 0;
    u8 wobjsel = 0;
    if (layers & 0x01) w12sel  |= 0x03;  /* BG1: W1 enable + invert */
    if (layers & 0x02) w12sel  |= 0x30;  /* BG2: W1 enable + invert */
    if (layers & 0x04) w34sel  |= 0x03;  /* BG3: W1 enable + invert */
    if (layers & 0x08) w34sel  |= 0x30;  /* BG4: W1 enable + invert */
    if (layers & 0x10) wobjsel |= 0x03;  /* OBJ: W1 enable + invert */

    /* Build into the inactive buffer while HDMA reads the active one.
     * Window masking is NOT enabled yet — avoids artifacts from stale
     * WH0/WH1 during the slow fillIrisTable computation. */
    if (hdma_iris_buffer == 0) {
        build_table = hdma_iris_table_b;
    } else {
        build_table = hdma_iris_table_a;
    }

    fillIrisTable(build_table, centerX, centerY, r2);

    /* Setup and enable HDMA to drive WH0/WH1 per scanline.
     * Use bank $00 explicitly — tables are in bank $00 RAMSECTION. */
    hdmaSetupBank(channel, HDMA_MODE_2REG, HDMA_DEST_WH0, build_table, 0x00);
    hdmaEnable(channel_mask[channel]);

    /* Wait for HDMA to initialize (happens at start of VBlank).
     * Only THEN enable window masking — ensures WH0/WH1 are being
     * driven by HDMA before the PPU uses them for clipping. */
    WaitForVBlank();

    REG_W12SEL  = w12sel;
    REG_W34SEL  = w34sel;
    REG_WOBJSEL = wobjsel;
    REG_TMW     = layers;

    /* Toggle active buffer */
    hdma_iris_buffer = (u8)(hdma_iris_buffer ^ 1);
}

void hdmaIrisWipeStop(u8 channel) {
    hdmaDisable(channel_mask[channel]);

    /* Restore all window registers to fully open */
    REG_W12SEL  = 0x00;
    REG_W34SEL  = 0x00;
    REG_WOBJSEL = 0x00;
    REG_TMW     = 0x00;
}

/*--------------------------------------------------------------------------
 * Water Ripple (full-screen, amplitude increases top-to-bottom)
 *--------------------------------------------------------------------------*/

/**
 * Fill a ripple table in bank $00 RAM using C pointers.
 * Per-scanline resolution (224 entries). Amplitude scales linearly
 * from 0 (top) to maxAmplitude (bottom).
 */
static void fillRippleTableRAM(u8 *table, u8 frame, u8 maxAmplitude, u8 frequency) {
    u16 i;
    u8 *p = table;
    u8 angle = frame;
    u16 amp_step = (maxAmplitude != 0) ? (u16)(((u16)maxAmplitude << 8) / 223) : 0;
    u16 amp_accum = 0;

    for (i = 0; i < WAVE_LINES; i++) {
        s16 sine_val = hdmaSin(angle);
        u8 amp = (u8)(amp_accum >> 8);
        s16 offset = (s16)((sine_val * (s16)amp + 128) >> 8);

        *p++ = 0x81;
        *p++ = (u8)(offset & 0xFF);
        *p++ = (u8)((offset >> 8) & 0xFF);

        angle += frequency;
        amp_accum += amp_step;
    }
    *p = 0x00;
}

void hdmaWaterRipple(u8 channel, u8 bg, u8 amplitude, u8 speed) {
    u8 destReg;

    switch (bg) {
        case 0: destReg = HDMA_DEST_BG1HOFS; break;
        case 1: destReg = HDMA_DEST_BG2HOFS; break;
        case 2: destReg = HDMA_DEST_BG3HOFS; break;
        default: return;
    }

    /* Cap amplitude to prevent s16 overflow in sine_val * amp */
    if (amplitude > 60) amplitude = 60;

    hdma_wave_channel = channel;
    hdma_wave_amplitude = amplitude;
    hdma_wave_frequency = 6;  /* Higher frequency for water-like appearance */
    hdma_wave_dest_reg = destReg;
    hdma_wave_speed = speed;
    hdma_wave_frame = 0;
    hdma_wave_mode = 1;  /* Ripple mode */

    hdma_table_a[0] = 0x00;
    hdma_active_buffer = 0;
    hdma_wave_enabled = 1;

    hdmaSetupBank(channel, HDMA_MODE_1REG_2X, destReg, hdma_table_a, 0x00);
    hdmaEnable(channel_mask[channel]);
}
