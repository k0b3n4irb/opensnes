/**
 * @file debug.h
 * @brief Debug utilities for SNES development
 *
 * Provides breakpoints, Nocash debug messages, and runtime assertions
 * for use with emulators (Mesen2, no$sns).
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
 *         SNES_BREAK();  // Halts in Mesen2 debugger
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
 * @brief Trigger a Mesen2 debugger breakpoint
 *
 * Emits a WDM $00 instruction which Mesen2 recognizes as a breakpoint.
 * Has no effect on real hardware (WDM is a 2-byte NOP on 65816).
 */
void consoleMesenBreakpoint(void);

/**
 * @brief Send a debug message to the Nocash debug console
 *
 * Writes a null-terminated string byte-by-byte to the debug register
 * at $21FC. Visible in Mesen2's debug console and no$sns.
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
 * @brief Breakpoint — halts execution in Mesen2 debugger
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
