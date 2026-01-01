/**
 * @file background.h
 * @brief SNES Background Layer Management
 *
 * Functions for configuring and scrolling background layers.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 *
 * @todo Implement background functions
 */

#ifndef OPENSNES_BACKGROUND_H
#define OPENSNES_BACKGROUND_H

#include <snes/types.h>

/*============================================================================
 * Initialization
 *============================================================================*/

/**
 * @brief Initialize background layer
 *
 * @param bg Background number (0-3)
 */
void bgInit(u8 bg);

/*============================================================================
 * Scrolling
 *============================================================================*/

/**
 * @brief Set background scroll position
 *
 * @param bg Background number (0-3)
 * @param x Horizontal scroll (0-1023)
 * @param y Vertical scroll (0-1023)
 */
void bgSetScroll(u8 bg, u16 x, u16 y);

/**
 * @brief Set horizontal scroll
 *
 * @param bg Background number (0-3)
 * @param x Horizontal scroll
 */
void bgSetScrollX(u8 bg, u16 x);

/**
 * @brief Set vertical scroll
 *
 * @param bg Background number (0-3)
 * @param y Vertical scroll
 */
void bgSetScrollY(u8 bg, u16 y);

/* More functions to be implemented */

#endif /* OPENSNES_BACKGROUND_H */
