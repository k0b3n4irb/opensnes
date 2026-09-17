/* libFuzzer target: font2snes's PNG decoder (stb_image, built with the
 * tool's own configuration: PNG only, no linear/HDR). font2snes calls
 * stbi_load on the file; this is the same decoder fed from memory. */
#include <stddef.h>
#include <stdint.h>
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    int w, h, n;
    if (size > 0x7FFFFFFF) return 0;
    unsigned char *px = stbi_load_from_memory(data, (int)size, &w, &h, &n, 0);
    if (px) stbi_image_free(px);
    return 0;
}
