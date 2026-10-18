/* fuzz_rearrange — gfx4snes's palette rearrangement
 * (tools/lib/gfx4snes/src/palettes.c, palette_rearrange_snes), which
 * opensnes-tileset --rearrange runs: it regroups the colours of every 8x8
 * tile into the banks of the palette and renumbers the pixels.
 *
 * Input: one byte of tile count (1-16), one byte choosing 2 or 4 bpp, a
 * palette of 256 BGR555 words, then the pixels (one palette index a byte).
 * A refusal (too many colours in a tile, not enough banks) is the tool's
 * `fatal`, caught here. On success two invariants are checked, and abort
 * on a breach:
 *   - every pixel keeps its colour (new palette at the new index == old
 *     palette at the old index);
 *   - index 0 is transparent by its place: a pixel of index 0, and only
 *     one, lands on the first slot of its bank (2026-10-15: an opaque
 *     index whose colour equalled index 0's was folded onto it).
 * Seeds: none needed, the format is all bytes. */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "palettes.h"

static jmp_buf refused;

void info(const char *f, ...) { (void)f; }
void warning(const char *f, ...) { (void)f; }
void note(const char *f, ...) { (void)f; }
void errorcontinue(const char *f, ...) { (void)f; }
void fatal(const char *f, ...) { (void)f; longjmp(refused, 1); }

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static int pal[256], old_pal[256];
    static unsigned char img[16 * 64], old_img[16 * 64];
    int nbtiles, nbcolors, inc, i;

    if (size < 2 + 512 + 64)
        return 0;
    nbtiles = 1 + data[0] % 16;
    nbcolors = (data[1] & 1) ? 16 : 4;
    inc = nbcolors == 4 ? 4 : 16;
    for (i = 0; i < 256; i++)
        pal[i] = (data[2 + 2 * i] | (data[3 + 2 * i] << 8)) & 0x7FFF;
    data += 514;
    size -= 514;
    if (size < (size_t)nbtiles * 64)
        nbtiles = (int)(size / 64);
    memcpy(img, data, (size_t)nbtiles * 64);
    memcpy(old_pal, pal, sizeof pal);
    memcpy(old_img, img, (size_t)nbtiles * 64);
    if (setjmp(refused))
        return 0;                       /* refused: a leak at most, no check */
    palette_rearrange_snes(img, pal, nbtiles, nbcolors, true);
    for (i = 0; i < nbtiles * 64; i++) {
        if (pal[img[i]] != old_pal[old_img[i]])
            abort();                    /* a pixel changed colour */
        if ((old_img[i] == 0) != (img[i] % inc == 0))
            abort();                    /* transparency gained or lost */
    }
    return 0;
}
