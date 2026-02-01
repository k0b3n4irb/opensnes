/**
 * @file registers.h
 * @brief SNES Hardware Register Definitions
 *
 * Memory-mapped I/O register addresses for PPU, CPU, DMA, and other
 * SNES hardware components.
 *
 * ## Usage
 *
 * @code
 * // Write to PPU register
 * REG_INIDISP = 0x80;  // Force blank
 *
 * // Read from CPU register
 * u8 status = REG_RDNMI;
 * @endcode
 *
 * ## Reference
 *
 * See docs/hardware/MEMORY_MAP.md for complete register documentation.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 *
 * ## Attribution
 *
 * Register definitions based on:
 * - SNESdev Wiki (https://snes.nesdev.org/)
 * - Fullsnes by Nocash
 * - PVSnesLib by Alekmaul
 */

#ifndef OPENSNES_REGISTERS_H
#define OPENSNES_REGISTERS_H

#include <snes/types.h>

/*============================================================================
 * PPU Registers ($2100-$213F)
 *============================================================================*/

/**
 * @defgroup ppu_regs PPU Registers
 * @brief Picture Processing Unit registers
 * @{
 */

/** @brief Display control (W) */
#define REG_INIDISP     (*(vu8*)0x2100)

/** @brief Object (sprite) size and base (W) */
#define REG_OBJSEL      (*(vu8*)0x2101)

/** @brief OAM address low (W) */
#define REG_OAMADDL     (*(vu8*)0x2102)

/** @brief OAM address high (W) */
#define REG_OAMADDH     (*(vu8*)0x2103)

/** @brief OAM data write (W) */
#define REG_OAMDATA     (*(vu8*)0x2104)

/** @brief BG mode and tile size (W) */
#define REG_BGMODE      (*(vu8*)0x2105)

/** @brief Mosaic effect (W) */
#define REG_MOSAIC      (*(vu8*)0x2106)

/** @brief BG1 tilemap address (W) */
#define REG_BG1SC       (*(vu8*)0x2107)

/** @brief BG2 tilemap address (W) */
#define REG_BG2SC       (*(vu8*)0x2108)

/** @brief BG3 tilemap address (W) */
#define REG_BG3SC       (*(vu8*)0x2109)

/** @brief BG4 tilemap address (W) */
#define REG_BG4SC       (*(vu8*)0x210A)

/** @brief BG1/2 tile data address (W) */
#define REG_BG12NBA     (*(vu8*)0x210B)

/** @brief BG3/4 tile data address (W) */
#define REG_BG34NBA     (*(vu8*)0x210C)

/** @brief BG1 horizontal scroll (W, 2x write) */
#define REG_BG1HOFS     (*(vu8*)0x210D)

/** @brief BG1 vertical scroll (W, 2x write) */
#define REG_BG1VOFS     (*(vu8*)0x210E)

/** @brief BG2 horizontal scroll (W, 2x write) */
#define REG_BG2HOFS     (*(vu8*)0x210F)

/** @brief BG2 vertical scroll (W, 2x write) */
#define REG_BG2VOFS     (*(vu8*)0x2110)

/** @brief BG3 horizontal scroll (W, 2x write) */
#define REG_BG3HOFS     (*(vu8*)0x2111)

/** @brief BG3 vertical scroll (W, 2x write) */
#define REG_BG3VOFS     (*(vu8*)0x2112)

/** @brief BG4 horizontal scroll (W, 2x write) */
#define REG_BG4HOFS     (*(vu8*)0x2113)

/** @brief BG4 vertical scroll (W, 2x write) */
#define REG_BG4VOFS     (*(vu8*)0x2114)

/** @brief VRAM address increment mode (W) */
#define REG_VMAIN       (*(vu8*)0x2115)

/** @brief VRAM address low (W) */
#define REG_VMADDL      (*(vu8*)0x2116)

/** @brief VRAM address high (W) */
#define REG_VMADDH      (*(vu8*)0x2117)

/** @brief VRAM data write low (W) */
#define REG_VMDATAL     (*(vu8*)0x2118)

/** @brief VRAM data write high (W) */
#define REG_VMDATAH     (*(vu8*)0x2119)

/** @brief Mode 7 settings (W) */
#define REG_M7SEL       (*(vu8*)0x211A)

/** @brief CGRAM address (W) */
#define REG_CGADD       (*(vu8*)0x2121)

/** @brief CGRAM data write (W) */
#define REG_CGDATA      (*(vu8*)0x2122)

/** @brief BG1/BG2 window mask settings (W) */
#define REG_W12SEL      (*(vu8*)0x2123)

/** @brief BG3/BG4 window mask settings (W) */
#define REG_W34SEL      (*(vu8*)0x2124)

/** @brief OBJ/MATH window mask settings (W) */
#define REG_WOBJSEL     (*(vu8*)0x2125)

/** @brief Window 1 left position (W) */
#define REG_WH0         (*(vu8*)0x2126)

/** @brief Window 1 right position (W) */
#define REG_WH1         (*(vu8*)0x2127)

/** @brief Window 2 left position (W) */
#define REG_WH2         (*(vu8*)0x2128)

/** @brief Window 2 right position (W) */
#define REG_WH3         (*(vu8*)0x2129)

/** @brief BG1-4 window logic (W) */
#define REG_WBGLOG      (*(vu8*)0x212A)

/** @brief OBJ/MATH window logic (W) */
#define REG_WOBJLOG     (*(vu8*)0x212B)

/** @brief Main screen designation (W) */
#define REG_TM          (*(vu8*)0x212C)

/** @brief Sub screen designation (W) */
#define REG_TS          (*(vu8*)0x212D)

/** @brief Main screen window mask (W) */
#define REG_TMW         (*(vu8*)0x212E)

/** @brief Sub screen window mask (W) */
#define REG_TSW         (*(vu8*)0x212F)

/** @brief Color math control A (W) */
#define REG_CGWSEL      (*(vu8*)0x2130)

/** @brief Color math control B (W) */
#define REG_CGADSUB     (*(vu8*)0x2131)

/** @brief Fixed color data (W) */
#define REG_COLDATA     (*(vu8*)0x2132)

/** @brief Screen mode/video select (W) */
#define REG_SETINI      (*(vu8*)0x2133)

/** @brief Multiplication result low (R) */
#define REG_MPYL        (*(vu8*)0x2134)

/** @brief Multiplication result mid (R) */
#define REG_MPYM        (*(vu8*)0x2135)

/** @brief Multiplication result high (R) */
#define REG_MPYH        (*(vu8*)0x2136)

/** @brief Software latch H/V counter (R) */
#define REG_SLHV        (*(vu8*)0x2137)

/** @brief OAM data read (R) */
#define REG_OAMDATAREAD (*(vu8*)0x2138)

/** @brief VRAM data read low (R) */
#define REG_RDVRAML     (*(vu8*)0x2139)

/** @brief VRAM data read high (R) */
#define REG_RDVRAMH     (*(vu8*)0x213A)

/** @brief CGRAM data read (R) */
#define REG_RDCGRAM     (*(vu8*)0x213B)

/** @brief H counter latch (R) */
#define REG_OPHCT       (*(vu8*)0x213C)

/** @brief V counter latch (R) */
#define REG_OPVCT       (*(vu8*)0x213D)

/** @brief PPU status flags (R) */
#define REG_STAT77      (*(vu8*)0x213E)

/** @brief PPU status flags 2 (R) */
#define REG_STAT78      (*(vu8*)0x213F)

/** @} */ /* end of ppu_regs */

/*============================================================================
 * CPU Registers ($4200-$421F)
 *============================================================================*/

/**
 * @defgroup cpu_regs CPU Registers
 * @brief CPU control and status registers
 * @{
 */

/** @brief Interrupt enable (W) */
#define REG_NMITIMEN    (*(vu8*)0x4200)

/** @brief I/O port write (W) */
#define REG_WRIO        (*(vu8*)0x4201)

/** @brief Multiplicand A (W) */
#define REG_WRMPYA      (*(vu8*)0x4202)

/** @brief Multiplicand B (W) */
#define REG_WRMPYB      (*(vu8*)0x4203)

/** @brief Dividend low (W) */
#define REG_WRDIVL      (*(vu8*)0x4204)

/** @brief Dividend high (W) */
#define REG_WRDIVH      (*(vu8*)0x4205)

/** @brief Divisor (W) */
#define REG_WRDIVB      (*(vu8*)0x4206)

/** @brief H-count timer low (W) */
#define REG_HTIMEL      (*(vu8*)0x4207)

/** @brief H-count timer high (W) */
#define REG_HTIMEH      (*(vu8*)0x4208)

/** @brief V-count timer low (W) */
#define REG_VTIMEL      (*(vu8*)0x4209)

/** @brief V-count timer high (W) */
#define REG_VTIMEH      (*(vu8*)0x420A)

/** @brief DMA enable (W) */
#define REG_MDMAEN      (*(vu8*)0x420B)

/** @brief HDMA enable (W) */
#define REG_HDMAEN      (*(vu8*)0x420C)

/** @brief FastROM enable (W) */
#define REG_MEMSEL      (*(vu8*)0x420D)

/** @brief NMI flag and version (R) */
#define REG_RDNMI       (*(vu8*)0x4210)

/** @brief IRQ flag (R) */
#define REG_TIMEUP      (*(vu8*)0x4211)

/** @brief H/V blank and joypad status (R) */
#define REG_HVBJOY      (*(vu8*)0x4212)

/** @brief I/O port read (R) */
#define REG_RDIO        (*(vu8*)0x4213)

/** @brief Division result low (R) */
#define REG_RDDIVL      (*(vu8*)0x4214)

/** @brief Division result high (R) */
#define REG_RDDIVH      (*(vu8*)0x4215)

/** @brief Multiplication result low (R) */
#define REG_RDMPYL      (*(vu8*)0x4216)

/** @brief Multiplication result high (R) */
#define REG_RDMPYH      (*(vu8*)0x4217)

/** @brief Joypad 1 data low (R) */
#define REG_JOY1L       (*(vu8*)0x4218)

/** @brief Joypad 1 data high (R) */
#define REG_JOY1H       (*(vu8*)0x4219)

/** @brief Joypad 2 data low (R) */
#define REG_JOY2L       (*(vu8*)0x421A)

/** @brief Joypad 2 data high (R) */
#define REG_JOY2H       (*(vu8*)0x421B)

/** @brief Joypad 3 data low (R) */
#define REG_JOY3L       (*(vu8*)0x421C)

/** @brief Joypad 3 data high (R) */
#define REG_JOY3H       (*(vu8*)0x421D)

/** @brief Joypad 4 data low (R) */
#define REG_JOY4L       (*(vu8*)0x421E)

/** @brief Joypad 4 data high (R) */
#define REG_JOY4H       (*(vu8*)0x421F)

/** @} */ /* end of cpu_regs */

/*============================================================================
 * DMA Registers ($43x0-$43xF)
 *============================================================================*/

/**
 * @defgroup dma_regs DMA Registers
 * @brief DMA channel registers (x = channel 0-7)
 * @{
 */

/** @brief DMA parameters for channel n */
#define REG_DMAP(n)     (*(vu8*)(0x4300 + ((n) << 4)))

/** @brief DMA B-bus address for channel n */
#define REG_BBAD(n)     (*(vu8*)(0x4301 + ((n) << 4)))

/** @brief DMA A-bus address low for channel n */
#define REG_A1TL(n)     (*(vu8*)(0x4302 + ((n) << 4)))

/** @brief DMA A-bus address high for channel n */
#define REG_A1TH(n)     (*(vu8*)(0x4303 + ((n) << 4)))

/** @brief DMA A-bus bank for channel n */
#define REG_A1B(n)      (*(vu8*)(0x4304 + ((n) << 4)))

/** @brief DMA size low for channel n */
#define REG_DASL(n)     (*(vu8*)(0x4305 + ((n) << 4)))

/** @brief DMA size high for channel n */
#define REG_DASH(n)     (*(vu8*)(0x4306 + ((n) << 4)))

/** @} */ /* end of dma_regs */

/*============================================================================
 * APU Registers ($2140-$2143)
 *============================================================================*/

/**
 * @defgroup apu_regs APU Registers
 * @brief Audio Processing Unit I/O ports
 *
 * These four ports are used for communication between the main CPU
 * and the SPC700 audio processor. Both CPUs can read and write to
 * their respective sides - writes go to output latches visible to
 * the other processor.
 *
 * @{
 */

/** @brief APU I/O port 0 (R/W) */
#define REG_APUIO0      (*(vu8*)0x2140)

/** @brief APU I/O port 1 (R/W) */
#define REG_APUIO1      (*(vu8*)0x2141)

/** @brief APU I/O port 2 (R/W) */
#define REG_APUIO2      (*(vu8*)0x2142)

/** @brief APU I/O port 3 (R/W) */
#define REG_APUIO3      (*(vu8*)0x2143)

/** @} */ /* end of apu_regs */

/*============================================================================
 * Register Value Constants
 *============================================================================*/

/**
 * @defgroup reg_const Register Constants
 * @brief Common values for hardware registers
 * @{
 */

/* INIDISP values */
#define INIDISP_FORCE_BLANK  0x80  /**< Force screen blank */
#define INIDISP_BRIGHTNESS(n) ((n) & 0x0F)  /**< Set brightness (0-15) */

/* BGMODE values */
#define BGMODE_MODE0  0  /**< 4 BG layers, 4 colors each */
#define BGMODE_MODE1  1  /**< 2 BG 16-color, 1 BG 4-color (most common) */
#define BGMODE_MODE2  2  /**< 2 BG 16-color with offset-per-tile */
#define BGMODE_MODE3  3  /**< 1 BG 256-color, 1 BG 16-color */
#define BGMODE_MODE7  7  /**< Mode 7 (rotation/scaling) */

/* NMITIMEN values */
#define NMITIMEN_NMI_ENABLE   0x80  /**< Enable NMI on VBlank */
#define NMITIMEN_JOY_ENABLE   0x01  /**< Enable auto joypad read */

/* TM/TS values (main/sub screen enable) */
#define TM_BG1  BIT(0)  /**< Enable BG1 on main screen */
#define TM_BG2  BIT(1)  /**< Enable BG2 on main screen */
#define TM_BG3  BIT(2)  /**< Enable BG3 on main screen */
#define TM_BG4  BIT(3)  /**< Enable BG4 on main screen */
#define TM_OBJ  BIT(4)  /**< Enable sprites on main screen */

/** @} */ /* end of reg_const */

#endif /* OPENSNES_REGISTERS_H */
