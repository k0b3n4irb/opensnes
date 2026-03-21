/**
 * @file sa1.h
 * @brief SA-1 Enhancement Chip Interface
 *
 * The SA-1 is a 65c816 coprocessor at 10.74 MHz (3× main CPU speed)
 * integrated into SA-1 cartridges. It shares the same ISA as the main
 * CPU, so compiled C code runs on it unmodified.
 *
 * Memory accessible by SA-1:
 *   - ROM ($00-$3F:$8000-$FFFF via Super MMC)
 *   - I-RAM ($3000-$37FF, 2KB, shared with main CPU)
 *   - BW-RAM ($40-$5F, 256KB, battery-backed)
 *
 * Memory NOT accessible by SA-1:
 *   - Main WRAM ($7E-$7F)
 *   - PPU registers ($2100-$213F)
 *   - APU registers ($2140-$2143)
 *   - Standard CPU I/O ($4200-$43FF)
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_SA1_H
#define OPENSNES_SA1_H

#include <snes/types.h>

/*============================================================================
 * SA-1 Register Definitions ($2200-$23FF)
 *============================================================================*/

/* --- SNES CPU → SA-1 control registers (write) --- */

#define REG_SA1_CCNT    (*(volatile u8*)0x2200)  /**< SA-1 CPU control */
#define REG_SA1_SIE     (*(volatile u8*)0x2201)  /**< SNES CPU interrupt enable */
#define REG_SA1_SIC     (*(volatile u8*)0x2202)  /**< SNES CPU interrupt clear */
#define REG_SA1_CRVL    (*(volatile u8*)0x2203)  /**< SA-1 reset vector low */
#define REG_SA1_CRVH    (*(volatile u8*)0x2204)  /**< SA-1 reset vector high */
#define REG_SA1_CNVL    (*(volatile u8*)0x2205)  /**< SA-1 NMI vector low */
#define REG_SA1_CNVH    (*(volatile u8*)0x2206)  /**< SA-1 NMI vector high */
#define REG_SA1_CIVL    (*(volatile u8*)0x2207)  /**< SA-1 IRQ vector low */
#define REG_SA1_CIVH    (*(volatile u8*)0x2208)  /**< SA-1 IRQ vector high */

/* --- SA-1 → SNES CPU control registers (write from SA-1 side) --- */

#define REG_SA1_SCNT    (*(volatile u8*)0x2209)  /**< SNES CPU control */
#define REG_SA1_CIE     (*(volatile u8*)0x220A)  /**< SA-1 CPU interrupt enable */
#define REG_SA1_CIC     (*(volatile u8*)0x220B)  /**< SA-1 CPU interrupt clear */

/* --- ROM banking (Super MMC) --- */

#define REG_SA1_CXB     (*(volatile u8*)0x2220)  /**< Bank C ROM mapping */
#define REG_SA1_DXB     (*(volatile u8*)0x2221)  /**< Bank D ROM mapping */
#define REG_SA1_EXB     (*(volatile u8*)0x2222)  /**< Bank E ROM mapping */
#define REG_SA1_FXB     (*(volatile u8*)0x2223)  /**< Bank F ROM mapping */

/* --- BW-RAM configuration --- */

#define REG_SA1_BMAPS   (*(volatile u8*)0x2224)  /**< SNES BW-RAM mapping */
#define REG_SA1_BMAP    (*(volatile u8*)0x2225)  /**< SA-1 BW-RAM mapping */
#define REG_SA1_SBWE    (*(volatile u8*)0x2226)  /**< SNES BW-RAM write enable */
#define REG_SA1_CBWE    (*(volatile u8*)0x2227)  /**< SA-1 BW-RAM write enable */
#define REG_SA1_BWPA    (*(volatile u8*)0x2228)  /**< BW-RAM write-protected area */

/* --- I-RAM protection --- */

#define REG_SA1_SIWP    (*(volatile u8*)0x2229)  /**< SNES I-RAM write protect */
#define REG_SA1_CIWP    (*(volatile u8*)0x222A)  /**< SA-1 I-RAM write protect */

/* --- DMA --- */

#define REG_SA1_DCNT    (*(volatile u8*)0x2230)  /**< DMA control */
#define REG_SA1_CDMA    (*(volatile u8*)0x2231)  /**< Character conversion DMA */
#define REG_SA1_SDAL    (*(volatile u8*)0x2232)  /**< DMA source address low */
#define REG_SA1_SDAH    (*(volatile u8*)0x2233)  /**< DMA source address high */
#define REG_SA1_SDAB    (*(volatile u8*)0x2234)  /**< DMA source address bank */
#define REG_SA1_DDAL    (*(volatile u8*)0x2235)  /**< DMA dest address low */
#define REG_SA1_DDAH    (*(volatile u8*)0x2236)  /**< DMA dest address high */
#define REG_SA1_DDAB    (*(volatile u8*)0x2237)  /**< DMA dest address bank (unused for I-RAM) */
#define REG_SA1_DTCL    (*(volatile u8*)0x2238)  /**< DMA byte count low */
#define REG_SA1_DTCH    (*(volatile u8*)0x2239)  /**< DMA byte count high */

/* --- Hardware arithmetic --- */

#define REG_SA1_MCNT    (*(volatile u8*)0x2250)  /**< Arithmetic control (0=mul, 1=div) */
#define REG_SA1_MAL     (*(volatile u8*)0x2251)  /**< Multiplicand/dividend low */
#define REG_SA1_MAH     (*(volatile u8*)0x2252)  /**< Multiplicand/dividend high */
#define REG_SA1_MBL     (*(volatile u8*)0x2253)  /**< Multiplier/divisor low */
#define REG_SA1_MBH     (*(volatile u8*)0x2254)  /**< Multiplier/divisor high */

/* --- Status registers (read) --- */

#define REG_SA1_SFR     (*(volatile u8*)0x2300)  /**< SNES CPU status flags */
#define REG_SA1_CFR     (*(volatile u8*)0x2301)  /**< SA-1 CPU status flags */
#define REG_SA1_HCRL    (*(volatile u8*)0x2302)  /**< H-counter low */
#define REG_SA1_HCRH    (*(volatile u8*)0x2303)  /**< H-counter high */
#define REG_SA1_VCRL    (*(volatile u8*)0x2304)  /**< V-counter low */
#define REG_SA1_VCRH    (*(volatile u8*)0x2305)  /**< V-counter high */
#define REG_SA1_MR      (*(volatile u32*)0x2306) /**< Arithmetic result (read as 32-bit) */
#define REG_SA1_OF      (*(volatile u8*)0x230B)  /**< Arithmetic overflow */

/*============================================================================
 * SA-1 Constants
 *============================================================================*/

/** @brief I-RAM base address (2KB shared between SNES CPU and SA-1) */
#define SA1_IRAM_BASE   0x3000

/** @brief I-RAM size in bytes */
#define SA1_IRAM_SIZE   2048

/** @brief I-RAM as a byte pointer */
#define SA1_IRAM        ((volatile u8*)SA1_IRAM_BASE)

/** @brief Magic value written by SA-1 boot stub to confirm it's running */
#define SA1_READY_MAGIC 0xA5

/** @brief I-RAM address for SA-1 ready flag */
#define SA1_READY_ADDR  SA1_IRAM_BASE

/* CCNT ($2200) bit definitions */
#define SA1_CCNT_SA1_IRQ    0x80  /**< Send IRQ to SA-1 */
#define SA1_CCNT_SA1_RDYB   0x60  /**< SA-1 ready bits */
#define SA1_CCNT_SA1_RESB   0x20  /**< SA-1 reset (0=reset, 1=release) */
#define SA1_CCNT_MSG        0x0F  /**< Message to SA-1 (4 bits) */

/*============================================================================
 * SA-1 API
 *============================================================================*/

/**
 * @brief Initialize and start the SA-1 coprocessor
 *
 * Writes the SA-1 reset vector, enables I-RAM/BW-RAM write access,
 * releases the SA-1 from reset, and waits for the ready flag in I-RAM.
 *
 * @return 1 if SA-1 started successfully, 0 if timeout
 */
u8 sa1Init(void);

#endif /* OPENSNES_SA1_H */
