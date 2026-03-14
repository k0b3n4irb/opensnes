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
 * Repeat mode (bit 7 = 1): Write same data EVERY scanline for N lines
 * ```
 * .db $82      ; Write data every scanline for 2 lines ($80 | 2)
 * .db $1F, $00 ; Data written on each of the 2 scanlines
 * .db $85      ; Write data every scanline for 5 lines
 * .db $1F, $08 ; Data
 * .db 0        ; End of table
 * ```
 * REQUIRED for scroll registers (BG1HOFS, etc.) and other write-twice/
 * latched registers that need re-writing every scanline.
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
 * hdmaEnable(HDMA_CHANNEL_6);
 *
 * // In main loop, HDMA runs automatically each frame
 * @endcode
 *
 * @warning **Do NOT use HDMA_CHANNEL_7** — the NMI handler uses DMA channel 7
 *          for OAM transfers every frame, which destroys any HDMA setup on that
 *          channel. Safe HDMA channels: 1-6 (channel 0 is used by dmaCopyVram).
 * @note HDMA tables must be in ROM or bank $7E RAM.
 *
 * ## Bank Byte Limitation
 *
 * hdmaSetup() hardcodes bank $00 for ROM addresses (>= $8000). If the
 * linker places a SUPERFREE table in bank $01+, HDMA will read wrong data.
 * Use hdmaSetupBank() with an explicit bank byte for ROM tables, or use
 * RAM-based tables (always bank $00) for dynamic effects.
 *
 * ## IMPORTANT: Scroll Registers Require Repeat Mode
 *
 * BG scroll registers (BG1HOFS, BG1VOFS, etc.) are latched registers that
 * require being written EVERY scanline to maintain their value. Use REPEAT
 * mode (bit 7 = 1) in the HDMA line count for these registers.
 *
 * Non-repeat mode (bit 7 = 0) writes data only ONCE per group, so the
 * scroll value is lost on subsequent scanlines — causing visible glitches.
 *
 * ```
 * // WRONG - Non-repeat writes once then skips, scroll value lost:
 * .db 32, $20, $00    ; Writes on line 1 only, lines 2-32 get stale value
 *
 * // CORRECT - Repeat writes every scanline, scroll value maintained:
 * .db $A0, $20, $00   ; $A0 = $80 | 32 = write every scanline for 32 lines
 * ```
 *
 * Summary:
 * - COLDATA ($2132), CGADD/CGDATA: non-repeat OK (registers hold value)
 * - BG scroll, window, Mode 7 matrix: MUST use repeat mode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_HDMA_H
#define OPENSNES_HDMA_H

#include <snes/types.h>

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
/** @brief HDMA channel 6 (recommended for HDMA) */
#define HDMA_CHANNEL_6  6
/** @brief HDMA channel 7 (recommended for HDMA) */
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
 * @brief Indirect HDMA flag (OR with mode)
 *
 * When set, table contains pointers to data instead of data itself.
 * Useful for large tables or dynamic data.
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

/** @brief Destination: Mode 7 matrix A ($211B) */
#define HDMA_DEST_M7A       0x1B

/*============================================================================
 * Core HDMA Functions
 *============================================================================*/

/**
 * @brief Set up an HDMA channel
 *
 * Configures an HDMA channel with the specified parameters. The channel
 * is NOT enabled automatically - call hdmaEnable() to start it.
 *
 * @param channel HDMA channel (0-7, use HDMA_CHANNEL_6 or _7)
 * @param mode Transfer mode (HDMA_MODE_*)
 * @param destReg Destination B-bus register (low byte of $21xx address)
 * @param table Pointer to HDMA table in ROM or RAM
 *
 * @code
 * hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, HDMA_DEST_COLDATA, my_table);
 * hdmaEnable(HDMA_CHANNEL_6);
 * @endcode
 */
void hdmaSetup(u8 channel, u8 mode, u8 destReg, const void *table);

/**
 * @brief Set up an HDMA channel with explicit source bank byte.
 *
 * Same as hdmaSetup() but allows specifying the ROM bank for HDMA tables
 * in banks other than $00. Use this when your HDMA table is in a SUPERFREE
 * section that may be placed in bank $01+ by the linker.
 *
 * @param channel  HDMA channel (0-7, use HDMA_CHANNEL_6 or _7)
 * @param mode     Transfer mode (HDMA_MODE_*)
 * @param destReg  Destination B-bus register (low byte of $21xx address)
 * @param table    Pointer to HDMA table in ROM or RAM
 * @param bank     Source bank byte ($00-$3F for LoROM)
 */
void hdmaSetupBank(u8 channel, u8 mode, u8 destReg, const void *table, u8 bank);

/**
 * @brief Enable HDMA channel(s)
 *
 * Enables the specified HDMA channel(s). HDMA will start on the next frame.
 *
 * @param channelMask Bitmask of channels to enable (1 << channel)
 *
 * @code
 * hdmaEnable(1 << HDMA_CHANNEL_6);              // Enable channel 6
 * hdmaEnable((1 << HDMA_CHANNEL_6) | (1 << HDMA_CHANNEL_7)); // Enable 6 and 7
 * @endcode
 */
void hdmaEnable(u8 channelMask);

/**
 * @brief Disable HDMA channel(s)
 *
 * Disables the specified HDMA channel(s).
 *
 * @param channelMask Bitmask of channels to disable
 */
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
 * Must be called once before using wave effects. Allocates internal
 * buffers and sets up the wave state.
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
 * @param channel HDMA channel to use (6 or 7 recommended)
 * @param bg Background layer to affect (0=BG1, 1=BG2, 2=BG3)
 * @param amplitude Wave amplitude in pixels (1-60, clamped internally)
 * @param frequency Wave frequency (1-16, higher = tighter waves). Period = 256/frequency scanlines.
 *
 * @code
 * hdmaWaveInit();
 * hdmaWaveH(HDMA_CHANNEL_6, 0, 4, 4);  // Gentle water reflection on BG1
 * hdmaEnable(1 << HDMA_CHANNEL_6);
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
 * @param channel HDMA channel (6 or 7 recommended)
 * @param topBrightness Brightness at top of screen (0-15, 15=full)
 * @param bottomBrightness Brightness at bottom of screen (0-15)
 *
 * @code
 * hdmaBrightnessGradient(HDMA_CHANNEL_7, 15, 0);  // Fade to black
 * @endcode
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
 * @param channel HDMA channel (6 or 7 recommended)
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
 * @param channel HDMA channel (6 or 7 recommended)
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
 * @param channel HDMA channel (6 or 7 recommended)
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
