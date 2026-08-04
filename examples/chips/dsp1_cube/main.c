/**
 * @file main.c
 * @brief DSP-1 pseudo-3D: a cube tumbling in 3D, rotated by the coprocessor.
 * @ingroup examples
 *
 * Each frame the DSP-1 builds a rotation matrix (dsp1Attitude) and transforms
 * the cube's 8 corners through it (dsp1Objective). The corners are drawn as 8
 * sprites at their orthographically-projected positions, so the cube visibly
 * tumbles in 3D — all the 3D math runs on the NEC uPD77C25, not the 65816.
 *
 * @par SNES Concepts
 * - DSP-1 coprocessor command interface (matrix + vector transform)
 * - Fixed-point 3D rotation offloaded from the CPU
 * - Orthographic projection (x',y' straight to screen)
 *
 * @par What to Observe
 * - A cube of 8 dots rotating smoothly about two axes
 *
 * @par Modules Used
 * console, dma, sprite, dsp1
 *
 * @warning Needs the DSP-1 firmware in luna (dsp1b.rom) — see the README.
 * @see snes/dsp1.h, .claude/notes/tech/dsp1_reference.md
 */
#include <snes.h>
#include <snes/dsp1.h>

/** 8x8 4bpp tile, colour index 1 everywhere. In the SNES 4bpp layout planes
 *  0/1 interleave per row, so plane 0 all-set is the ODD bytes 0,2,…,14. */
static const u8 dot_tile[32] = {
    0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
    0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
};
/** Two colours: index 0 transparent, index 1 white ($7FFF). */
static const u8 dot_pal[4] = { 0x00, 0x00, 0xFF, 0x7F };

/** The cube's 8 corners in model space (DSP-1 integer coordinates). */
static const s16 cube[8][3] = {
    {-80,-80,-80}, { 80,-80,-80}, { 80, 80,-80}, {-80, 80,-80},
    {-80,-80, 80}, { 80,-80, 80}, { 80, 80, 80}, {-80, 80, 80},
};

int main(void) {
    u16 az = 0, ay = 0;
    u8 i;

    consoleInit();
    setMode(BG_MODE1, 0);
    WaitForVBlank();
    /* Upload the dot tile + palette and set the sprite tile base correctly
     * (oamInitGfxSet computes OBSEL's base index from the VRAM address). */
    oamInitGfxSet((u8 *)dot_tile, sizeof dot_tile, (u8 *)dot_pal, sizeof dot_pal,
                  0, 0x4000, OBJ_SIZE8_L16);
    setMainScreen(LAYER_OBJ);
    setScreenOn();

    dsp1Init();                 /* resync the DSP-1 before the first command */

    while (1) {
        az += 0x0140;               /* tumble about two axes */
        ay += 0x00A0;
        dsp1Attitude(0x7FFF, az, ay, 0);

        for (i = 0; i < 8; i++) {
            dsp1Objective(cube[i][0], cube[i][1], cube[i][2]);
            /* orthographic: rotated x'/y' -> screen, centred */
            oamSet(i, 124 + dsp1_o0, 108 + dsp1_o1, 0, 0, 3, 0);
        }

        WaitForVBlank();
    }
    return 0;
}
