/**
 * @file hdma.h
 * @brief SNES HDMA (Horizontal-blanking DMA)
 *
 * HDMA transfers data to PPU registers once per scanline during HBlank,
 * enabling effects like color gradients, parallax scrolling, and wave
 * distortion that change across the screen.
 *
 * ## How HDMA Works
 *
 * HDMA uses a table in memory that specifies:
 * - How many scanlines to apply a value
 * - The value(s) to write to the destination register
 *
 * Each entry in the table is:
 * - 1 byte: Line count (bit 7 = repeat mode, bits 0-6 = count)
 * - N bytes: Data to write (1-4 bytes depending on transfer mode)
 *
 * ## Table Format
 *
 * Non-repeat mode (bit 7 = 0): Write data ONCE, then skip N-1 scanlines
 * ```
 * .db 32       ; Write data once, hold for 32 scanlines
 * .db $1F, $00 ; Data (2 bytes for transfer mode 1)
 * .db 16       ; Write data once, hold for 16 scanlines
 * .db $1F, $08 ; Data
 * .db 0        ; End of table
 * ```
 * Use for registers that hold their value (COLDATA, CGADD).
 * Efficient: 1 data set per group, regardless of line count.
 *
 * Repeat mode (bit 7 = 1): the entry carries ONE DATA SET PER SCANLINE —
 * N lines means N data sets follow the count byte, each written on its line.
 * ```
 * .db $82      ; repeat, 2 lines: TWO data sets follow
 * .db $1F, $00 ; line 1
 * .db $1F, $08 ; line 2
 * .db $81      ; repeat, 1 line: one data set (the common per-line form)
 * .db $1F, $10
 * .db 0        ; End of table
 * ```
 * Use it when the value changes every line (a wave, a perspective table).
 * A table of `$81, data` triples is the simplest per-scanline layout.
 * (Until 2026-09-20 this block showed `$82` followed by a single data set:
 * the hardware would have read the next entry's count byte as line 2's data.)
 *
 * ## Usage Example
 *
 * @code
 * // Define HDMA table in ROM
 * const u8 gradient_table[] = {
 *     32, 0x00,    // 32 lines: color 0
 *     32, 0x08,    // 32 lines: color 8
 *     32, 0x10,    // 32 lines: color 16
 *     0            // End
 * };
 *
 * // Set up HDMA channel 6 to write to fixed color register
 * hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, 0x32, gradient_table);
 * hdmaEnableMask(1 << HDMA_CHANNEL_6);   // a MASK, not a channel number
 *
 * // In main loop, HDMA runs automatically each frame
 * @endcode
 *
 * @warning **Do NOT use HDMA_CHANNEL_7** — the NMI handler uses DMA channel 7
 *          for OAM transfers every frame, which destroys any HDMA setup on that
 *          channel. Safe HDMA channels: 1-6 (channel 0 is used by dmaCopyVram).
 *          On a Super FX build, gsuSetupHdmaBlanking() takes **channel 1**
 *          for its INIDISP letterbox while the bands are on (superfx.h);
 *          pick 2-6 there.
 * @note HDMA tables must be in ROM or bank $7E RAM.
 *
 * ## Bank byte
 *
 * The table pointer is a 4-byte far pointer (chantier A6, v0.19.0):
 * hdmaSetup() programs the channel's source bank from the pointer's bank
 * byte, so a table in any ROM bank (the asset banks, where C const data
 * lives since #127.3) or in bank $7E RAM works as is. A table assembled
 * outside C is a far pointer too (its label carries the bank); the
 * explicit-bank form of 0.x was removed on 2026-10-05.
 *
 * ## Which mode for which register
 *
 * Any PPU register HOLDS the value HDMA wrote until something writes it
 * again, scroll registers included: a non-repeat entry (`32, lo, hi`) writes
 * once and the value stays for the 32 lines. Repeat mode is not about the
 * register "forgetting" — it is needed exactly when the DATA differs from
 * line to line, because only repeat entries carry per-line data.
 *
 * This section said the opposite until 2026-09-20 ("scroll registers require
 * repeat mode … the value is lost"), with an example (`$A0, $20, $00`) that
 * was itself malformed — a 32-line repeat entry needs 32 data sets. The claim
 * came from a misread table format; the references state it plainly:
 * snesdev-wiki, "DMA registers / HDMA table format" ("$01-$80: write once,
 * then wait for X scanlines; $81-$FF: write every scanline for X-$80
 * scanlines"; in repeat mode "the total size of the data section is the
 * number of scanlines multiplied by the number of bytes in the pattern"), and
 * anomie's register doc, same wording. See also
 * `.claude/rules/hardware_claims.md`.
 *
 * - constant over a band (a colour band, a fixed split scroll): non-repeat
 * - different every line (wave, Mode 7 perspective): repeat, `$81` per line
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_HDMA_H
#define OPENSNES_HDMA_H

#include <snes/types.h>

/* Removed on 2026-10-05 (1.0 plan, lot C): hdmaSetupBank.
 * The replacements are in docs/UPGRADING.md; `make check-upgrade` names them. */

/*============================================================================
 * HDMA Channel Definitions
 *============================================================================*/

/** @brief HDMA channel 0 (conflicts with common DMA usage) */
#define HDMA_CHANNEL_0  0
/** @brief HDMA channel 1 */
#define HDMA_CHANNEL_1  1
/** @brief HDMA channel 2 */
#define HDMA_CHANNEL_2  2
/** @brief HDMA channel 3 */
#define HDMA_CHANNEL_3  3
/** @brief HDMA channel 4 */
#define HDMA_CHANNEL_4  4
/** @brief HDMA channel 5 */
#define HDMA_CHANNEL_5  5
/** @brief HDMA channel 6 (recommended for HDMA — never used by the runtime) */
#define HDMA_CHANNEL_6  6
/** @brief HDMA channel 7 — **reserved for the OAM DMA**, do not use for HDMA
 *  (see the @warning at the top of this header) */
#define HDMA_CHANNEL_7  7

/*============================================================================
 * HDMA Transfer Modes
 *============================================================================*/

/**
 * @brief HDMA mode: 1 register, 1 byte
 *
 * Writes 1 byte to destination register each scanline.
 * Table entry: 1 byte line count + 1 byte data
 */
#define HDMA_MODE_1REG      0x00

/**
 * @brief HDMA mode: 2 registers, 2 bytes (low/high)
 *
 * Writes 2 bytes to consecutive registers (e.g., scroll low/high).
 * Table entry: 1 byte line count + 2 bytes data
 */
#define HDMA_MODE_2REG      0x01

/**
 * @brief HDMA mode: 1 register, 2 bytes written twice
 *
 * Writes 2 bytes to same register (for double-write registers).
 * Table entry: 1 byte line count + 2 bytes data
 */
#define HDMA_MODE_1REG_2X   0x02

/**
 * @brief HDMA mode: 2 registers, 4 bytes (2 to each)
 *
 * Writes 4 bytes: 2 to dest, 2 to dest+1.
 * Table entry: 1 byte line count + 4 bytes data
 */
#define HDMA_MODE_2REG_2X   0x03

/**
 * @brief HDMA mode: 4 registers, 4 bytes
 *
 * Writes 4 bytes to 4 consecutive registers.
 * Table entry: 1 byte line count + 4 bytes data
 */
#define HDMA_MODE_4REG      0x04

/**
 * @brief Indirect HDMA flag (DMAP bit 6) — use hdmaSetupIndirect()
 *
 * When set, the table contains [count][ptr16] entries whose pointers
 * reference the actual data. Useful for large or dynamic per-line data
 * (e.g. a full CGRAM gradient per scanline).
 *
 * @warning Do NOT simply OR this into the mode of hdmaSetup(): indirect
 * transfers also need the indirect DATA BANK register ($43x7), which
 * plain hdmaSetup never programs — the pointers would dereference an
 * unprogrammed bank. hdmaSetupIndirect() sets both the bit and $43x7.
 */
#define HDMA_INDIRECT       0x40

/*============================================================================
 * Common Destination Registers
 *============================================================================*/

/** @brief Destination: CGRAM address ($2121) - for palette effects */
#define HDMA_DEST_CGADD     0x21

/** @brief Destination: CGRAM data ($2122) - for color effects */
#define HDMA_DEST_CGDATA    0x22

/** @brief Destination: BG1 H scroll ($210D) */
#define HDMA_DEST_BG1HOFS   0x0D

/** @brief Destination: BG1 V scroll ($210E) */
#define HDMA_DEST_BG1VOFS   0x0E

/** @brief Destination: BG2 H scroll ($210F) */
#define HDMA_DEST_BG2HOFS   0x0F

/** @brief Destination: BG2 V scroll ($2110) */
#define HDMA_DEST_BG2VOFS   0x10

/** @brief Destination: BG3 H scroll ($2111) */
#define HDMA_DEST_BG3HOFS   0x11

/** @brief Destination: BG3 V scroll ($2112) */
#define HDMA_DEST_BG3VOFS   0x12

/** @brief Destination: Window 1 left ($2126) */
#define HDMA_DEST_WH0       0x26

/** @brief Destination: Window 1 right ($2127) */
#define HDMA_DEST_WH1       0x27

/** @brief Destination: Fixed color ($2132) */
#define HDMA_DEST_COLDATA   0x32

/** @brief Destination: INIDISP brightness ($2100) */
#define HDMA_DEST_INIDISP   0x00

/** @brief Destination: BGMODE ($2105) — mid-frame BG mode switch (F-Zero split) */
#define HDMA_DEST_BGMODE    0x05

/** @brief Destination: TM main-screen layer enable ($212C) */
#define HDMA_DEST_TM        0x2C

/** @brief Destination: Mode 7 matrix A ($211B) */
#define HDMA_DEST_M7A       0x1B

/** @brief Destination: Mode 7 matrix B ($211C) */
#define HDMA_DEST_M7B       0x1C

/** @brief Destination: Mode 7 matrix C ($211D) */
#define HDMA_DEST_M7C       0x1D

/** @brief Destination: Mode 7 matrix D ($211E) */
#define HDMA_DEST_M7D       0x1E

/*============================================================================
 * Core HDMA Functions
 *============================================================================*/

/**
 * @brief Set up an HDMA channel
 *
 * Configures an HDMA channel with the specified parameters. The channel
 * is NOT enabled automatically - call hdmaEnableMask() to start it.
 *
 * @param channel HDMA channel (0-7, use HDMA_CHANNEL_6 or lower — 7 belongs to the NMI OAM DMA)
 * @param mode Transfer mode (HDMA_MODE_*)
 * @param destReg Destination B-bus register (low byte of $21xx address)
 * @param table Pointer to HDMA table in ROM or RAM
 *
 * @code
 * hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, HDMA_DEST_COLDATA, my_table);
 * hdmaEnableMask(1 << HDMA_CHANNEL_6);   // a MASK, not a channel number
 * @endcode
 */
void hdmaSetup(u8 channel, u8 mode, u8 destReg, const void *table);

/**
 * @brief Configure an INDIRECT HDMA channel (table of pointers to data)
 *
 * Table entries are [count][ptr_lo][ptr_hi]: each 16-bit pointer
 * references `count` lines worth of data in `dataBank`. The INDIRECT
 * bit is OR'd into `mode` internally — pass a plain HDMA_MODE_*.
 * The table's own bank comes from the far pointer (any bank works);
 * `dataBank` is the bank of the POINTED-TO data ($43x7).
 *
 * Classic use: per-scanline CGRAM reloads (HiColor-style gradients) —
 * each table entry points at a block of palette data for that line.
 *
 * @param channel  HDMA channel (0-7)
 * @param mode     Transfer pattern (HDMA_MODE_*, without HDMA_INDIRECT)
 * @param destReg  Destination register (HDMA_DEST_*)
 * @param table    Indirect table ([count][ptr16] entries + 0 terminator)
 * @param dataBank Bank byte of the data the pointers reference. For a C
 *                 array, extract it from the far pointer:
 *                 `(u8)((u32)(void *)my_data >> 16)` (post-A6 pointers
 *                 carry the bank in byte 2) — RAM arrays give 0.
 */
void hdmaSetupIndirect(u8 channel, u8 mode, u8 destReg, const void *table,
                       u8 dataBank);

/**
 * @brief Enable HDMA channel(s), given as a bit mask
 *
 * Enables the specified HDMA channel(s). The channel runs from the next
 * HBlank: hdmaSetup() has initialised its table address and line counter
 * by hand, which the hardware only does by itself at the start of a frame
 * (anomie-regs, "DMA and HDMA"). snesdev-wiki advises writing HDMAEN during
 * VBlank while the screen is on; an effect enabled mid-frame shows from
 * the next line. The one pair of this header that takes a mask where every
 * other function takes a channel number: the name says so.
 *
 * @param channelMask Bitmask of channels to enable (1 << channel)
 *
 * @code
 * hdmaEnableMask(1 << HDMA_CHANNEL_6);              // Enable channel 6
 * hdmaEnableMask((1 << HDMA_CHANNEL_6) | (1 << HDMA_CHANNEL_5)); // Enable 6 and 5
 * @endcode
 */
void hdmaEnableMask(u8 channelMask);

/**
 * @brief Disable HDMA channel(s), given as a bit mask
 *
 * Disables the specified HDMA channel(s).
 *
 * @param channelMask Bitmask of channels to disable (1 << channel)
 */
void hdmaDisableMask(u8 channelMask);

/**
 * @brief The pre-2026-10-03 name of hdmaEnableMask(). Takes a MASK.
 *
 * Deprecated so that the name can come back at 1.0 taking a channel number,
 * like the rest of this header. Until then it is the same function.
 */
OPENSNES_DEPRECATED("use hdmaEnableMask() — at 1.0 hdmaEnable() will take a channel number")
void hdmaEnable(u8 channelMask);

/** @brief The pre-2026-10-03 name of hdmaDisableMask(). Takes a MASK. */
OPENSNES_DEPRECATED("use hdmaDisableMask() — at 1.0 hdmaDisable() will take a channel number")
void hdmaDisable(u8 channelMask);

/**
 * @brief Disable all HDMA channels
 *
 * Convenience function to stop all HDMA activity.
 */
void hdmaDisableAll(void);

/**
 * @brief Get currently enabled HDMA channels
 *
 * @return Bitmask of enabled channels
 */
u8 hdmaGetEnabled(void);

/**
 * @brief Update HDMA table pointer (for dynamic effects)
 *
 * Changes the table pointer for an already-configured channel.
 * Takes effect on the next frame.
 *
 * @param channel HDMA channel (0-7)
 * @param table New table pointer
 */
void hdmaSetTable(u8 channel, const void *table);

/*============================================================================
 * HDMA Effect Helpers
 *============================================================================*/

/**
 * @brief Set up a background parallax scroll effect
 *
 * Creates horizontal parallax scrolling where each section of the screen
 * scrolls at a different speed based on the scroll table.
 *
 * @param channel HDMA channel to use
 * @param bg Background layer (0=BG1, 1=BG2, 2=BG3)
 * @param scrollTable HDMA table with scroll values
 *
 * @note Table format: line count + 2 bytes (scroll low/high) per entry
 */
void hdmaParallax(u8 channel, u8 bg, const void *scrollTable);

/**
 * @brief Set up a fixed color gradient effect
 *
 * Creates a vertical color gradient by changing the fixed color register
 * per scanline. Useful for sky gradients, underwater effects, etc.
 *
 * @param channel HDMA channel to use
 * @param colorTable HDMA table with COLDATA values
 *
 * @note Table format: line count + 1 byte (COLDATA value) per entry
 * @note COLDATA format: bits 7-5 = color select (RGB), bits 4-0 = intensity
 */
void hdmaGradient(u8 channel, const void *colorTable);

/**
 * @brief Set up window position HDMA for shape effects
 *
 * Uses HDMA to change window boundaries per scanline, creating shapes
 * like circles, triangles, or custom masks.
 *
 * @param channel HDMA channel to use
 * @param windowTable HDMA table with left/right pairs
 *
 * @note Table format: line count + 2 bytes (left, right) per entry
 * @note Uses mode 2REG to write both WH0 and WH1
 */
void hdmaWindowShape(u8 channel, const void *windowTable);

/*============================================================================
 * HDMA Wave Effect Functions
 *============================================================================*/

/**
 * @brief Initialize HDMA wave effect system
 *
 * Must be called once before using wave effects. Resets the wave state (the
 * buffers are static — nothing is allocated).
 *
 * @warning It starts with hdmaDisableAll(): EVERY HDMA channel is switched
 *          off, yours included. Call it before setting up your own channels.
 */
void hdmaWaveInit(void);

/**
 * @brief Set up horizontal wave effect (water reflection)
 *
 * Creates a wavy horizontal distortion, commonly used for:
 * - Water reflections
 * - Heat shimmer
 * - Dream/flashback sequences
 *
 * @param channel HDMA channel to use (6 or lower; 7 belongs to the NMI OAM DMA)
 * @param bg Background layer to affect (0=BG1, 1=BG2, 2=BG3)
 * @param amplitude Wave amplitude in pixels (1-60, clamped internally)
 * @param frequency Wave frequency (1-16, higher = tighter waves). Period = 256/frequency scanlines.
 *
 * @code
 * hdmaWaveInit();
 * hdmaWaveH(HDMA_CHANNEL_6, 0, 4, 4);  // Gentle water reflection on BG1
 * hdmaEnableMask(1 << HDMA_CHANNEL_6);
 *
 * while (1) {
 *     WaitForVBlank();
 *     hdmaWaveUpdate();  // Animate the wave
 * }
 * @endcode
 */
void hdmaWaveH(u8 channel, u8 bg, u8 amplitude, u8 frequency);

/**
 * @brief Update wave animation
 *
 * Call this once per frame (after WaitForVBlank) to animate
 * the wave effect. Updates the HDMA table with new wave values.
 *
 * @note Only needed if wave effects are active
 */
void hdmaWaveUpdate(void);

/**
 * @brief Stop wave effect and disable HDMA channel
 *
 * The wave table writes ABSOLUTE offsets into the layer's HOFS, over the
 * value bgSetScroll() set; while it runs, the layer's own scroll is not
 * applied. Stopping marks the layer's scroll dirty so the NMI rewrites it
 * from the shadows at the next VBlank (until 2026-10-03 it wrote 0).
 *
 * Disables the wave effect and frees the HDMA channel.
 */
void hdmaWaveStop(void);

/**
 * @brief Set wave speed
 *
 * @param speed Animation speed (1=slow, 4=fast, default=2)
 */
void hdmaWaveSetSpeed(u8 speed);

/*============================================================================
 * HDMA Brightness Gradient
 *============================================================================*/

/**
 * @brief Create a vertical brightness gradient
 *
 * Smoothly fades screen brightness from top to bottom using HDMA
 * on the INIDISP register ($2100). Useful for:
 * - Fade-to-black at screen bottom
 * - Spotlight / vignette effects
 * - Underwater depth dimming
 *
 * @param channel HDMA channel (6 or lower; 7 belongs to the NMI OAM DMA)
 * @param topBrightness Brightness at top of screen (0-15, 15=full)
 * @param bottomBrightness Brightness at bottom of screen (0-15)
 *
 * @code
 * hdmaBrightnessGradient(HDMA_CHANNEL_5, 15, 0);  // Fade to black
 * @endcode
 *
 * Called again while the effect runs, it only moves the channel's table
 * pointer, which HDMA reads at the next frame: the change lands whole.
 * (Until 2026-10-03 the channel was set up again, which restarted the table
 * at the next HBlank — one frame with the top of the table on the bottom of
 * the screen.)
 */
void hdmaBrightnessGradient(u8 channel, u8 topBrightness, u8 bottomBrightness);

/**
 * @brief Stop brightness gradient and restore full brightness
 *
 * @param channel The channel used for the gradient
 */
void hdmaBrightnessGradientStop(u8 channel);

/*============================================================================
 * HDMA Color Gradient
 *============================================================================*/

/**
 * @brief Create a per-scanline CGRAM color gradient
 *
 * Smoothly interpolates a palette color from one value to another across
 * the screen. Uses HDMA to rewrite a CGRAM entry per scanline. Useful for:
 * - Sky color gradients (blue to orange sunset)
 * - Water depth color shifts
 * - Background atmosphere effects
 *
 * @param channel HDMA channel (6 or lower; 7 belongs to the NMI OAM DMA)
 * @param colorIndex CGRAM color index to modify (0-255)
 * @param topColor 15-bit SNES color at top of screen (use RGB() macro)
 * @param bottomColor 15-bit SNES color at bottom of screen
 *
 * @code
 * // Blue sky fading to orange at horizon
 * hdmaColorGradient(HDMA_CHANNEL_6, 0,
 *                   RGB(4, 8, 28),    // Deep blue
 *                   RGB(28, 16, 4));   // Orange
 * @endcode
 *
 * Called again while the effect runs, it only moves the channel's table
 * pointer, which HDMA reads at the next frame: the change lands whole.
 * (Until 2026-10-03 the channel was set up again, which restarted the table
 * at the next HBlank — one frame with the top of the table on the bottom of
 * the screen.)
 */
void hdmaColorGradient(u8 channel, u8 colorIndex, u16 topColor, u16 bottomColor);

/**
 * @brief Stop color gradient effect
 *
 * @param channel The channel used for the gradient
 */
void hdmaColorGradientStop(u8 channel);

/*============================================================================
 * HDMA Iris Wipe (Circular Window)
 *============================================================================*/

/**
 * @brief Create a circular window mask (iris/spotlight effect)
 *
 * Uses HDMA to drive window registers (WH0/WH1) per scanline,
 * approximating a circle. Configures all window registers automatically.
 * Useful for:
 * - Scene transitions (iris in/out)
 * - Spotlight effects
 * - Circular vignette
 *
 * @param channel HDMA channel (6 or lower; 7 belongs to the NMI OAM DMA)
 * @param layers Layer bitmask to apply window masking (TM_BG1, TM_BG2, etc.)
 * @param centerX Horizontal center of circle (0-255)
 * @param centerY Vertical center of circle (0-223)
 * @param radius Circle radius in pixels (0-128)
 *
 * @note Call again with a different radius to animate the wipe.
 *
 * @code
 * // Iris wipe on BG1, centered on screen
 * hdmaIrisWipe(HDMA_CHANNEL_6, TM_BG1, 128, 112, 80);
 *
 * // Animate iris opening
 * for (r = 0; r < 128; r += 2) {
 *     hdmaIrisWipe(HDMA_CHANNEL_6, TM_BG1, 128, 112, r);
 *     WaitForVBlank();
 * }
 * @endcode
 *
 * Called again while the effect runs, it only moves the channel's table
 * pointer, which HDMA reads at the next frame: the change lands whole.
 * (Until 2026-10-03 the channel was set up again, which restarted the table
 * at the next HBlank — one frame with the top of the table on the bottom of
 * the screen.)
 *
 * While it runs the effect owns W12SEL, W34SEL, WOBJSEL and TMW and writes
 * them directly: the window module's own shadows do not see those writes,
 * so do not call windowEnable() / windowSetMainMask() for the same layers
 * until hdmaIrisWipeStop().
 *
 * @note Not cheap: the table is rebuilt from the circle on every call —
 *       about 2.15 million master clocks for a radius of 80 (measured on
 *       luna, library audit 2026-10-03), roughly six frames of CPU — and
 *       the FIRST call also waits one VBlank before enabling the window
 *       masking. Animate it every few frames, not every frame, and do not
 *       call it from an NMI callback.
 */
void hdmaIrisWipe(u8 channel, u8 layers, u8 centerX, u8 centerY, u8 radius);

/**
 * @brief Stop iris wipe effect and restore window registers
 *
 * Disables the HDMA channel and clears all window masking registers
 * (W12SEL, W34SEL, WOBJSEL, TMW) to restore normal display.
 *
 * @param channel The channel used for the iris wipe
 */
void hdmaIrisWipeStop(u8 channel);

/*============================================================================
 * HDMA Water Ripple
 *============================================================================*/

/**
 * @brief Create a water ripple distortion effect
 *
 * Similar to hdmaWaveH but with amplitude that increases from top to bottom,
 * simulating underwater refraction or heat haze. Uses the wave system's
 * double-buffered tables internally.
 *
 * @param channel HDMA channel (6 or lower; 7 belongs to the NMI OAM DMA)
 * @param bg Background layer (0=BG1, 1=BG2, 2=BG3)
 * @param amplitude Maximum ripple amplitude at bottom of screen (1-60 pixels, clamped)
 * @param speed Animation speed (1=slow, 4=fast)
 *
 * @note Amplitude is clamped to 60 to prevent s16 overflow in sine computation.
 * @note Cannot be used simultaneously with hdmaWaveH (shared buffers).
 * @note Call hdmaWaveUpdate() each frame to animate.
 * @note Call hdmaWaveStop() to stop the effect.
 *
 * @code
 * hdmaWaterRipple(HDMA_CHANNEL_6, 0, 8, 2);
 *
 * while (1) {
 *     WaitForVBlank();
 *     hdmaWaveUpdate();  // Animate ripple
 * }
 * @endcode
 */
void hdmaWaterRipple(u8 channel, u8 bg, u8 amplitude, u8 speed);

#endif /* OPENSNES_HDMA_H */
