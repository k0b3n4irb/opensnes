/**
 * @file mp5.h
 * @brief SNES MultiPlayer5 (Multitap) Support
 *
 * Functions for detecting and using the MultiPlayer5 adapter,
 * which allows up to 5 controllers on a single SNES.
 *
 * When enabled, the NMI handler automatically reads controllers 3-5
 * in addition to the standard controllers 1-2. All five controllers
 * are accessible through the standard padHeld()/padPressed() functions
 * (pad indices 0-4).
 *
 * ## Usage
 *
 * @code
 * #include <snes.h>
 * #include <snes/mp5.h>
 *
 * int main(void) {
 *     consoleInit();
 *
 *     // Detect MultiPlayer5 adapter
 *     if (detectMPlay5()) {
 *         // MultiPlayer5 found! Pads 0-4 now active.
 *     }
 *
 *     while (1) {
 *         WaitForVBlank();
 *
 *         // Read all 5 controllers
 *         u16 p1 = padHeld(0);
 *         u16 p2 = padHeld(1);
 *         u16 p3 = padHeld(2);
 *         u16 p4 = padHeld(3);
 *         u16 p5 = padHeld(4);
 *     }
 * }
 * @endcode
 *
 * @note MultiPlayer5 is mutually exclusive with mouse and Super Scope.
 *       When MP5 is active, mouse and scope reading are disabled.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_MP5_H
#define OPENSNES_MP5_H

#include <snes/types.h>

/**
 * @brief Detect MultiPlayer5 adapter
 *
 * Probes the controller port using the signature detection protocol
 * from the original SNES development manual (chapter 9.6).
 *
 * If detected, enables MultiPlayer5 reading in the NMI handler.
 * Controllers 3-5 (padHeld(2), padHeld(3), padHeld(4)) become active.
 *
 * @return 1 if MultiPlayer5 detected, 0 if not
 */
u8 detectMPlay5(void);

/**
 * @brief Check if MultiPlayer5 is currently active
 *
 * @return 1 if active, 0 if not
 */
u8 mp5IsConnected(void);

/**
 * @brief Manually enable MultiPlayer5 reading
 *
 * Use this if you know a multitap is connected without calling
 * detectMPlay5(). Enables pad 2-4 reading in the NMI handler.
 */
void mp5Enable(void);

/**
 * @brief Disable MultiPlayer5 reading
 *
 * Stops reading pads 2-4 in the NMI handler. Only pads 0-1 are
 * read after calling this function.
 */
void mp5Disable(void);

#endif /* OPENSNES_MP5_H */
