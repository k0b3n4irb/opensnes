/**
 * @file string.h
 * @brief Memory and string functions: memcpy, memmove, memset, strlen,
 *        strcmp, strcpy, strncpy
 *
 * The seven functions of the C library a game uses most, written in
 * assembly. Add `string` to `LIB_MODULES`; a ROM links only the functions
 * it calls.
 *
 * ## Why not a loop in C
 *
 * A C loop over a plain pointer reads and writes bank $00 (see
 * KNOWN_LIMITATIONS.md): it cannot copy out of a `const` table in ROM into
 * a `FAR` buffer. These functions follow the full 24-bit pointer on both
 * sides, so every combination works — ROM, plain RAM, `FAR` RAM:
 *
 * @code
 * #include <snes/string.h>
 *
 * static const u8 level1[64] = { ... };   // in ROM, any bank
 * FAR u8 level[64];                       // in $7E:2000-$FFFF
 *
 * memcpy(level, level1, sizeof(level));
 * memset(level, 0, sizeof(level));
 * @endcode
 *
 * ## Speed
 *
 * From 12 bytes up, memcpy copies with the CPU's block move (`mvn`): 7 CPU
 * cycles a byte, about 54 master cycles a byte between two RAM buffers on
 * luna. memmove does the same in either direction (`mvp` backward, from 14
 * bytes). memset stores a word at a time through the pointer, 8 CPU cycles
 * a byte. Shorter copies go a word at a time through the pointers (about
 * 12 CPU cycles a byte), which is faster than setting up the block move. A
 * call costs about 1 200 master cycles before its first byte, the C call
 * itself included.
 *
 * All three may be called from an NMI callback, also while the main thread
 * is inside one: the block move's instruction is written on the stack for
 * each call (11 bytes of stack at most beyond the call's own).
 *
 * ## Pointer types
 *
 * A destination is a `FAR` pointer and a source a `const` one: the two
 * types every pointer converts to without a cast (`<snes/types.h>`), so a
 * plain buffer, a `FAR` buffer and a `const` table are all accepted as they
 * are. The functions that return their destination return it as a `FAR`
 * pointer for the same reason: it may be one. Storing that result in a
 * plain pointer is a compile error, as for any `FAR` pointer.
 *
 * ## Limits
 *
 * - The prototypes are the standard ones, with this target's widths: a
 *   size is an `unsigned int`, 16 bits, so 65535 bytes at most; `memset`
 *   takes its byte as an `int` and uses the low 8 bits.
 * - An object must not straddle two banks (none that the linker places
 *   does).
 * - Not for VRAM, CGRAM or OAM: those are behind PPU ports, use the DMA
 *   functions of `<snes/dma.h>`.
 * - `strcmp` compares bytes as unsigned and returns their difference:
 *   only its sign and its being zero are meaningful.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_STRING_H
#define OPENSNES_STRING_H

#include <snes/types.h>

/**
 * @brief Copy n bytes from src to dst
 * @param dst Destination (RAM, plain or FAR)
 * @param src Source (ROM or RAM)
 * @param n Number of bytes
 * @return dst
 *
 * The copy runs forward. If dst lies inside src..src+n the bytes not yet
 * read are overwritten: use memmove() for overlapping areas.
 */
void FAR *memcpy(void FAR *dst, const void *src, unsigned int n);

/**
 * @brief Copy n bytes from src to dst, the areas may overlap
 * @param dst Destination
 * @param src Source
 * @param n Number of bytes
 * @return dst
 *
 * Copies backward when dst lies inside src..src+n, forward otherwise.
 */
void FAR *memmove(void FAR *dst, const void *src, unsigned int n);

/**
 * @brief Fill n bytes with a value
 * @param dst Destination
 * @param c Byte value (the low 8 bits are used)
 * @param n Number of bytes
 * @return dst
 */
void FAR *memset(void FAR *dst, int c, unsigned int n);

/**
 * @brief Length of a string
 * @param s Zero-terminated string
 * @return Number of bytes before the terminator
 */
unsigned int strlen(const char *s);

/**
 * @brief Compare two strings
 * @param a First string
 * @param b Second string
 * @return 0 if equal; negative if a sorts before b, positive if after
 *         (bytes compared as unsigned)
 */
int strcmp(const char *a, const char *b);

/**
 * @brief Copy a string, terminator included
 * @param dst Destination, large enough for src and its terminator
 * @param src Zero-terminated string
 * @return dst
 */
char FAR *strcpy(char FAR *dst, const char *src);

/**
 * @brief Copy at most n bytes of a string
 * @param dst Destination, at least n bytes
 * @param src Zero-terminated string
 * @param n Number of bytes written to dst
 * @return dst
 *
 * If src is shorter than n, the rest of the n bytes is filled with zeros.
 * @warning As in standard C, dst is NOT terminated when src has n bytes or
 *          more before its terminator.
 */
char FAR *strncpy(char FAR *dst, const char *src, unsigned int n);

#endif /* OPENSNES_STRING_H */
