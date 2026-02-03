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
 * Direct mode (bit 7 = 0): Write different data each scanline
 * ```
 * .db 3        ; Apply next value for 3 lines
 * .db $1F, $00 ; Data (2 bytes for mode 1)
 * .db 5        ; Apply next value for 5 lines
 * .db $1F, $08 ; Data
 * .db 0        ; End of table
 * ```
 *
 * Repeat mode (bit 7 = 1): Write same data for N lines, then advance
 * ```
 * .db $82      ; Repeat next value for 2 lines ($80 | 2)
 * .db $1F, $00 ; Data
 * .db $85      ; Repeat next value for 5 lines
 * .db $1F, $08 ; Data
 * .db 0        ; End of table
 * ```
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
 * @note HDMA channels 6-7 are recommended to avoid conflicts with DMA.
 * @note HDMA tables must be in ROM or bank $7E RAM.
 *
 * ## IMPORTANT: Scroll Registers Require Repeat Mode
 *
 * BG scroll registers (BG1HOFS, BG1VOFS, etc.) require REPEAT mode (bit 7 = 1)
 * in the HDMA line count. Direct mode (bit 7 = 0) does NOT work for scroll.
 *
 * ```
 * // WRONG - Direct mode doesn't work for scroll:
 * .db 32, $20, $00    ; Won't scroll!
 *
 * // CORRECT - Repeat mode for scroll registers:
 * .db $A0, $20, $00   ; $A0 = $80 | 32 = repeat for 32 lines
 * ```
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
 * @param bg Background layer (1-4)
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
 * @param bg Background layer to affect (1-3)
 * @param amplitude Wave amplitude in pixels (1-16)
 * @param frequency Wave frequency (1=long waves, 8=short waves)
 *
 * @code
 * hdmaWaveInit();
 * hdmaWaveH(HDMA_CHANNEL_6, 1, 4, 2);  // Gentle water reflection on BG1
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

#endif /* OPENSNES_HDMA_H */
