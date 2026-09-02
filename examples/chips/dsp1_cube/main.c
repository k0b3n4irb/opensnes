/**
 * @file main.c
 * @brief DSP-1 pseudo-3D: a cube tumbling in 3D, rotated by the coprocessor.
 * @ingroup examples
 *
 * Each frame the DSP-1 builds a rotation matrix (dsp1Attitude), transforms
 * the cube's 8 corners through it (dsp1Objective), and projects each corner
 * onto the screen with true hardware perspective (dsp1Project, set up once
 * by dsp1Parameter). The corners are drawn as 8 sprites — all the 3D math,
 * including the perspective divide, runs on the NEC uPD77C25, not the 65816.
 *
 * @par SNES Concepts
 * - DSP-1 coprocessor command interface (matrix + vector transform)
 * - Fixed-point 3D rotation offloaded from the CPU
 * - Hardware perspective projection (Parameter -> Project pipeline)
 * - dsp1Present() known-answer probe before relying on the chip
 *
 * @par What to Observe
 * - A cube of 8 dots rotating smoothly about two axes
 * - Perspective: corners swinging toward the camera drift outward (closer
 *   = bigger separation), the far face contracts — no longer a flat tumble
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

/** @brief 1 when the DSP-1 answered the known-answer probe — parked in WRAM
 *  so the luna test manifest can assert it (`coproc_dsp1.toml`). */
volatile u16 dsp1_ok = 0;

/** The cube's 8 corners in model space (DSP-1 integer coordinates). */
static const s16 cube[8][3] = {
    {-80,-80,-80}, { 80,-80,-80}, { 80, 80,-80}, {-80, 80,-80},
    {-80,-80, 80}, { 80,-80, 80}, { 80, 80, 80}, {-80, 80, 80},
};

/** @brief Distance from the camera to the cube's centre, along the DSP-1's
 *  depth axis. Empirically characterised on luna (DSP-1B): with
 *  azs=0x4000 the projection looks along +Y — X is across, Z is up.
 *  Rotated corners span ±~139 (80·√3), so 400 keeps every corner in front
 *  of the eye. */
#define CUBE_DIST 400

/** @brief dsp1Parameter FOV knobs, tuned empirically on luna. The effective
 *  focal length is close to VIEW_LFE + VIEW_LES (projected offset ≈
 *  (lfe+les)·x/y): 96+256 spreads the corners ±~92 px at CUBE_DIST 400 —
 *  filling the 224-line screen without clipping at the nearest swing. */
#define VIEW_LFE 96
#define VIEW_LES 256
/** @brief Screen-plane zenith angle: 0x4000 (90°) = horizontal view, +Y is
 *  the depth axis. The all-zero setup is degenerate (every point projects
 *  to the origin) — this angle is what makes the projection live. */
#define VIEW_AZS 0x4000

int main(void) {
    u16 az = 0, ay = 0;
    u8 i;
    s16 wx, wy, wz;

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

    dsp1_ok = dsp1Present();
    if (dsp1_ok) {
        /* Camera at the origin looking along +Y (VIEW_AZS). Cx/Cy raster
         * coefficients land in dsp1_o0/o1; we recentre manually below so
         * they are not needed here. */
        dsp1Parameter(0, 0, 0, VIEW_LFE, VIEW_LES, 0, VIEW_AZS);
    }

    while (1) {
        az += 0x0140;               /* tumble about two axes */
        ay += 0x00A0;
        dsp1Attitude(0x7FFF, az, ay, 0);

        for (i = 0; i < 8; i++) {
            dsp1Objective(cube[i][0], cube[i][1], cube[i][2]);
            wx = dsp1_o0;           /* copy: dsp1Project reuses the o-slots */
            wy = dsp1_o1 + CUBE_DIST;  /* +Y is depth: push it before the eye */
            wz = dsp1_o2;
            dsp1Project(wx, wy, wz);
            /* Screen mapping (measured): H is mirrored (negative for +X) and
             * V is up-positive, while SNES Y grows downward — so subtract
             * both from the screen centre. M (dsp1_o2) is the depth scale,
             * unused here, but it is how you would size sprites with
             * distance. */
            oamSet(i, (u16)(124 - dsp1_o0), (u16)(108 - dsp1_o1), 0, 0, 3, 0);
        }

        WaitForVBlank();
    }
    return 0;
}
