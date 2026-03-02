/*
 * DynamicMap - Screen refresh utilities
 *
 * Uses smapDma() to transfer spritemap from bank $7E to VRAM.
 */

#include <snes.h>
#include "maputil.h"

/* Assembly helper: DMA from spritemap buffer (bank $7E) to VRAM */
extern void smapDma(u16 byte_offset, u16 vram_addr, u16 byte_count);

void screenRefreshTile(u8 tile, u16 address) {
    u16 offset = (u16)tile * 2048;
    smapDma(offset, address + (u16)tile * 1024, 2048);
}

void screenRefresh(u16 address) {
    WaitForVBlank();
    screenRefreshTile(0, address);
    screenRefreshTile(1, address);
    WaitForVBlank();
    screenRefreshTile(2, address);
    screenRefreshTile(3, address);
}
