/* fuzz_wav — opensnes-sample's WAV parser (tools/opensnes-sample/src/wav.c)
 * on arbitrary bytes: a lying chunk size, a truncated data chunk, an
 * EXTENSIBLE header shorter than its sub-format. Seeds: the golden fixture. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "wav.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    wav_t w;
    char why[160];
    if (wav_parse(data, size, &w, why, sizeof why))
        free(w.pcm);
    return 0;
}
