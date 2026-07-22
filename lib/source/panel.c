/**
 * @file panel.c
 * @brief 9-slice panel stamping — see snes/panel.h for the contract.
 */
#include <snes/panel.h>
#include <snes/dma.h>
#include <snes/console.h>

#define PANEL_MAP_W  32
#define PANEL_MAP_H  32

/** @brief Tilemap attribute bits shared by every entry of a layer. */
static u16 panel_attr(const Panel *p) {
    u16 a = (u16)((p->palette & 0x07) << 10);
    if (p->priority) {
        a |= 0x2000;
    }
    return a;
}

void panelInit(const Panel *p) {
    u16 i;

    for (i = 0; i < PANEL_MAP_W * PANEL_MAP_H; i++) {
        p->map[i] = 0;
    }
}

void panelDraw(const Panel *p, u8 x, u8 y, u8 w, u8 h) {
    u16 attr = panel_attr(p);
    u16 row, col, t;

    /* A 9-slice needs two corners in each direction to mean anything. */
    if (w < 2 || h < 2 || x >= PANEL_MAP_W || y >= PANEL_MAP_H) {
        return;
    }

    for (row = 0; row < h; row++) {
        if (y + row >= PANEL_MAP_H) {
            break;
        }
        for (col = 0; col < w; col++) {
            u16 sheet_row, sheet_col;

            if (x + col >= PANEL_MAP_W) {
                break;
            }
            /* which of the nine cells this tile is: 0 = leading edge,
             * 1 = middle, 2 = trailing edge, per axis */
            sheet_row = (row == 0) ? 0 : (row == h - 1) ? 2 : 1;
            sheet_col = (col == 0) ? 0 : (col == w - 1) ? 2 : 1;

            t = (u16)(p->base_tile + sheet_row * p->stride + sheet_col);
            p->map[(y + row) * PANEL_MAP_W + (x + col)] = (u16)(t | attr);
        }
    }
}

void panelClear(const Panel *p, u8 x, u8 y, u8 w, u8 h) {
    u16 row, col;

    for (row = 0; row < h; row++) {
        if (y + row >= PANEL_MAP_H) {
            break;
        }
        for (col = 0; col < w; col++) {
            if (x + col >= PANEL_MAP_W) {
                break;
            }
            p->map[(y + row) * PANEL_MAP_W + (x + col)] = 0;
        }
    }
}

void panelPut(const Panel *p, u8 x, u8 y, u16 tile) {
    if (x >= PANEL_MAP_W || y >= PANEL_MAP_H) {
        return;
    }
    p->map[y * PANEL_MAP_W + x] = (u16)(tile | panel_attr(p));
}

void panelFlush(const Panel *p) {
    /* FORCED BLANK, not brightness 0: only INIDISP bit 7 opens the VRAM
     * write window outside VBlank, and 2 KB does not fit VBlank next to
     * a game's own transfers. setBrightness(0) blacks the screen while
     * the PPU keeps fetching, and the tail of the copy is dropped. */
    setScreenOff();
    dmaCopyVram((const u8 *)p->map, p->vram_addr,
                PANEL_MAP_W * PANEL_MAP_H * 2);
    setScreenOn();
}
