/**
 * @file sprite_dynamic_helpers.c
 * @brief Pixel-size lookups for the dynamic sprite engine.
 *
 * `mode_large_size` and `mode_small_size` resolve the pixel size of
 * each half of an OBJ_SIZE_* size pair. They live in their own
 * translation unit so the call from `oamDynamicDraw` /
 * `oamMetaDrawDyn` stays a real `jsl` rather than being inlined
 * away — this used to matter as a workaround for a cproc/QBE
 * codegen bug (see `memory/cc65816_conditional_jsl_codegen_bug.md`)
 * but the bug has since been fixed; the function form survived as
 * the more readable shape.
 */

#include <snes/types.h>

/**
 * @brief Pixel size of the "large" half of an OBJ_SIZE_* pair.
 *
 * @param mode  OBJ_SIZE_* constant 0..5 (matches the engine's
 *              size-mode enum).
 * @return      Large-side pixel count (16, 32, or 64).
 */
u8 mode_large_size(u8 mode) {
    if (mode == 0) return 16;
    if (mode == 1) return 32;
    if (mode == 2) return 64;
    if (mode == 3) return 32;
    if (mode == 4) return 64;
    return 64;
}

/**
 * @brief Pixel size of the "small" half of an OBJ_SIZE_* pair.
 *
 * @param mode  OBJ_SIZE_* constant 0..5.
 * @return      Small-side pixel count (8, 16, or 32).
 */
u8 mode_small_size(u8 mode) {
    if (mode <= 2) return 8;
    if (mode <= 4) return 16;
    return 32;
}
