/**
 * @file superfx.c
 * @brief SuperFX (GSU) library — C initialization
 */

#include <snes/superfx.h>

u8 gsuInit(void) {
    /* Set safe defaults */
    gsu_cfgr = 0x80;        /* IRQ mask, no fast multiply */
    gsu_scmr = 0x19;        /* 4bpp + RAN + RON (most common for PLOT) */
    gsu_scbr = 0x00;        /* Buffer A */
    gsu_dma_src_hi = 0x00;  /* DMA from buffer A */
    return superfx_status != 0;
}
