/**
 * @file snes.h
 * @brief OpenSNES common-case master header
 *
 * Pulls every module a typical project needs (console, video, sprite,
 * background, input, dma, text, interrupt, system, mode7, hdma, window,
 * colormath, mosaic, map, debug). **Specialty modules are opt-in** —
 * include them separately when your project actually uses them, to keep
 * compile times down and the dependency graph honest:
 *
 * - `<snes/audio.h>` — SPC700 audio driver
 * - `<snes/math.h>` — fixed-point math, LUTs
 * - `<snes/sram.h>` — battery-backed save RAM
 * - `<snes/collision.h>` — bounding-box collision
 * - `<snes/lzss.h>` — LZSS decompression to VRAM
 * - `<snes/gameloop.h>` — gameloop framework opt-in
 * - `<snes/asset.h>` — typed background / tileset bundles
 * - `<snes/scene.h>` — push/pop scene stack
 *
 * The Doxygen list at the bottom of this header repeats the same split
 * for IDE / cross-reference tooling.
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

/* The values below are kept in sync with `CHANGELOG.md` head and verified by
 * `devtools/check_doc_drift.py` at lint time (CI gate `lint.yml::doc-drift`).
 * When bumping the CHANGELOG for a release, bump these three macros in the
 * same commit — `.claude/rules/release.md` documents the step, and `make
 * lint-docs` fails fast on mismatch. */

/** @brief OpenSNES major version */
#define OPENSNES_VERSION_MAJOR 0

/** @brief OpenSNES minor version */
#define OPENSNES_VERSION_MINOR 18

/** @brief OpenSNES patch version */
#define OPENSNES_VERSION_PATCH 0

/** @brief OpenSNES version string */
#define OPENSNES_VERSION_STRING "0.18.0"

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

/* System variables (crt0.asm exports) */
#include <snes/system.h>

/* Mode 7 */
#include <snes/mode7.h>

/* HDMA */
#include <snes/hdma.h>

/* Window/Masking */
#include <snes/window.h>

/* Color Math */
#include <snes/colormath.h>

/* Mosaic Effects */
#include <snes/mosaic.h>

/* Map Engine */
#include <snes/map.h>

/* Debug Utilities */
#include <snes/debug.h>

/*============================================================================
 * Specialty modules — include separately, opt-in only
 *============================================================================
 *
 * These modules carry real linker / dependency cost when included, so the
 * master header keeps them out by default. Include the ones your project
 * actually uses, and add the matching name to LIB_MODULES in your example
 * Makefile (the transitive resolver in make/common.mk pulls deps for you).
 *
 *   #include <snes/audio.h>     // SPC700 audio driver
 *   #include <snes/math.h>      // fixed-point math, LUTs, hardware multiplier
 *   #include <snes/sram.h>      // battery-backed save RAM
 *   #include <snes/collision.h> // bounding-box collision
 *   #include <snes/lzss.h>      // LZSS decompression to VRAM
 *   #include <snes/gameloop.h>  // gameloop framework opt-in
 *   #include <snes/asset.h>     // typed BgAsset / GfxAsset bundles
 *   #include <snes/scene.h>     // push/pop scene stack
 */

#endif /* OPENSNES_H */
