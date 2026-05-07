/**
 * @file sprite_dynamic_internal.h
 * @brief Internal helpers shared by sprite_dynamic_dispatch.c and
 *        sprite_dynamic_meta.c.
 *
 * NOT a public header — do not install, do not include from
 * lib/include/snes/. This file lives next to the .c files that
 * implement the dynamic-sprite dispatcher and metasprite walker so
 * the helpers they share have one declaration site.
 */

#ifndef SNES_SPRITE_DYNAMIC_INTERNAL_H
#define SNES_SPRITE_DYNAMIC_INTERNAL_H

#include <snes/types.h>

/**
 * @brief Pixel size of the "large" half of an OBJ_SIZE_* size pair.
 *        Implemented in sprite_dynamic_helpers.c.
 */
u8 mode_large_size(u8 mode);

/**
 * @brief Pixel size of the "small" half of an OBJ_SIZE_* size pair.
 *        Implemented in sprite_dynamic_helpers.c.
 */
u8 mode_small_size(u8 mode);

#endif /* SNES_SPRITE_DYNAMIC_INTERNAL_H */
