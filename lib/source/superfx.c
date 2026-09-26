/**
 * @file superfx.c
 * @brief SuperFX (GSU) library — C initialization
 */

#include <snes/superfx.h>

/* gsuInit() and gsuIsPresent() are `inline` in superfx.h. Force-emit canonical here. */
u8 (*const __opensnes_force_emit_gsuInit)(void) = gsuInit;
u8 (*const __opensnes_force_emit_gsuIsPresent)(void) = gsuIsPresent;

void gsuSetProgram(const void *program) {
    /* A 4-byte far pointer (chantier A6): bank in bits 16-23. */
    u32 far_address = (u32)program;
    gsu_prog_addr = (u16)far_address;
    gsu_prog_bank = (u8)(far_address >> 16);
}
