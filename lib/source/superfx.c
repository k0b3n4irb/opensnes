/**
 * @file superfx.c
 * @brief SuperFX (GSU) coprocessor library functions
 */

#include <snes/superfx.h>

u8 gsuInit(void) {
    return superfx_status != 0;
}
