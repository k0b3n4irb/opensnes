/**
 * @file snes.h
 * @brief OpenSNES Master Header
 *
 * Include this file to access all OpenSNES functionality.
 *
 * @code
 * #include <snes.h>
 *
 * int main(void) {
 *     consoleInit();
 *     // Your game code here
 *     while (1) {
 *         WaitForVBlank();
 *     }
 * }
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_H
#define OPENSNES_H

/*============================================================================
 * Version Information
 *============================================================================*/

/** @brief OpenSNES major version */
#define OPENSNES_VERSION_MAJOR 0

/** @brief OpenSNES minor version */
#define OPENSNES_VERSION_MINOR 1

/** @brief OpenSNES patch version */
#define OPENSNES_VERSION_PATCH 0

/** @brief OpenSNES version string */
#define OPENSNES_VERSION_STRING "0.1.0-dev"

/*============================================================================
 * Core Headers
 *============================================================================*/

/* Standard types */
#include <snes/types.h>

/* Hardware registers */
#include <snes/registers.h>

/* Console initialization */
#include <snes/console.h>

/* Video / PPU */
#include <snes/video.h>

/* Sprites */
#include <snes/sprite.h>

/* Backgrounds */
#include <snes/background.h>

/* Input */
#include <snes/input.h>

/* DMA */
#include <snes/dma.h>

/* Text */
#include <snes/text.h>

/* Interrupts */
#include <snes/interrupt.h>

/* Mode 7 */
#include <snes/mode7.h>

/*============================================================================
 * Optional Headers (include separately if needed)
 *============================================================================*/

/* Audio: #include <snes/audio.h> */
/* Math: #include <snes/math.h> */

#endif /* OPENSNES_H */
