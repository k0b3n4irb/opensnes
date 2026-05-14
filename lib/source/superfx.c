/**
 * @file superfx.c
 * @brief SuperFX (GSU) library — C initialization
 */

#include <snes/superfx.h>

/* gsuInit() is `inline` in superfx.h. Force-emit canonical here. */
u8 (*const __opensnes_force_emit_gsuInit)(void) = gsuInit;
