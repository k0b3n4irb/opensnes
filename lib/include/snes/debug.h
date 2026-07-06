/**
 * @file debug.h
 * @brief Debug utilities for SNES development
 *
 * Provides breakpoints, Nocash debug messages, and runtime assertions
 * for use with emulators (luna, Mesen2, no$sns).
 *
 * ## Usage
 *
 * @code
 * #include <snes.h>
 * #include <snes/debug.h>
 *
 * void enemy_update(u16 idx) {
 *     SNES_ASSERT(objWorkspace.type != 0);
 *     SNES_NOCASH("enemy_update called");
 *     // ...
 *     if (something_wrong) {
 *         SNES_BREAK();  // WDM $00 — assert channel in luna, breakpoint in Mesen2
 *     }
 * }
 * @endcode
 *
 * ## Build
 *
 * Add `debug` to LIB_MODULES:
 * @code
 * LIB_MODULES := console sprite dma debug
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_DEBUG_H
#define OPENSNES_DEBUG_H

#include <snes/types.h>

/*============================================================================
 * Functions (implemented in debug.asm)
 *============================================================================*/

/**
 * @brief Trigger an emulator breakpoint (WDM $00)
 *
 * Emits a WDM $00 instruction — luna captures it as an assert/WDM event
 * (fails the test harness run); Mesen2 recognizes it as a breakpoint.
 * (Function name kept for source compatibility.)
 * Has no effect on real hardware (WDM is a 2-byte NOP on 65816).
 */
void consoleMesenBreakpoint(void);

/**
 * @brief Send a debug message to the Nocash debug console
 *
 * Writes a null-terminated string byte-by-byte to the debug register
 * at $21FC. Visible in luna's nocash log, Mesen2's debug console and no$sns.
 *
 * @param msg Null-terminated string to display
 *
 * @note This is a simple const-string version. No format string support.
 */
void consoleNocashMessage(const char *msg);

/*============================================================================
 * Macros
 *============================================================================*/

/**
 * @brief Breakpoint — WDM $00 (luna assert channel / Mesen2 debugger halt)
 */
#define SNES_BREAK() consoleMesenBreakpoint()

/**
 * @brief Send a debug message to emulator console
 * @param msg Const string literal
 */
#define SNES_NOCASH(msg) consoleNocashMessage(msg)

/**
 * @brief Runtime assertion — breaks in emulator if condition is false
 *
 * When NDEBUG is defined, assertions compile to nothing.
 * Otherwise, a failed assertion triggers a Mesen2 breakpoint.
 *
 * @param cond Condition that must be true
 */
#ifdef NDEBUG
  #define SNES_ASSERT(cond) ((void)0)
#else
  #define SNES_ASSERT(cond) do { if (!(cond)) { SNES_BREAK(); } } while(0)
#endif

#endif /* OPENSNES_DEBUG_H */
