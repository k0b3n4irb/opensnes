/* libFuzzer target: aseprite2snes's JSON parser (json.c). The tool reads the
 * Aseprite --data export as a NUL-terminated string and calls json_parse. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char *text = malloc(size + 1);
    if (!text) return 0;
    memcpy(text, data, size);
    text[size] = '\0';
    jerror err;
    jnode *root = json_parse(text, &err);
    if (root) json_free(root);
    free(text);
    return 0;
}
