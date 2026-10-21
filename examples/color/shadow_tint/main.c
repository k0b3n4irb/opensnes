/**
 * @file main.c
 * @brief Colour — shadow & tint: recolour a whole scene at once
 * @ingroup examples
 *
 * Colour math against a *fixed* colour is how a game changes mood without
 * touching a single tile or palette entry. Subtract a grey and the scene
 * darkens — dusk, a dungeon, nightfall. Add a colour and the scene takes
 * that cast — the blue of being underwater, the orange of a sunset or lava
 * glow, the red flash of taking a hit. One register pair does it to every
 * pixel of a layer, every frame, for free.
 *
 * The scene here is a street of the city at night — apartment blocks and a
 * noodle sign (res/street.png) — so you can watch the effect land on a
 * picture. Press A to cycle:
 *
 *   none -> blackout (darken) -> green rain (tint) -> red alert (tint) -> none
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Colour math with the FIXED-colour source (COLDATA $2132): result is
 *   main-screen ± a constant, applied per enabled layer
 * - colorMathShadow() = subtract grey (darken); colorMathTint() = add a
 *   colour (cast) — the two share the fixed-colour register and the
 *   add/subtract bit, so only one is active at a time
 * - The effect is a PPU pass over the final image: no VRAM, CGRAM or OAM
 *   traffic, so it costs nothing per frame and scales to the whole screen
 *
 * @par What to Observe
 * Boot: the plain street. Press A to darken it (a blackout), then wash it
 * green (a toxic rain), then red (the alarm), then back to plain. Every
 * pixel shifts together. `fx_state` is the probe oracle (0..3).
 *
 * @par Modules Used
 * console, dma, background, colormath, input
 *
 * @see lib/include/snes/colormath.h — colorMathShadow(), colorMathTint()
 */

#include <snes.h>
#include <snes/colormath.h>
#include <snes/input.h>

/** @brief Probe oracle: current effect (0 none, 1 blackout, 2 green rain, 3 red alert) */
u8 fx_state;

#include "res/street.inc"   /* generated: street_tiles[], street_map[], street_pal[], each with _end */

/** @brief Apply the effect for the current fx_state (call after VBlank) */
static void apply_effect(void) {
    switch (fx_state) {
        case 1: colorMathShadow(LAYER_BG1, 14);       break; /* blackout   */
        case 2: colorMathTint(LAYER_BG1, 0, 10, 2);   break; /* green rain */
        case 3: colorMathTint(LAYER_BG1, 18, 0, 0);   break; /* red alert  */
        default: colorMathDisable();                      break; /* none     */
    }
}

int main(void) {
    consoleInit();
    colorMathInit();

    /* The street: 444 tiles at VRAM $0000, its map at $3C00, palette 0 */
    dmaCopyVram(street_tiles, 0x0000, street_tiles_end - street_tiles);
    dmaCopyVram(street_map, 0x3C00, street_map_end - street_map);
    dmaCopyCGram(street_pal, 0, street_pal_end - street_pal);

    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3C00, SC_32x32);
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_BG1);

    fx_state = 0;               /* boot: plain scene, no colour math */
    apply_effect();

    setScreenOn();

    while (1) {
        WaitForVBlank();

        if (padPressed(0) & KEY_A) {
            fx_state = (u8)((fx_state + 1) & 3);
            apply_effect();
        }
    }

    return 0;
}
