/**
 * @file superfx.h
 * @brief SuperFX (GSU) coprocessor register definitions and API
 *
 * The SuperFX/GSU is a 16-bit RISC coprocessor running at 10.74 or 21.47 MHz.
 * It has its own instruction set (not 65816) and communicates with the main
 * CPU through shared cartridge SRAM and GSU registers at $3000-$303F.
 *
 * CRITICAL: When the GSU is running, it owns the ROM and RAM buses.
 * The SNES CPU cannot read ROM or SRAM until the GSU stops (STOP instruction).
 *
 * @see docs/tutorials/superfx.md
 */

#ifndef SNES_SUPERFX_H
#define SNES_SUPERFX_H

#include <snes/types.h>

/*============================================================================
 * GSU General Purpose Registers ($3000-$301F)
 * 16-bit, readable/writable by SNES CPU when GSU is stopped.
 * Writing R15 high byte ($301F) starts GSU execution.
 *============================================================================*/

#define REG_GSU_R0     (*(volatile u16*)0x3000)
#define REG_GSU_R1     (*(volatile u16*)0x3002)
#define REG_GSU_R2     (*(volatile u16*)0x3004)
#define REG_GSU_R3     (*(volatile u16*)0x3006)
#define REG_GSU_R4     (*(volatile u16*)0x3008)
#define REG_GSU_R5     (*(volatile u16*)0x300A)
#define REG_GSU_R6     (*(volatile u16*)0x300C)
#define REG_GSU_R7     (*(volatile u16*)0x300E)
#define REG_GSU_R8     (*(volatile u16*)0x3010)
#define REG_GSU_R9     (*(volatile u16*)0x3012)
#define REG_GSU_R10    (*(volatile u16*)0x3014)
#define REG_GSU_R11    (*(volatile u16*)0x3016)  /**< LINK return address */
#define REG_GSU_R12    (*(volatile u16*)0x3018)  /**< LOOP counter */
#define REG_GSU_R13    (*(volatile u16*)0x301A)  /**< LOOP target address */
#define REG_GSU_R14    (*(volatile u16*)0x301C)  /**< ROM data pointer */
#define REG_GSU_R15    (*(volatile u16*)0x301E)  /**< Program Counter (write high byte = GO!) */

/*============================================================================
 * GSU Control Registers ($3030-$303F)
 *============================================================================*/

/** Status/Flag Register (16-bit, read/write) */
#define REG_SFR        (*(volatile u16*)0x3030)
#define REG_SFR_L      (*(volatile u8*)0x3030)
#define REG_SFR_H      (*(volatile u8*)0x3031)
#define SFR_GO         0x20    /**< Bit 5: GSU is running (1=running, 0=stopped) */
#define SFR_IRQ        0x80    /**< Bit 7 of high byte: IRQ flag (set on STOP) */

/** Backup RAM Register (1-bit, write) */
#define REG_BRAMR      (*(volatile u8*)0x3033)

/** Program Bank Register (8-bit, read/write) */
#define REG_PBR        (*(volatile u8*)0x3034)

/** ROM Bank Register (8-bit, read-only from SNES CPU) */
#define REG_ROMBR      (*(volatile u8*)0x3036)

/** Screen Base Register (8-bit, write) — PLOT pixel buffer base in SRAM */
#define REG_SCBR       (*(volatile u8*)0x3038)

/** Clock Select Register (8-bit, write) */
#define REG_CLSR       (*(volatile u8*)0x3039)
#define CLSR_10MHZ     0x00    /**< 10.74 MHz (default) */
#define CLSR_21MHZ     0x01    /**< 21.47 MHz (GSU-1/GSU-2 only, not Mario Chip 1) */

/** Screen Mode Register (8-bit, write) — bus arbitration + PLOT config */
#define REG_SCMR       (*(volatile u8*)0x303A)
#define SCMR_2BPP      0x00    /**< 2 bits per pixel */
#define SCMR_4BPP      0x01    /**< 4 bits per pixel */
#define SCMR_8BPP      0x03    /**< 8 bits per pixel */
#define SCMR_RAN       0x08    /**< Bit 3: GSU owns RAM (1=GSU, 0=SNES CPU) */
#define SCMR_RON       0x10    /**< Bit 4: GSU owns ROM (1=GSU, 0=SNES CPU) */
#define SCMR_HT0       0x04    /**< Height bit 0 */
#define SCMR_HT1       0x20    /**< Height bit 1 */
#define SCMR_H128      0x00    /**< 128 pixels high */
#define SCMR_H160      0x04    /**< 160 pixels high */
#define SCMR_H192      0x20    /**< 192 pixels high */
#define SCMR_OBJ       0x24    /**< OBJ mode */

/** Version Code Register (8-bit, read-only) */
#define REG_VCR        (*(volatile u8*)0x303B)

/** RAM Bank Register (8-bit, read-only from SNES CPU) */
#define REG_RAMBR      (*(volatile u8*)0x303C)

/** Cache Base Register (16-bit, read-only) */
#define REG_CBR        (*(volatile u16*)0x303E)

/*============================================================================
 * SRAM addresses (SNES CPU side)
 *============================================================================*/

#define GSU_SRAM_BASE  0x700000  /**< Cartridge SRAM, bank $70 */

/*============================================================================
 * API Functions
 *============================================================================*/

/**
 * @brief Detect SuperFX hardware
 * @return Non-zero if GSU detected (VCR chip version), 0 if not present
 *
 * Reads VCR register ($303B). Returns the chip version code.
 * On non-SuperFX cartridges, reads open bus (typically $00 or $FF).
 */
extern u8 superfx_status;

/**
 * @brief Initialize SuperFX — checks if GSU hardware is present
 * @return 1 if GSU detected, 0 if not
 *
 * The crt0.asm startup code reads VCR and stores the result in
 * superfx_status. This function returns whether the GSU was detected.
 */
u8 gsuInit(void);

#endif /* SNES_SUPERFX_H */
