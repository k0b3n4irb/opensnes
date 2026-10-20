/**
 * @file main.c
 * @brief Mode 7 racing — hover-bikes on a rooftop circuit, F-Zero style
 * @ingroup examples
 *
 * A drivable race circuit on the Mode 7 plane. Kit's hover-bike stays
 * fixed near the bottom of the screen and the WORLD rotates and scrolls
 * under it — the classic SNES racing camera:
 * per frame, mode7SetAngle(heading) + mode7SetScroll/SetCenter follow
 * the bike's fixed-point position, through the mode7 module.
 *
 * - D-pad left/right: steer — A: accelerate — B: brake
 * - The gravel slows the bike down; the lagoon, the quay and the rooftop
 *   props stop it; the chevron pads throw it forward.
 * - Two rivals leave the line when Kit does, and ride their lanes.
 *
 * The circuit, its 128x128 class map (one byte per tile), the rivals'
 * lanes and the bike sheet are written by devtools/art_assets/build.py
 * (racing.py) from the PixelLab art of assets/neo-lumen/; the build
 * converts the circuit with opensnes-tileset (mode = 7) and the bikes
 * with opensnes-sprite.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - The F-Zero camera: SetCenter(bike) makes the bike the rotation
 *   pivot, SetScroll places it low on screen, SetAngle spins the
 *   world opposite to the bike's heading
 * - Sprites standing on a turning Mode 7 plane: a rival's world position
 *   goes through the inverse of the plane's matrix to reach the screen,
 *   and its picture turns with the camera (five frames and the flips)
 * - Fixed-point driving physics from the math module: velocity =
 *   fixMul(speed, fixSin/fixCos(heading)) — same 0-255 angle unit as
 *   mode7SetAngle, so heading feeds both directly
 * - Mode 7 VRAM interleave loaded with dmaCopyVramMode7(); OBJ tiles
 *   coexist at name base 3 (word $6000)
 *
 * @par What to Observe
 * Boot on the start line, a rival on each side. Hold A: the checker line
 * recedes, the rivals pull away and the track streams under the bike;
 * steer through the chicane. A rival seen side-on turns its bike with the
 * camera. Cutting the gravel slows you; a chevron pad gives a burst of
 * speed; the quay is solid.
 *
 * @par Modules Used
 * console, dma, background, sprite, math, input, mode7
 *
 * @see devtools/art_assets/racing.py — how the circuit and the bikes are made
 * @see lib/source/mode7.c — the C Mode 7 module this game exercises
 */

#include <snes.h>
#include <snes/mode7.h>
#include <snes/math.h>
#include <snes/input.h>

#include "res/track.inc"   /* generated: track_tiles[], track_map[], track_pal[] (Mode 7 .pc7 / .mp7), each with _end */
#include "res/bikes.inc"   /* generated: bikes_tiles[], bikes_pal[], each with _end */

/** @brief Class of each track tile, 128x128 (data.asm, res/track_class.bin):
 * 0 road, 1 gravel, 2 water or a prop, 3 boost pad. */
extern const u8 track_class[];
/** @brief The rivals' lanes (data.asm): 2 lanes x 256 points of (x, y) */
extern const u16 lanes_xy[];
/** @brief The heading at each point of the two lanes (64 = east) */
extern const u8 lanes_heading[];

/** @brief Screen position of the bike's centre (the rotation pivot) */
#define CAR_SCREEN_X 128
#define CAR_SCREEN_Y 168

/** @brief Physics tuning (positions in 12.4 fixed, speeds in 8.8) */
#define ACCEL        6
#define BRAKE        12
#define DRAG         2
#define MAX_SPEED    0x0300     /* 3.0 px/frame */
#define MAX_GRASS    0x00C0     /* gravel cap: 0.75 px/frame */
#define BOOST_SPEED  0x0500     /* a pad: 5.0 px/frame, then back to MAX_SPEED */
#define STEER_STEP   2

/** @brief Tiles of the bike sheet: 32x32 frames four to a row, 16 tiles a row */
#define FRAME_TILE(f) (((f) >> 2) * 64 + ((f) & 3) * 4)
/** @brief Kit's frame, then each rival's five turned frames (0 to 90 degrees) */
#define KIT_FRAME    0
#define RIVAL_FRAME(r) (1 + (r) * 5)

/** @brief Probe oracles / game state (12.4 fixed-point pixels) */
u16 car_x;
u16 car_y;
u16 car_speed;                  /* 8.8 px/frame */
u8 car_heading;                 /* 0-255, the shared angle unit */
u8 car_surface;                 /* current class under the bike */
/** @brief Where each rival is on its lane: point number in 8.8 (256 points a lap) */
u16 rival_pos[2];
/** @brief Set once Kit moves: the rivals wait for him on the line */
u8 race_on;

/** @brief Each rival's speed along its lane, points per frame in 8.8 */
static const u16 rival_speed[2] = { 0x0050, 0x0056 };

/**
 * @brief Put a world point (pixels) on the screen, through the camera.
 *
 * The plane shows world = car + M * (screen - pivot), M = the matrix
 * mode7SetAngle() writes at scale 0x0080 (cos, -sin / sin, cos, halved);
 * this is the inverse, M^-1 = 2 * (cos, sin / -sin, cos).
 *
 * @return 0 if the point is too far from the bike to be on screen
 */
static u8 to_screen(s16 wx, s16 wy, s16 *sx, s16 *sy) {
    s16 dx = (s16)(wx - (s16)(car_x >> 4));
    s16 dy = (s16)(wy - (s16)(car_y >> 4));
    fixed c, s;

    if (dx < -160 || dx > 160 || dy < -160 || dy > 160)
        return 0;
    c = fixCos(car_heading);
    s = fixSin(car_heading);
    *sx = (s16)(CAR_SCREEN_X + 2 * (fixMul(dx, c) + fixMul(dy, s)));
    *sy = (s16)(CAR_SCREEN_Y + 2 * (fixMul(dy, c) - fixMul(dx, s)));
    return (u8)(*sx > -16 && *sx < 272 && *sy > -16 && *sy < 240);
}

/**
 * @brief Draw rival `r` and its shadow where its lane puts it.
 *
 * Its picture turns with the camera: the direction it faces on screen is
 * its heading minus Kit's, in 16 steps of 22.5 degrees. Five frames turn
 * the bike from up (0) to right (4); the flips give the rest — V turns
 * a direction d into 8 - d, H into 16 - d, both into d + 8.
 */
static void draw_rival(u8 r) {
    u16 lane = (u16)(r * 256);
    u8 i = (u8)(rival_pos[r] >> 8);
    u8 j = (u8)(i + 1);
    s16 f = (s16)(rival_pos[r] & 0xFF);
    const u16 *a = &lanes_xy[(u16)((lane + i) * 2)];
    const u16 *b = &lanes_xy[(u16)((lane + j) * 2)];
    s16 wx = (s16)(a[0] + (((s16)(b[0] - a[0]) * f) >> 8));
    s16 wy = (s16)(a[1] + (((s16)(b[1] - a[1]) * f) >> 8));
    s16 sx, sy;
    u8 d, frame, flags;

    if (!to_screen(wx, wy, &sx, &sy)) {
        oamHide((u16)(1 + r));
        oamHide((u16)(4 + r));
        return;
    }
    d = (u8)(((u8)(lanes_heading[lane + i] - car_heading) + 8) >> 4) & 15;
    if (d <= 4)       { frame = d;              flags = 0; }
    else if (d <= 8)  { frame = (u8)(8 - d);    flags = 0x80; }
    else if (d <= 12) { frame = (u8)(d - 8);    flags = 0xC0; }
    else              { frame = (u8)(16 - d);   flags = 0x40; }

    /* sprites 1-2 the rivals, 4-5 their shadows: a lower OAM index is in front */
    oamSet((u16)(1 + r), (u16)(sx - 16), (u16)(sy - 16),
           FRAME_TILE(RIVAL_FRAME(r) + frame), 0, 3, flags);
    oamSet((u16)(4 + r), (u16)(sx - 12), (u16)(sy - 10),
           FRAME_TILE(RIVAL_FRAME(r) + frame), 1, 3, flags);
}

int main(void) {
    u16 keys;
    u8 i;

    consoleInit();

    /* Mode 7 plane: interleaved tiles+map upload */
    dmaCopyVramMode7(track_map, (u16)(track_map_end - track_map),
                     track_tiles, (u16)(track_tiles_end - track_tiles));
    dmaCopyCGram(track_pal, 0, (u16)(track_pal_end - track_pal));

    /* The bikes at OBJ name base 3 (word $6000), in forced blank (6 KB):
     * palette 0 their colours, palette 1 all dark for the shadows */
    dmaCopyVram(bikes_tiles, 0x6000, (u16)(bikes_tiles_end - bikes_tiles));
    dmaCopyCGram(bikes_pal, OBJ_CGRAM_BASE, PALETTE_16_SIZE);
    for (i = 1; i < 16; i++)
        setColor((u16)(OBJ_CGRAM_BASE + 16 + i), RGB(1, 2, 3));

    setMode(BG_MODE7, 0);
    mode7Init();
    mode7SetScale(0x0080, 0x0080);   /* the camera: the track magnified twice */
    oamInit(OBJ_SIZE8_L32, 3);
    for (i = 0; i < 6; i++)
        oamSetSize(i, OBJ_LARGE);
    /* sprite 0 = Kit, sprite 3 = his shadow; it floats, so the shadow is offset */
    oamSet(0, CAR_SCREEN_X - 16, CAR_SCREEN_Y - 16, FRAME_TILE(KIT_FRAME), 0, 3, 0);
    oamSet(3, CAR_SCREEN_X - 12, CAR_SCREEN_Y - 10, FRAME_TILE(KIT_FRAME), 1, 3, 0);

    /* Start on the line (world 312, 272), heading +x (east) */
    car_x = (u16)(312 << 4);
    car_y = (u16)(272 << 4);
    car_heading = 64;
    car_speed = 0;
    rival_pos[0] = 0x0100;           /* the rivals one point ahead, either side */
    rival_pos[1] = 0x0100;
    race_on = 0;

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

        /* the surface under the bike caps the speed; a pad throws it forward */
        if (car_surface == 3) {
            car_speed = BOOST_SPEED;
        }
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

        /* class of the DESTINATION tile: water and props block, else move */
        px = (u16)(nx >> 7);                     /* 12.4 -> tile (px>>3) */
        py = (u16)(ny >> 7);
        car_surface = track_class[(u16)((py << 7) + px)];
        if (car_surface == 2) {
            car_speed = 0;                       /* solid: full stop */
        } else {
            car_x = nx;
            car_y = ny;
        }

        /* the F-Zero camera: pivot on the bike, the bike low on screen */
        mode7SetAngle(car_heading);
        mode7SetCenter((s16)(car_x >> 4), (s16)(car_y >> 4));
        mode7SetScroll((s16)((car_x >> 4) - CAR_SCREEN_X),
                       (s16)((car_y >> 4) - CAR_SCREEN_Y));

        /* the rivals leave the line with Kit, then never wait */
        if (car_speed != 0)
            race_on = 1;
        for (i = 0; i < 2; i++) {
            if (race_on)
                rival_pos[i] = (u16)(rival_pos[i] + rival_speed[i]);
            draw_rival(i);
        }
    }

    return 0;
}
