/**
 * @file main.c
 * @brief Mode 7 racing — an F-Zero-style mini game (#4, part 1)
 * @ingroup examples
 *
 * A drivable race circuit on the Mode 7 plane. The car stays fixed
 * near the bottom of the screen and the WORLD rotates and scrolls
 * under it — the classic SNES racing camera:
 * per frame, mode7SetAngle(heading) + mode7SetScroll/SetCenter follow
 * the car's fixed-point position. This is also the per-frame
 * dogfooding of the C-migrated mode7 module (C1 audit).
 *
 * - D-pad left/right: steer — A: accelerate — B: brake
 * - Grass slows the car down; the red wall ring stops it.
 *
 * The track is generated at build time (gen_track.py -> gfx4snes
 * -M 7); collision reads a 128x128 class map (road/grass/wall, one
 * byte per tile) through a far pointer — 16 KB of data deliberately
 * outside bank $00 (see the bank $00 budget rules).
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - The F-Zero camera: SetCenter(car) makes the car the rotation
 *   pivot, SetScroll places it low on screen, SetAngle spins the
 *   world opposite to the car's heading
 * - Fixed-point driving physics from the math module: velocity =
 *   fixMul(speed, fixSin/fixCos(heading)) — same 0-255 angle unit as
 *   mode7SetAngle, so heading feeds both directly
 * - Mode 7 VRAM interleave loaded with dmaCopyVramMode7(); OBJ tiles
 *   coexist at name base 3 (word $6000)
 * - A procedural 16x16 car sprite (4bpp planes built in C)
 *
 * @par What to Observe
 * Boot on the start line. Hold A: the checker line recedes and the
 * track streams under the car; steer through the chicane. Cutting the
 * grass visibly slows you; the border wall is solid.
 *
 * @par Modules Used
 * console, dma, background, sprite, math, input
 *
 * @see gen_track.py — the reproducible track generator
 * @see lib/source/mode7.c — the C Mode 7 module this game exercises
 */

#include <snes.h>
#include <snes/mode7.h>
#include <snes/math.h>
#include <snes/input.h>

extern u8 track_til[], track_til_end[];
extern u8 track_map[], track_map_end[];
extern u8 track_pal[], track_pal_end[];
/** @brief Banked-data accessor (data.asm): C pointer derefs are
 * bank-$00-hardcoded (B2), so the 16 KB class map in bank $01+ is
 * read through one `lda.l track_class,x` in ASM. */
extern u8 track_class_at(u16 idx);

/** @brief Screen position of the car sprite (rotation pivot) */
#define CAR_SCREEN_X 120
#define CAR_SCREEN_Y 168

/** @brief Physics tuning (positions in 12.4 fixed, speeds in 8.8) */
#define ACCEL        6
#define BRAKE        12
#define DRAG         2
#define MAX_SPEED    0x0300     /* 3.0 px/frame */
#define MAX_GRASS    0x00C0     /* grass cap: 0.75 px/frame */
#define STEER_STEP   2

/** @brief Probe oracles / game state (12.4 fixed-point pixels) */
u16 car_x;
u16 car_y;
u16 car_speed;                  /* 8.8 px/frame */
u8 car_heading;                 /* 0-255, the shared angle unit */
u8 car_surface;                 /* current class under the car */


/** @brief 4 8x8 4bpp tiles = one 16x16 car (built at init) */
static u8 car_tiles[128];

/**
 * @brief Draw the car arrow into a 16x16 pixel grid, encode as 4bpp
 *
 * Palette indices: 0 transparent, 1 body (red), 2 cockpit (white).
 * The arrow points UP — the world rotates, the car never does.
 */
static void build_car_sprite(void) {
    static u8 pix[16][16];
    u8 x, y, t, row, plane;

    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            u8 hw = (u8)(1 + (y >> 1));          /* half-width grows down */
            u8 dx = (u8)(x >= 8 ? x - 8 : 7 - x); /* distance from center */
            pix[y][x] = (u8)((y > 1 && dx < hw && y < 15) ? 1 : 0);
        }
    }
    for (y = 5; y < 9; y++) {                    /* cockpit block */
        for (x = 6; x < 10; x++) {
            pix[y][x] = 2;
        }
    }

    /* encode: OBJ 16x16 = tiles (0,0),(8,0),(0,8),(8,8) at n, n+1,
     * n+16, n+17 — but dmaCopyVram of 128 linear bytes lands them as
     * n..n+3, so we place quadrants in OAM tile order n, n+1 and use
     * two rows via pitch: simplest is encoding quadrant (qy,qx) into
     * tiles 0,1,16,17 positions manually — here we upload 0,1 then
     * 16,17 with two DMAs from this buffer (see init). Buffer layout:
     * [tile00][tile01][tile10][tile11], 32 bytes each. */
    {
        u8 q;
        for (q = 0; q < 4; q++) {
            u8 qx = (u8)((q & 1) * 8);
            u8 qy = (u8)((q >> 1) * 8);
            for (row = 0; row < 8; row++) {
                for (plane = 0; plane < 4; plane += 2) {
                    u8 b0 = 0, b1 = 0;
                    for (t = 0; t < 8; t++) {
                        u8 p = pix[qy + row][qx + t];
                        b0 = (u8)((b0 << 1) | ((p >> plane) & 1));
                        b1 = (u8)((b1 << 1) | ((p >> (plane + 1)) & 1));
                    }
                    /* planes 0-1 rows interleave in the first 16
                     * bytes, planes 2-3 in the second 16 */
                    car_tiles[(u16)(q * 32) + (plane ? 16 : 0) + row * 2] = b0;
                    car_tiles[(u16)(q * 32) + (plane ? 16 : 0) + row * 2 + 1] = b1;
                }
            }
        }
    }
}

int main(void) {
    u16 keys;
    u16 palcar;

    consoleInit();

    /* Mode 7 plane: interleaved tiles+map upload */
    dmaCopyVramMode7(track_map, (u16)(track_map_end - track_map),
                     track_til, (u16)(track_til_end - track_til));
    dmaCopyCGram(track_pal, 0, (u16)(track_pal_end - track_pal));

    /* Car sprite: procedural tiles at OBJ name base 3 (word $6000),
     * quadrants at tile 0,1 (row 0) and 16,17 (row 1) */
    build_car_sprite();
    dmaCopyVram(car_tiles, 0x6000, 64);          /* tiles 0,1  */
    dmaCopyVram(car_tiles + 64, 0x6100, 64);     /* tiles 16,17 */

    /* Sprite palette 0 (CGRAM 128): 0=transparent, 1=red, 2=white */
    palcar = RGB(28, 4, 4);
    dmaCopyCGram((u8 *)&palcar, 129, 2);
    palcar = RGB(31, 31, 31);
    dmaCopyCGram((u8 *)&palcar, 130, 2);

    setMode(BG_MODE7, 0);
    mode7Init();
    oamInit(OBJ_SIZE8_L16, 3);
    oamSet(0, CAR_SCREEN_X, CAR_SCREEN_Y, 0, 0, 3, 0);
    oamSetSize(0, OBJ_LARGE);

    /* Start on the line (world 312, 272), heading +x (east) */
    car_x = (u16)(312 << 4);
    car_y = (u16)(272 << 4);
    car_heading = 64;
    car_speed = 0;


    setMainScreen(TM_BG1 | LAYER_OBJ);
    setScreenOn();

    while (1) {
        s16 vx, vy;
        u16 px, py;
        u16 nx, ny;
        u16 cap;

        WaitForVBlank();
        keys = padHeld(0);

        if (keys & KEY_LEFT) {
            car_heading = (u8)(car_heading - STEER_STEP);
        }
        if (keys & KEY_RIGHT) {
            car_heading = (u8)(car_heading + STEER_STEP);
        }
        if (keys & KEY_A) {
            if (car_speed < MAX_SPEED) {
                car_speed += ACCEL;
            }
        } else if (car_speed >= DRAG) {
            car_speed -= DRAG;
        } else {
            car_speed = 0;
        }
        if ((keys & KEY_B) && car_speed >= BRAKE) {
            car_speed -= BRAKE;
        }

        /* surface under the car caps the speed */
        cap = (car_surface == 1) ? MAX_GRASS : MAX_SPEED;
        if (car_speed > cap) {
            car_speed = (u16)(car_speed - (DRAG * 4));
        }

        /* velocity: forward = (sin(h), -cos(h)), 8.8 * 8.8 -> 8.8;
         * positions advance in 12.4, so >> 4 the 8.8 delta */
        vx = fixMul((s16)car_speed, fixSin(car_heading));
        vy = (s16)-fixMul((s16)car_speed, fixCos(car_heading));
        nx = (u16)(car_x + (vx >> 4));
        ny = (u16)(car_y + (vy >> 4));

        /* class of the DESTINATION tile: wall blocks, else move */
        px = (u16)(nx >> 7);                     /* 12.4 -> tile (px>>3) */
        py = (u16)(ny >> 7);
        car_surface = track_class_at((u16)((py << 7) + px));
        if (car_surface == 2) {
            car_speed = 0;                       /* wall: full stop */
        } else {
            car_x = nx;
            car_y = ny;
        }

        /* the F-Zero camera: pivot on the car, car low on screen */
        mode7SetAngle(car_heading);
        mode7SetCenter((s16)(car_x >> 4), (s16)(car_y >> 4));
        mode7SetScroll((s16)((car_x >> 4) - CAR_SCREEN_X),
                       (s16)((car_y >> 4) - CAR_SCREEN_Y));
    }

    return 0;
}
