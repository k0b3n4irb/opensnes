/**
 * @file main.c
 * @brief Eight-direction scrolling over a map of 1,433 distinct characters
 * @ingroup examples
 *
 * An archipelago of 1280x896 pixels, 160x112 characters, drawn with 1,433
 * distinct 8x8 characters once mirrored copies are merged: more than the
 * 1,024 a tilemap entry can name, so the whole set never fits VRAM. The
 * screen never shows more than 389 at once (the converter measured every
 * position), and the `tilering` module keeps a ring of 640 character slots
 * in VRAM: as the camera moves it loads the characters of the column and
 * the row that come into view, and frees the slots of those that leave.
 *
 * The camera flies a fixed course on its own — up, across, down at a
 * diagonal, at 3 to 7 pixels a frame — so that a test sees the same frames
 * every time; the D-pad takes over (B held: fast), START gives the course
 * back. The figures the course produced are kept in `tr_*` variables for
 * the test (testing/manifests/scrolling_tile_ring*.toml,
 * testing/tile_ring_budget.py).
 *
 * The map is invented: devtools/art_assets/tile_ring.py draws it from a
 * fixed seed.
 *
 * @par SNES Concepts
 * - A tilemap entry names 1,024 characters: a large map needs streaming
 * - A ring of character slots, freed by reference count, refilled in sequence
 * - A 64x32 tilemap as a window that follows the camera on both axes
 * - VRAM transfers noted during the frame, sent in VBlank (vramqueue)
 * - The VBlank budget: what one frame of scrolling costs, NTSC and PAL
 *
 * @par What to Observe
 * - The map scrolls in all eight directions without a wrong character
 * - Nothing to see of the ring itself: that is the point
 *
 * @par Modules Used
 * console, background, input, tilering, vramqueue
 *
 * @see tilering.h, vramqueue.h
 */

#include <snes.h>
#include <snes/tilering.h>
#include <snes/vramqueue.h>

/** @brief The map (opensnes-tilering, from res/archipelago.png and its settings file) */
#include "res/archipelago.inc"

/*============================================================================
 * VRAM layout
 *============================================================================*/

/** @brief BG1 characters: the ring's 640 slots, $0000-$27FF (20 KB) */
#define CHR_VRAM    0x0000
/** @brief Slots in the ring: the worst screen (389) and room to keep characters for a return */
#define RING_SLOTS  640
/** @brief BG1 tilemap, 64x32 entries (4 KB), above the slots */
#define MAP_VRAM    0x3000

/** @brief Camera limits: the map less one screen */
#define CAM_MAX_X   (archipelago_WIDTH * 8 - 256)
/** @brief Camera limits: the map less one screen (224 lines) */
#define CAM_MAX_Y   (archipelago_HEIGHT * 8 - 224)

/*============================================================================
 * The ring and its work areas
 *============================================================================*/

/** @brief What the converter wrote, in one structure */
static const TileRingMap world = archipelago_TILERING_MAP;
/** @brief The ring's state (plain RAM) */
TileRing ring;
/** @brief The slot of each map character */
FAR u16 slot_of[archipelago_CHARS];
/** @brief The character of each slot */
FAR u16 char_of[RING_SLOTS];
/** @brief Cells of the window showing each slot */
FAR u8 refs[RING_SLOTS];
/** @brief The frame's new characters, copied from ROM, sent by one or two DMAs */
FAR u8 staging[TILERING_STAGING_BYTES(33, 29)];

/*============================================================================
 * The course
 *============================================================================*/

/** @brief One leg: where the camera goes, and how fast (pixels a frame on each axis) */
typedef struct {
    u16 x, y, speed;
} Leg;

/** @brief The course, from the bottom-left corner and back to it */
static const Leg course[] = {
    {          0,          0, 7 },     /* up the left edge */
    {  CAM_MAX_X,          0, 7 },     /* across the top */
    {        352,  CAM_MAX_Y, 5 },     /* down-left at a diagonal */
    {          0,  CAM_MAX_Y, 7 },     /* left along the bottom */
    {  CAM_MAX_Y,          0, 5 },     /* up-right at a diagonal */
    {  CAM_MAX_X,          0, 7 },
    {  CAM_MAX_X,  CAM_MAX_Y, 7 },     /* down the right edge */
    {          0,  CAM_MAX_Y, 3 },     /* home, slowly */
};
/** @brief Legs in the course */
#define LEGS (sizeof(course) / sizeof(course[0]))

/** @brief Camera position in map pixels */
u16 camx, camy;
/** @brief The leg being flown */
u16 leg;
/** @brief 1 while the D-pad drives the camera */
u16 manual;

/*============================================================================
 * Figures for the test
 *============================================================================*/

/** @brief 1 = direct mode (no staging buffer); a test pokes it before main */
u16 tr_direct;
/** @brief Frames of the first lap, and laps completed */
u16 tr_frames, tr_laps;
/** @brief Most characters one frame queued (first lap) */
u16 tr_max_chars;
/** @brief Most queue entries one frame noted (first lap) */
u16 tr_max_entries;
/** @brief Most vramQueueCost() before a flush, in bytes (first lap; x 8 = master cycles) */
u16 tr_max_cost;
/** @brief Characters loaded over the first lap */
u16 tr_total_chars;
/** @brief Most slots in use at once (first lap) */
u16 tr_max_resident;
/** @brief The latest scanline a flush ended on, counted from the start of VBlank (line 225) */
u16 tr_max_lines;
/** @brief Characters that found no free slot, transfers the queue refused: 0 and 0 */
u16 tr_starved, tr_refused;
/** @brief The worst frame the module predicts for the course's fastest legs (7 px, and 5 + 5) */
u16 tr_cost_7, tr_cost_55;

/**
 * @brief The scanline now, from the PPU's latched counter
 * @return 0-261 (NTSC) or 0-311 (PAL)
 */
static u16 scanline(void) {
    u16 lo, hi;
    (void)REG_STAT78;          /* resets the counters' low/high read order */
    (void)REG_SLHV;            /* latches them */
    lo = REG_OPVCT;
    hi = REG_OPVCT & 1;
    return lo | (hi << 8);
}

/**
 * @brief Move v toward target by at most step
 * @param v Current value
 * @param target Where it goes
 * @param step Most it moves
 * @return The new value
 */
static u16 approach(u16 v, u16 target, u16 step) {
    if (v < target)
        return (target - v > step) ? v + step : target;
    if (v > target)
        return (v - target > step) ? v - step : target;
    return v;
}

/**
 * @brief Entry point — the archipelago, flown over
 * @return Never returns
 */
int main(void) {
    u16 pad, step, lines;

    consoleInit();
    setMode(BG_MODE1, 0);

    dmaCopyCGram(archipelago_pal, 0, archipelago_pal_end - archipelago_pal);
    bgSetGfxPtr(0, CHR_VRAM);
    bgSetMapPtr(0, MAP_VRAM, SC_64x32);

    tileRingInit(&ring, &world, CHR_VRAM, 0, RING_SLOTS, MAP_VRAM,
                 slot_of, char_of, refs, tr_direct ? (u8 FAR *)0 : staging);
    tr_cost_7 = tileRingCost(&ring, 7, 0);
    tr_cost_55 = tileRingCost(&ring, 5, 5);

    camx = 0;
    camy = CAM_MAX_Y;
    tileRingFill(&ring, camx, camy);        /* the first screen, in force blank */
    bgSetScroll(0, camx, camy);

    setMainScreen(LAYER_BG1);
    setScreenOn();

    while (1) {
        pad = padHeld(0);
        if (pad & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT))
            manual = 1;
        if (pad & KEY_START)
            manual = 0;

        if (manual) {
            step = (pad & KEY_B) ? 7 : 2;
            if ((pad & KEY_LEFT) && camx >= step) camx -= step;
            if ((pad & KEY_RIGHT) && camx + step <= CAM_MAX_X) camx += step;
            if ((pad & KEY_UP) && camy >= step) camy -= step;
            if ((pad & KEY_DOWN) && camy + step <= CAM_MAX_Y) camy += step;
        } else {
            camx = approach(camx, course[leg].x, course[leg].speed);
            camy = approach(camy, course[leg].y, course[leg].speed);
            if (camx == course[leg].x && camy == course[leg].y) {
                leg++;
                if (leg == LEGS) {
                    leg = 0;
                    tr_laps++;
                }
            }
        }

        tileRingUpdate(&ring, camx, camy);
        bgSetScroll(0, camx, camy);         /* applied by the next VBlank, with the queue */

        if (tr_laps == 0) {
            tr_frames++;
            tr_total_chars += ring.last_chars;
            if (ring.last_chars > tr_max_chars) tr_max_chars = ring.last_chars;
            if (ring.last_entries > tr_max_entries) tr_max_entries = ring.last_entries;
            if (vramQueueCost() > tr_max_cost) tr_max_cost = vramQueueCost();
            if (ring.resident > tr_max_resident) tr_max_resident = ring.resident;
        }
        tr_starved = ring.starved;
        tr_refused = ring.refused + vram_queue_refused;

        WaitForVBlank();
        vramQueueFlush();
        lines = scanline();
        lines = (lines >= 225) ? lines - 225 : lines + (isPAL() ? 312 : 262) - 225;
        if (lines > tr_max_lines) tr_max_lines = lines;
    }

    return 0;
}
