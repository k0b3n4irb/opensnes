/* libFuzzer harness for lodepng's decoder (gaps review H5, 2026-09-14).
 *
 * gfx4snes and img2snes both decode user PNGs with the vendored lodepng;
 * a malformed file must yield an error code, never a memory fault. The
 * harness feeds every input to lodepng_decode32() (RGBA8, the path both
 * tools take) and frees whatever comes back. Built with
 * -fsanitize=fuzzer,address,undefined by tools/fuzz/Makefile. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "lodepng.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    unsigned char *out = NULL;
    unsigned w = 0, h = 0;
    unsigned err = lodepng_decode32(&out, &w, &h, data, size);
    (void)err;
    free(out);
    return 0;
}
