/**
 * @file main.c
 * @brief shmup_1942 — S2: vertical auto-scroll
 * @ingroup examples
 *
 * The scene from S1 (procedurally-generated grass with water channels,
 * sand beaches and organic dirt patches with tents) is now extended to
 * 256×512 pixels — two screens tall — and scrolls vertically at 1 px
 * per frame. The 32×64 tilemap is loaded once into VRAM and the SNES
 * BG-scroll register cycles through it indefinitely, wrapping at the
 * map height.
 *
 * Vertical scroll matches the 1942 / Aero Fighters camera convention:
 * the terrain flows downward across the screen while the player ship
 * (added in S3) appears to fly upward over it.
 *
 * @par SNES Concepts
 * - SC_32x64 / `BG_MAP_32x64`: 32×64 tilemap (two screens tall, 2 KB
 *   of VRAM)
 * - `bgSetScroll()` writes BG scroll registers via the dirty-flag
 *   mechanism; the actual hardware register update happens in the NMI
 *   handler, so scroll changes apply cleanly at the next VBlank
 * - Constant-velocity scroll: increment Y each frame, wrap at the map
 *   height in pixels (512)
 *
 * @par What to Observe
 * - The terrain flows downward across the screen continuously
 * - Water channels, dirt patches and tents come into view and pass
 *   below the visible window
 * - After ~8.5 s the scroll wraps back to the top of the map
 *
 * @par Modules Used
 * console, dma, background, asset
 *
 * @see background.h, asset.h
 */

#include <snes.h>
#include <snes/asset.h>

/**
 * @brief Composed scene bundle (4bpp, 16 colours, 32×64 tilemap).
 */
DECLARE_BG_ASSET(scene, BG_16COLORS, SC_32x64);

/** Map height in pixels — full tilemap, used for scroll wraparound. */
#define MAP_HEIGHT_PX 512

/**
 * @brief Entry point — load the scene and auto-scroll vertically.
 *
 * @return Never returns.
 */
int main(void) {
    setScreenOff();

    /* BG1: tilemap at VRAM $0000, tiles at $4000, palette slot 0.
     * The SC_32x64 size in the asset bundle wires the tilemap layout
     * automatically — bgLoad() writes the full 2 KB tilemap. */
    bgLoad(0, &scene, 0, 0x4000, 0x0000);

    setMode(BG_MODE1, 0);
    setMainScreen(TM_BG1);
    bgSetScroll(0, 0, 0);

    setScreenOn();

    u16 scroll_y = 0;
    while (1) {
        WaitForVBlank();

        /* Advance the scroll by 1 px per frame. The modulo wraps the
         * value at the map height — SNES hardware also wraps the scroll
         * register at the map boundary, but doing it in C keeps the
         * counter bounded and the wraparound point predictable. */
        scroll_y = (u16)((scroll_y + 1) % MAP_HEIGHT_PX);
        bgSetScroll(0, 0, scroll_y);
    }

    return 0;
}
