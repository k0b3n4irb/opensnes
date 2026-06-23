/**
 * @file types.h
 * @brief OpenSNES Standard Types
 *
 * Provides fixed-width integer types and common macros for SNES development.
 *
 * ## Attribution
 *
 * Originally from: PVSnesLib (https://github.com/alekmaul/pvsneslib)
 * Author: Alekmaul
 * License: zlib (compatible with MIT)
 * Modifications:
 *   - Renamed file from snestypes.h
 *   - Added comprehensive documentation
 *   - Reorganized sections
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_TYPES_H
#define OPENSNES_TYPES_H

/*============================================================================
 * Fixed-Width Integer Types
 *============================================================================*/

/**
 * @defgroup types Fixed-Width Types
 * @brief Integer types with guaranteed sizes
 *
 * On the 65816 CPU (cproc compiler), since chantier A1 (2026-05-08):
 * - char  = 8 bits
 * - short = 16 bits
 * - int   = 16 bits (2 bytes)
 * - long  = 32 bits (4 bytes)
 * (see compiler/cproc/type.c — `typeint` size 2, `typelong` size 4.)
 *
 * Always use these typedefs instead of C primitive types.
 * @{
 */

/** @brief 8-bit signed integer (-128 to 127) */
typedef signed char s8;

/** @brief 8-bit unsigned integer (0 to 255) */
typedef unsigned char u8;

/** @brief 16-bit signed integer (-32768 to 32767) */
typedef signed short s16;

/** @brief 16-bit unsigned integer (0 to 65535) */
typedef unsigned short u16;

/**
 * @brief 32-bit signed integer (-2147483648 to 2147483647)
 *
 * Conditionally typedef'd to maintain `sizeof(s32) == 4` on the two
 * compilers that touch this header:
 * - **cc65816** (cproc + QBE w65816 — the actual SDK compiler): `long`
 *   is 4 bytes since chantier A1 (2026-05-08). Pre-A1 used `int` for
 *   the 32-bit type because cproc treated `int` as 32-bit; A1 made
 *   `int` 2 bytes, so the canonical 32-bit type moves to `long`. The
 *   `bin/cc65816` wrapper passes `-D__OPENSNES__=1` to its preprocessor
 *   so this branch fires.
 * - **Host clang** (the `-fsyntax-only` parallel lint pass in
 *   `make/common.mk`): on x86_64 / arm64, `int` is 4 bytes and
 *   `long` is 8. To keep `sizeof(s32) == 4` on the lint side, we
 *   fall back to `int` here (the `#else` branch). The lint does
 *   NOT receive `-D__OPENSNES__`.
 *
 * The underlying typedef differs per compiler, but `sizeof(s32) == 4`
 * holds in both — which is the property the SDK's static asserts and
 * data layout rely on.
 */
#ifdef __OPENSNES__
typedef signed long s32;
#else
typedef signed int s32;
#endif

/**
 * @brief 32-bit unsigned integer (0 to 4294967295)
 *
 * See @ref s32 for the cc65816-vs-host-lint conditional rationale.
 */
#ifdef __OPENSNES__
typedef unsigned long u32;
#else
typedef unsigned int u32;
#endif

/** @} */ /* end of types group */

/*============================================================================
 * Volatile Types
 *============================================================================*/

/**
 * @defgroup volatile_types Volatile Types
 * @brief Volatile types for hardware register access
 *
 * Use these when accessing memory-mapped I/O registers to prevent
 * the compiler from optimizing away reads/writes.
 * @{
 */

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;

/** @} */

/*============================================================================
 * Boolean Type
 *============================================================================*/

/**
 * @defgroup bool_type Boolean
 * @brief Boolean type and constants
 *
 * @warning TRUE is 0xFF, not 1! This matches SNES convention where
 *          bit patterns are often used. Always compare with != FALSE
 *          rather than == TRUE.
 *
 * @code
 * if (flag) { }         // Good
 * if (flag != FALSE) {} // Good
 * if (flag == TRUE) {}  // Bad! Will fail for non-0xFF truthy values
 * @endcode
 * @{
 */

/*
 * Bool type compatibility:
 * - C11 compilers (cproc) have built-in bool/_Bool
 * - Legacy compilers (816-tcc) need typedef
 * We check for __STDC_VERSION__ >= C99 as indicator of modern compiler
 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
typedef unsigned char bool;
#define false 0
#define true  1
#endif

/* SNES-specific TRUE/FALSE (0xFF for TRUE is common in SNES code) */
#define FALSE 0
#define TRUE  0xFF

/** @} */

/*============================================================================
 * Common Macros
 *============================================================================*/

/**
 * @defgroup macros Common Macros
 * @brief Utility macros
 * @{
 */

/** @brief Null pointer constant */
#ifndef NULL
#define NULL ((void*)0)
#endif

/**
 * @brief Create a bit mask
 *
 * @param n Bit number (0-15)
 * @return Bitmask with bit n set
 *
 * @code
 * u16 flags = BIT(0) | BIT(3);  // 0x0009
 * @endcode
 *
 * Uses `1u` (unsigned) so that `BIT(15) == 0x8000` holds under
 * cc65816's 16-bit `int` (since chantier A1, 2026-05-08). With
 * signed `1`, `1 << 15` overflows to -32768 on a 16-bit int and
 * compares unequal to the (long-promoted) `0x8000` literal.
 */
#define BIT(n) (1u << (n))

/**
 * @brief Get low byte of 16-bit value
 * @param x 16-bit value
 * @return Low 8 bits
 */
#define LO_BYTE(x) ((u8)((x) & 0xFF))

/**
 * @brief Get high byte of 16-bit value
 * @param x 16-bit value
 * @return High 8 bits
 */
#define HI_BYTE(x) ((u8)(((x) >> 8) & 0xFF))

/**
 * @brief Combine two bytes into 16-bit value
 * @param lo Low byte
 * @param hi High byte
 * @return 16-bit value
 */
#define MAKE_WORD(lo, hi) ((u16)(((u8)(hi) << 8) | (u8)(lo)))

/**
 * @brief Get minimum of two values
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @brief Get maximum of two values
 */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * @brief Clamp value to range
 * @param x Value to clamp
 * @param lo Minimum value
 * @param hi Maximum value
 */
#define CLAMP(x, lo, hi) (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))

/**
 * @brief Get number of elements in array
 * @param arr Array variable
 * @return Number of elements
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/** @} */

/*============================================================================
 * Function Pointer Types
 *============================================================================*/

/**
 * @defgroup func_ptr Function Pointers
 * @brief Common function pointer typedefs
 * @{
 */

/** @brief Void function taking no arguments */
typedef void (*VoidFn)(void);

/** @} */

#endif /* OPENSNES_TYPES_H */
