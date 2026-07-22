/**
 * @file main.c
 * @brief Mode 7 flying — a Pilotwings-style demo (#4, part 2)
 * @ingroup examples
 *
 * A plane over a Mode 7 landscape. The core trick: **altitude drives
 * the Mode 7 scale** — climbing zooms the plane texture out (you see
 * more terrain), diving zooms in, and a shadow sprite slides away
 * from the plane as you climb: the two classic SNES depth cues, from
 * two register writes per frame.
 *
 * - D-pad left/right: turn — up: climb — down: dive
 * - A: throttle up — B: throttle down
 * - Objective: land on the three striped pads (slow + low over a
 *   pad). The backdrop flashes green on a good landing; water is a
 *   splash (respawn); fields bounce you back up.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Altitude as Mode 7 scale: mode7SetScale(0x0100 + alt) then
 *   SetAngle (the scale feeds the matrix at SetAngle time)
 * - The shadow depth cue: same sprite shape, dark palette, screen
 *   offset proportional to altitude
 * - Surface classes read through a banked-data ASM accessor (B2 —
 *   same pattern as mode7_racing)
 * - Fixed-point flight model: heading/speed/altitude in 8.8, the
 *   math module's shared 0-255 angle unit
 *
 * @par What to Observe
 * Take off (hold A, then Up): the fields shrink and the shadow drifts
 * down-screen. Line up a pad, throttle down, descend — a green flash
 * counts the landing. Ditch in the river to see the respawn.
 *
 * @par Modules Used
 * console, dma, background, sprite, math, mode7, input
 *
 * @see gen_terrain.py — the reproducible terrain generator
 */

#include <snes.h>
#include <snes/mode7.h>
#include <snes/math.h>
#include <snes/input.h>

extern u8 terrain_til[], terrain_til_end[];
extern u8 terrain_map[], terrain_map_end[];
extern u8 terrain_pal[], terrain_pal_end[];

/** @brief Banked-data accessor (data.asm) for the class map */
extern u8 terrain_class_at(u16 idx);

/** @brief Screen position of the plane (and its shadow's origin) */
#define PLANE_SCREEN_X 120
#define PLANE_SCREEN_Y 140

/** @brief Flight model tuning (8.8 fixed) */
#define THROTTLE_STEP 4
#define MAX_SPEED     0x0280
#define MIN_FLY_SPEED 0x0080    /* below this the plane sinks */
#define CLIMB_STEP    6
#define MAX_ALT       0x0480
#define LAND_SPEED    0x00E0    /* max speed for a good landing */

/** @brief Probe oracles / game state */
u16 plane_x;                    /* 12.4 px */
u16 plane_y;                    /* 12.4 px */
u16 plane_speed;                /* 8.8 px/frame */
u16 plane_alt;                  /* 8.8 "meters" */
u8 plane_heading;               /* 0-255 */
u8 landings;                    /* good landings so far */
u8 surface;                     /* class under the plane */
u8 airborne;                    /* 1 once truly airborne (alt > 0x60) */

/** @brief 4 8x8 4bpp tiles = one 16x16 shape (built at init) */
static u8 shape_tiles[128];

/** @brief Encode a 16x16 palette-index grid into 4bpp OBJ quadrants */
static void encode_16x16(u8 pix[16][16]) {
    u8 row, plane_i, t, q;
    for (q = 0; q < 4; q++) {
        u8 qx = (u8)((q & 1) * 8);
        u8 qy = (u8)((q >> 1) * 8);
        for (row = 0; row < 8; row++) {
            for (plane_i = 0; plane_i < 4; plane_i += 2) {
                u8 b0 = 0, b1 = 0;
                for (t = 0; t < 8; t++) {
                    u8 p = pix[qy + row][qx + t];
                    b0 = (u8)((b0 << 1) | ((p >> plane_i) & 1));
                    b1 = (u8)((b1 << 1) | ((p >> (plane_i + 1)) & 1));
                }
                shape_tiles[(u16)(q * 32) + (plane_i ? 16 : 0) + row * 2] = b0;
                shape_tiles[(u16)(q * 32) + (plane_i ? 16 : 0) + row * 2 + 1] = b1;
            }
        }
    }
}

/** @brief Draw the plane silhouette (index 1 body, 2 canopy) */
static void build_plane(void) {
    static u8 pix[16][16];
    u8 x, y;
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            u8 dx = (u8)(x >= 8 ? x - 8 : 7 - x);
            u8 v = 0;
            if (y >= 2 && y <= 13 && dx <= 1) {
                v = 1;                           /* fuselage */
            }
            if (y >= 6 && y <= 9 && dx <= (u8)(7 - (y - 6) * 2)) {
                v = 1;                           /* main wings */
            }
            if (y >= 12 && y <= 13 && dx <= 3) {
                v = 1;                           /* tail */
            }
            pix[y][x] = v;
        }
    }
    for (y = 4; y < 7; y++) {
        pix[y][7] = 2;
        pix[y][8] = 2;                           /* canopy */
    }
    encode_16x16(pix);
}

int main(void) {
    u16 keys;
    u16 pal;

    consoleInit();

    dmaCopyVramMode7(terrain_map, (u16)(terrain_map_end - terrain_map),
                     terrain_til, (u16)(terrain_til_end - terrain_til));
    dmaCopyCGram(terrain_pal, 0, (u16)(terrain_pal_end - terrain_pal));

    /* plane tiles 0,1/16,17 at OBJ base 3; the SAME shape with the
     * shadow palette makes the shadow sprite */
    build_plane();
    dmaCopyVram(shape_tiles, 0x6000, 64);
    dmaCopyVram(shape_tiles + 64, 0x6100, 64);

    /* palette 0 (CGRAM 128+): white body, blue canopy;
     * palette 1 (CGRAM 144+): all-dark = the shadow */
    pal = RGB(30, 30, 31);
    dmaCopyCGram((u8 *)&pal, 129, 2);
    pal = RGB(8, 12, 28);
    dmaCopyCGram((u8 *)&pal, 130, 2);
    pal = RGB(4, 8, 4);
    dmaCopyCGram((u8 *)&pal, 145, 2);
    pal = RGB(4, 8, 4);
    dmaCopyCGram((u8 *)&pal, 146, 2);

    setMode(BG_MODE7, 0);
    mode7Init();
    oamInit(OBJ_SIZE8_L16, 3);
    /* sprite 0 = shadow (behind), sprite 1 = plane */
    oamSet(0, PLANE_SCREEN_X, PLANE_SCREEN_Y, 0, 1, 2, 0);
    oamSetSize(0, OBJ_LARGE);
    oamSet(1, PLANE_SCREEN_X, PLANE_SCREEN_Y, 0, 0, 3, 0);
    oamSetSize(1, OBJ_LARGE);

    /* start parked on pad 1 (200,200) */
    plane_x = (u16)(200 << 4);
    plane_y = (u16)(232 << 4);
    plane_heading = 0;
    plane_speed = 0;
    plane_alt = 0;

    setMainScreen(TM_BG1 | LAYER_OBJ);
    setScreenOn();

    while (1) {
        s16 vx, vy;
        u16 idx, scale;

        WaitForVBlank();
        keys = padHeld(0);

        if (keys & KEY_LEFT) {
            plane_heading = (u8)(plane_heading - 2);
        }
        if (keys & KEY_RIGHT) {
            plane_heading = (u8)(plane_heading + 2);
        }
        if ((keys & KEY_A) && plane_speed < MAX_SPEED) {
            plane_speed += THROTTLE_STEP;
        }
        if ((keys & KEY_B) && plane_speed >= THROTTLE_STEP) {
            plane_speed -= THROTTLE_STEP;
        }
        if ((keys & KEY_UP) && plane_speed >= MIN_FLY_SPEED
                && plane_alt < MAX_ALT) {
            plane_alt += CLIMB_STEP;
        }
        if ((keys & KEY_DOWN) && plane_alt >= CLIMB_STEP) {
            plane_alt -= CLIMB_STEP;
        }
        /* too slow to fly: sink */
        if (plane_speed < MIN_FLY_SPEED && plane_alt >= 2) {
            plane_alt -= 2;
        }

        /* motion */
        vx = fixMul((s16)plane_speed, fixSin(plane_heading));
        vy = (s16)-fixMul((s16)plane_speed, fixCos(plane_heading));
        plane_x = (u16)(plane_x + (vx >> 4)) & 0x3FFF;   /* wrap the plane(t) */
        plane_y = (u16)(plane_y + (vy >> 4)) & 0x3FFF;

        idx = (u16)(((plane_y >> 7) << 7) + (plane_x >> 7));
        surface = terrain_class_at(idx);

        /* flight state: the landing EVENT only fires on the
         * airborne -> ground transition; taxiing on the ground (pads,
         * fields) is free rolling so takeoff is possible. */
        if (plane_alt > 0x60) {
            airborne = 1;
            setColor(0, RGB(10, 14, 24));         /* sky tint */
        } else if (plane_alt < 2) {
            if (surface == 1) {
                /* water is always a splash: respawn parked on pad 1 */
                plane_x = (u16)(200 << 4);
                plane_y = (u16)(232 << 4);
                plane_speed = 0;
                plane_alt = 0;
                plane_heading = 0;
                airborne = 0;
                setColor(0, RGB(8, 8, 24));
            } else if (airborne) {
                /* touchdown */
                if (surface == 2 && plane_speed <= LAND_SPEED) {
                    landings++;                   /* a good landing! */
                    plane_speed = 0;
                    airborne = 0;
                    setColor(0, RGB(6, 24, 6));
                } else if (plane_speed > MIN_FLY_SPEED) {
                    plane_alt = 0x0040;           /* too fast: bounce */
                    setColor(0, RGB(24, 12, 4));
                } else {
                    plane_speed >>= 1;            /* rough field touch */
                    airborne = 0;
                    setColor(0, RGB(20, 18, 6));
                }
            }
        }

        /* altitude -> scale (0x0100 + alt), then the matrix */
        scale = (u16)(0x0100 + plane_alt);
        mode7SetScale(scale, scale);
        mode7SetAngle(plane_heading);
        mode7SetCenter((s16)(plane_x >> 4), (s16)(plane_y >> 4));
        mode7SetScroll((s16)((plane_x >> 4) - PLANE_SCREEN_X),
                       (s16)((plane_y >> 4) - PLANE_SCREEN_Y));

        /* the shadow slides down-screen with altitude */
        oamSet(0, PLANE_SCREEN_X + 2,
               (u16)(PLANE_SCREEN_Y + 6 + (plane_alt >> 6)), 0, 1, 2, 0);
        oamSetSize(0, OBJ_LARGE);
    }

    return 0;
}
