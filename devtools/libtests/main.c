/*
 * Library runtime-assertion fixture — exercises lib/source functions with
 * known vectors and stores the results in WRAM globals, which the harness
 * (test_libtest.py, via `luna state --assert`) checks against expected
 * values. Static checks and the visual-regression hash can't prove a
 * return value or a bounds guard; only an execution check can.
 *
 * Current coverage:
 *   - math: div16/mod16 (bounded long division), mul16, sqrt16
 *   - text: cursor_y wrap — printing past row 31 must wrap to row 0
 *     instead of writing past tilemapBuffer[2048] into the RAM sections
 *     that follow it (text_config is the first casualty pre-fix)
 *
 * Globals live in bank $00 WRAM (< $2000), so `--assert 00:<off>=<bytes>`
 * reads them. Values are little-endian.
 */
#include <snes.h>
#include <snes/math.h>
#include <snes/text.h>

/* --- math vectors --- */
u16 r_div_a;    /* div16(100, 7)    -> 14 */
u16 r_mod_a;    /* mod16(100, 7)    -> 2 */
u16 r_div_max;  /* div16(65535, 1)  -> 65535 (worst case of the old O(quotient) loop) */
u16 r_div_zero; /* div16(42, 0)     -> 0 (documented contract) */
u16 r_mod_zero; /* mod16(42, 0)     -> 0 (documented contract) */
u16 r_mul;      /* mul16(123, 45)   -> 5535 */
u16 r_sqrt;     /* sqrt16(144)      -> 12 */

/* --- text overflow sentinels --- */
u8 s_map_width; /* text_config.map_width after 40 printed rows -> 32.
                 * Pre-fix, rows 32+ wrote past tilemapBuffer into whatever
                 * RAM section the linker placed next (layout-dependent). */
u8 s_cursor_y;  /* textGetY() after 40 newlines -> 8 (40 wraps to 40-32).
                 * The deterministic sentinel: pre-fix this read 40. */

u16 r_done;     /* 0xBEEF once every assignment above has executed */

int main(void) {
    u8 i;

    r_div_a    = div16(100, 7);
    r_mod_a    = mod16(100, 7);
    r_div_max  = div16(65535, 1);
    r_div_zero = div16(42, 0);
    r_mod_zero = mod16(42, 0);
    r_mul      = mul16(123, 45);
    r_sqrt     = sqrt16(144);

    textModeInit();

    /* 40 rows of >= 7 glyphs each: rows 32-39 must wrap to rows 0-7.
     * Pre-fix they wrote at buffer offsets 2048+, i.e. over text_config
     * (glyph 6 of row 32 lands exactly on map_width). */
    for (i = 0; i < 40; i++) {
        textPrint("OVERFLOW");
        textPutChar('\n');
    }

    s_map_width = text_config.map_width;
    s_cursor_y  = textGetY();
    r_done      = 0xBEEF;

    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
