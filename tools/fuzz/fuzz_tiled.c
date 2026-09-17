/* libFuzzer target: tmx2snes's Tiled JSON loader (cute_tiled), from memory.
 * The whole map parse — objects, layers, tilesets, properties — on an
 * attacker-shaped .tmj. tmx2snes calls exactly this with the file bytes.
 *
 * Two harness details, both about cute_tiled's error handling:
 *
 *  - The library aborts (CUTE_TILED_CRASH) wherever the author had no error
 *    label to jump to. That is its error path, not a bug, so we longjmp out
 *    of it and let the fuzzer look for the reads and writes the checks do
 *    not cover. tmx2snes overrides the same macro to print a diagnostic.
 *  - Jumping out skips the library's own cleanup, so every malformed input
 *    would leak and libFuzzer would eventually report an out-of-memory that
 *    says nothing about cute_tiled. Every allocation the parser makes goes
 *    through CUTE_TILED_ALLOC (strpool defers to it), so the harness routes
 *    that through a registry and releases whatever is still held after a
 *    jump. The library's normal error path frees on its own; the registry
 *    is then already empty.
 */
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define FUZZ_MAX_ALLOCS 65536
static void *fuzz_allocs[FUZZ_MAX_ALLOCS];
static size_t fuzz_n;

static void *fuzz_alloc(size_t size)
{
    void *p = malloc(size);
    if (p && fuzz_n < FUZZ_MAX_ALLOCS) fuzz_allocs[fuzz_n++] = p;
    return p;
}

static void fuzz_free(void *p)
{
    size_t i;
    if (!p) return;
    for (i = fuzz_n; i-- > 0;) {
        if (fuzz_allocs[i] == p) { fuzz_allocs[i] = fuzz_allocs[--fuzz_n]; break; }
    }
    free(p);
}

static void fuzz_free_all(void)
{
    while (fuzz_n) free(fuzz_allocs[--fuzz_n]);
}

static jmp_buf fuzz_tiled_bail;
#define CUTE_TILED_ALLOC(size, ctx) fuzz_alloc(size)
#define CUTE_TILED_FREE(mem, ctx) fuzz_free(mem)
#define CUTE_TILED_CRASH() longjmp(fuzz_tiled_bail, 1)
#define CUTE_TILED_IMPLEMENTATION
#include "cute_tiled.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    cute_tiled_map_t *map;

    if (size == 0 || size > 0x7FFFFFFF) return 0;
    fuzz_n = 0;
    if (setjmp(fuzz_tiled_bail)) { fuzz_free_all(); return 0; }
    map = cute_tiled_load_map_from_memory(data, (int)size, 0);
    if (map) cute_tiled_free_map(map);
    fuzz_free_all();
    return 0;
}
