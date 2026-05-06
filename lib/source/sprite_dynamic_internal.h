/**
 * @file sprite_dynamic_internal.h
 * @brief Internal helpers shared by sprite_dynamic_dispatch.c and
 *        sprite_dynamic_meta.c.
 *
 * NOT a public header — do not install, do not include from
 * lib/include/snes/. This file lives next to the .c files that
 * implement the dynamic-sprite dispatcher and metasprite walker so
 * the helpers they share stay in one place.
 */

#ifndef SNES_SPRITE_DYNAMIC_INTERNAL_H
#define SNES_SPRITE_DYNAMIC_INTERNAL_H

#include <snes/types.h>

/**
 * @brief Pixel size of the "large" half of an OBJ_SIZE_* size pair.
 *
 * Implemented as a macro (not a function) to dodge a cc65816 codegen
 * issue where a JSL inside a conditional fallback path followed by a
 * `cmp` dispatch produced a stale A-register read on the comparison,
 * silently skipping the dispatch. Inlining keeps everything in
 * registers / locals. See `memory/cc65816_conditional_jsl_codegen_bug.md`
 * for the full write-up of the underlying bug.
 */
#define MODE_LARGE_SIZE(mode) \
    ((u8)((mode) == 0 ? 16 : \
          (mode) == 1 ? 32 : \
          (mode) == 2 ? 64 : \
          (mode) == 3 ? 32 : \
          (mode) == 4 ? 64 : 64))

/**
 * @brief Pixel size of the "small" half of an OBJ_SIZE_* size pair.
 *
 * Same macro-not-function rationale as `MODE_LARGE_SIZE`.
 */
#define MODE_SMALL_SIZE(mode) \
    ((u8)((mode) <= 2 ? 8 : \
          (mode) <= 4 ? 16 : 32))

#endif /* SNES_SPRITE_DYNAMIC_INTERNAL_H */
