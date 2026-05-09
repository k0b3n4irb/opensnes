/**
 * @file math_sqrt.c
 * @brief Square-root half of `<snes/math.h>` — extracted from math.c so the
 *        hdma module can pull just sqrt16 without dragging the full
 *        sin/cos LUT (~512 B) and the arithmetic helpers (~500 B) into
 *        every example that links hdma.
 *
 * Module split shipped 2026-05-09 (post-B6 cleanup) after the window
 * example crossed the bank-$00 budget cliff: pre-B6 it had ~1 KB free,
 * post-B6 only 66 B. The hdma → math transitive dep was the cause —
 * hdma.c uses sqrt16 internally for the iris-wipe radius, but the
 * implicit "the hdma module depends on the math module" pulled the
 * whole math.c in. Splitting sqrt out into its own .c keeps hdma
 * users at near-zero cost while preserving `<snes/math.h>` as the
 * single public header.
 *
 * The split is invisible to user code: anyone who includes
 * `<snes/math.h>` and lists `math` in their LIB_MODULES still gets
 * sqrt16 / fixSqrt because `_DEP_math := math_sqrt` makes the
 * transitive resolver pull math_sqrt.o automatically.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/math.h>

u16 sqrt16(u16 n) {
    /* Canonical bit-by-bit integer square root.
     *
     * Invariant: at each iteration, `result` is the partial answer
     * built so far, and `bit` is the next power-of-four bit to test.
     * If (result + bit)^2 ≤ n, that bit goes into the answer; the
     * remaining n shrinks by `result + bit`, then result shifts in
     * the new bit. Otherwise just shift result (the bit was zero).
     *
     * Lifted from the previously-private isqrt() in lib/source/hdma.c
     * (chantier B6 made it public — same algorithm, identical
     * result for every input).
     */
    u16 result = 0;
    u16 bit = 0x4000;
    while (bit > n) bit >>= 2;
    while (bit != 0) {
        if (n >= result + bit) {
            n = (u16)(n - result - bit);
            result = (u16)((result >> 1) + bit);
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

fixed fixSqrt(fixed x) {
    if (x < 0) return 0;
    /* sqrt(x/256) = sqrt(x) / 16, so the 8.8 representation of
     * sqrt(x) is sqrt(raw) * 16 (because 8.8 = real * 256 and
     * sqrt(real) = sqrt(raw) / 16, so sqrt(real) * 256 = sqrt(raw) * 16).
     * The shift-by-4 caps fractional precision at 4 bits — adequate
     * for distance / hypot calculations, will be raised once the
     * QBE 32-bit codegen (catalogue A7) lands and we can do
     * `sqrt16(x << 8) >> 4` without truncation. */
    return (fixed)((s16)(sqrt16((u16)x) << 4));
}
