/**
 * @file main.c
 * @brief shmup_1942 — S1: static scene render
 * @ingroup examples
 *
 * First stage of an iterative shoot 'em up project built on Kenney's CC0
 * *Pixel Shmup* pack. `res/scene.png` is a 256×256 grass-biome screen
 * composed from `ground.png`'s 16×16 source tiles by
 * `res/compose_scene.py` — pure grass fill with scattered trees, two red
 * tent enemy bases and a few bushes/rings. gfx4snes converts it into a
 * tileset + palette + tilemap, and main() hands the bundle to `bgLoad()`
 * for a single static frame on BG1 in Mode 1.
 *
 * No movement, no sprites, no scrolling — the player ship, scrolling and
 * enemies come in S2-S4.
 *
 * @par SNES Concepts
 * - Mode 1: BG1 is 4bpp (up to 16 colours per palette slot)
 * - `DECLARE_BG_ASSET` bundles tiles + palette + tilemap with the colour
 *   mode and map size locked at declaration time
 * - `bgLoad()` collapses three manual VRAM/CGRAM setup calls into one
 * - VRAM layout: tilemap at $0000, tile graphics at $4000 (no overlap)
 *
 * @par What to Observe
 * - A 256×256 grass field fills the whole screen
 * - Two red tents (stacked 3 tiles tall: roof + body + flag) sit in the
 *   middle band
 * - Trees, bushes and wooden rings scatter for visual texture
 * - Everything is static
 *
 * @par Modules Used
 * console, dma, background, asset
 *
 * @see background.h, asset.h
 */

#include <snes.h>
#include <snes/asset.h>

/**
 * @brief Composed scene bundle (4bpp, 16 colours, 32×32 tilemap).
 *
 * `DECLARE_BG_ASSET(scene, ...)` expands to extern declarations for
 * `scene_tiles`, `scene_pal`, `scene_map` (with matching `_end` labels)
 * and a `static const BgAsset scene = {...}`. The data symbols themselves
 * live in `data.asm`.
 */
DECLARE_BG_ASSET(scene, BG_16COLORS, SC_32x32);

/**
 * @brief Entry point — load the scene and hold a static frame.
 *
 * @return Never returns (infinite VBlank wait).
 */
int main(void) {
    setScreenOff();

    /* BG1: tilemap at VRAM $0000, tiles at $4000, palette slot 0 */
    bgLoad(0, &scene, 0, 0x4000, 0x0000);

    setMode(BG_MODE1, 0);
    setMainScreen(TM_BG1);
    bgSetScroll(0, 0, 0);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
