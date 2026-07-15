/**
 * @file main.c
 * @brief Rotating Mode 7 perspective — full matrix driven by 4 HDMA channels
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s Mode7 Perspective demo
 * (PeterLemon/SNES, PPU/Mode7/Perspective). Four HDMA channels each feed
 * one Mode 7 matrix register per scanline (M7A/M7B/M7C/M7D from cos, sin,
 * -sin, cos tables), so the ground plane is BOTH perspective-projected
 * (each line scaled by 20480/y) AND rotated (48 angle steps) — the full
 * affine matrix per line, where graphics/backgrounds/mode7_perspective
 * drives only the two diagonal terms (no rotation).
 *
 * Rotating means the whole matrix changes per angle: the demo precomputes
 * 48 tables per trig function (krom's exact tables, extracted verbatim;
 * entry(a,y) = trig(2*PI*a/48) * 20480 / y in 8.8 fixed point) and
 * repoints all four HDMA channels each frame.
 *
 * @par SNES Concepts
 * - Per-scanline Mode 7 matrix writes (perspective + rotation combined)
 * - 4-channel HDMA coordination on one register family
 * - HDMA table repointing as a zero-copy animation primitive
 * - Mode 7 interleaved VRAM (dmaCopyVramMode7)
 *
 * @par What to Observe
 * A kart-style track receding to the horizon. D-pad moves over the plane,
 * L/R rotate the world in 7.5-degree steps, Y/A nudge the pivot on X,
 * X/B nudge it on Y (krom's control map).
 *
 * @par Modules Used
 * console, dma, background, hdma, input, mode7
 *
 * @see https://github.com/PeterLemon/SNES — original demo & tables
 * @see devtools/m7ptables.py — table extraction + math verification
 */

#include <snes.h>

/** @brief Mode 7 ground: 9-tile prefab track (original art) */
extern u8 ground_pc7[], ground_pc7_end[];
extern u8 ground_mp7[], ground_mp7_end[];
extern u8 ground_pal[], ground_pal_end[];

/** @brief krom's tables, verbatim: 48 x [1][val16]x224 + terminator */
extern u8 m7cos[], m7sin[], m7nsin[];

/** @brief One angle table: 224 x 3-byte HDMA entries + terminator */
#define TABLE_STRIDE 673u
/** @brief 48 rotation steps of 7.5 degrees */
#define ANGLES 48u

/** @brief Rotation step index, 0..47 */
static u16 angle;
/** @brief BG1 scroll shadows (krom init: 384, 768) */
static u16 scr_x, scr_y;
/** @brief Mode 7 pivot shadows (krom init: 512, 1152) */
static u16 pos_x, pos_y;

/**
 * @brief Point all four matrix channels at the current angle's tables.
 *
 * cos feeds both diagonal terms (M7A, M7D), sin/-sin the off-diagonal
 * pair (M7B, M7C) — a standard rotation matrix, perspective-scaled per
 * line inside the tables themselves. hdmaSetup() re-arms the channel and
 * reads the bank from the far pointer, so repointing is just four calls.
 */
static void repointTables(void) {
    u16 off = angle * TABLE_STRIDE;
    hdmaSetup(HDMA_CHANNEL_0, HDMA_MODE_1REG_2X, HDMA_DEST_M7A, m7cos + off);
    hdmaSetup(HDMA_CHANNEL_1, HDMA_MODE_1REG_2X, HDMA_DEST_M7B, m7sin + off);
    hdmaSetup(HDMA_CHANNEL_2, HDMA_MODE_1REG_2X, HDMA_DEST_M7C, m7nsin + off);
    hdmaSetup(HDMA_CHANNEL_3, HDMA_MODE_1REG_2X, HDMA_DEST_M7D, m7cos + off);
}


int main(void) {
    u16 pad;

    consoleInit();
    setScreenOff();

    dmaCopyCGram(ground_pal, 0, (u16)(ground_pal_end - ground_pal));
    /* Out-of-map area renders the backdrop (CGRAM color 0) under
     * M7SEL=$80 — force it black like krom's (gfx4snes put a tile
     * color there) */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x00;
    dmaCopyVramMode7(ground_mp7, (u16)(ground_mp7_end - ground_mp7),
                     ground_pc7, (u16)(ground_pc7_end - ground_pc7));

    setMode(BG_MODE7, 0);
    mode7Init();
    /* krom: M7SEL = $80 — screen-over area shows tile 0 (the wrap bit
     * family; see mode7.h for the field breakdown) */
    mode7SetSettings(0x80);

    /* krom's start pose: camera over the track, horizon up top */
    scr_x = 384; scr_y = 768;
    pos_x = 512; pos_y = 1152;
    angle = 0;

    setMainScreen(TM_BG1);
    repointTables();
    /* hdmaSetup configures but does NOT enable — arm all four channels
     * (krom: HDMAEN = %00001111) */
    hdmaEnable(0x0F);
    setScreenOn();

    while (1) {
        WaitForVBlank();

        /* Right after the NMI returns we're still ~35 lines inside
         * VBlank — krom's own InputLoop writes here too. Re-arming an
         * HDMA channel mid-frame would restart its table walk. */
        bgSetScroll(0, scr_x, scr_y);
        mode7SetCenter((s16)pos_x, (s16)pos_y);
        repointTables();

        pad = padHeld(0);

        if (pad & KEY_L) { angle++; if (angle >= ANGLES) angle = 0; }
        if (pad & KEY_R) { if (angle == 0) angle = ANGLES; angle--; }

        if (pad & KEY_UP)    { scr_y--; pos_y--; }
        if (pad & KEY_DOWN)  { scr_y++; pos_y++; }
        if (pad & KEY_LEFT)  { scr_x--; pos_x--; }
        if (pad & KEY_RIGHT) { scr_x++; pos_x++; }

        /* krom: Y/A tweak the pivot X, X/B the pivot Y */
        if (pad & KEY_Y) pos_x--;
        if (pad & KEY_A) pos_x++;
        if (pad & KEY_X) pos_y--;
        if (pad & KEY_B) pos_y++;
    }

    return 0;
}
