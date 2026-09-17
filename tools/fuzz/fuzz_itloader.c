/* libFuzzer harness for smconv's Impulse Tracker loader (gaps review H5).
 *
 * itl_module_create() trusts every count and offset in the .it header —
 * it has no error path — so a corrupt module is where a memory fault
 * would live. The loader reads through the io layer from a path, so each
 * input is written to a tmpfs file first (/dev/shm when present). Built
 * with -fsanitize=fuzzer,address,undefined by tools/fuzz/Makefile. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "itloader.h"

static char path[64];

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    const char *dir = access("/dev/shm", W_OK) == 0 ? "/dev/shm" : "/tmp";
    snprintf(path, sizeof path, "%s/fuzz_itloader_%d.it", dir, (int)getpid());
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return 0;
    fwrite(data, 1, size, f);
    fclose(f);
    itl_module_t *m = itl_module_create(path);
    if (m)
        itl_module_destroy(m);
    return 0;
}
