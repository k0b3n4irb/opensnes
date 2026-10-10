/*
 * scene.c — the same library calls through both SDKs.
 *
 * Each row is what a game asks of the library in one frame, repeated REPS
 * times: read the pad, scroll three backgrounds, place 32 sprites, move
 * them, change their size, send 2 KB to VRAM, print 20 characters. ROW
 * (from which.h) selects the row; CALLS = 0 compiles the same loops with
 * the library calls left out — the loop bodies are empty — and the runner
 * subtracts that ROM from the one with CALLS = 1: what remains is the
 * calls, arguments included, and nothing of the loop around them.
 *
 * PVS (from which.h) is 1 when PVSnesLib builds this file. The names and
 * the argument lists differ between the SDKs; the work asked for does not.
 */
#include <snes.h>
#include "which.h"
#if !PVS
#include <snes/vramqueue.h>
#endif

u16 res;
u16 done;

#define REPS 20

static u8 payload[2048];
#if PVS
extern char tilfont, palfont;
#endif

void row_pad(void) {
    u16 r, i;
    for (r = 0; r < REPS; r++) {
        for (i = 0; i < 10; i++) {
#if CALLS
#if PVS
            res += padsCurrent(0);
            res += padsDown(0);
            res += padsUp(0);
#else
            res += padHeld(0);
            res += padPressed(0);
            res += padReleased(0);
#endif
#endif
        }
    }
}

void row_scroll(void) {
    u16 r, i;
    for (r = 0; r < REPS; r++) {
        for (i = 0; i < 10; i++) {
#if CALLS
            bgSetScroll(0, r + i, i);
            bgSetScroll(1, r, r);
            bgSetScroll(2, i, r + i);
#endif
        }
    }
}

void row_oamset(void) {
    u16 r, i;
    for (r = 0; r < REPS; r++) {
        for (i = 0; i < 32; i++) {
#if CALLS
#if PVS
            oamSet(i << 2, i << 3, r + i, 3, 0, 0, i, 0);
#else
            oamSet(i, i << 3, r + i, i, 0, 3, 0);
#endif
#endif
        }
    }
}

void row_oamxy(void) {
    u16 r, i;
    for (r = 0; r < REPS; r++) {
        for (i = 0; i < 32; i++) {
#if CALLS
#if PVS
            oamSetXY(i << 2, i << 3, r + i);
#else
            oamSetXY(i, i << 3, r + i);
#endif
#endif
        }
    }
}

void row_oamsize(void) {
    u16 r, i;
    for (r = 0; r < REPS; r++) {
        for (i = 0; i < 32; i++) {
#if CALLS
#if PVS
            oamSetEx(i << 2, OBJ_LARGE, OBJ_SHOW);
#else
            oamSetSize(i, 1);
#endif
#endif
        }
    }
}

void row_dma(void) {
    u16 r;
    for (r = 0; r < REPS; r++) {
#if CALLS
        dmaCopyVram(payload, 0x4000, 2048);
#endif
    }
}

void row_text(void) {
    u16 r;
    for (r = 0; r < REPS; r++) {
#if CALLS
#if PVS
        consoleDrawText(2, 4 + (r & 15), "TWENTY CHARACTERS OK");
#else
        textPrintAt(2, 4 + (r & 15), "TWENTY CHARACTERS OK");
#endif
#endif
        /* the frame boundary: PVSnesLib uploads the text in its handler */
        WaitForVBlank();
    }
}

/* World-space sprites (issue #165, after a real project's match screen): 19
 * sprites in world coordinates, 18 of 32x32 and one of 16x16, 13 of them on
 * screen, placed once a frame against a camera that moves.
 *   worldc: the loop in C, the same source for both SDKs, writing the OAM
 *           shadow directly — the cheapest a game could write it in C.
 *   world:  OpenSNES calls oamPlaceWorld(), twice (one size a call);
 *           PVSnesLib has no such call and runs the C loop. */
#define WN 19
static s16 wx[WN];
static s16 wy[WN];
static u8 wtile[WN];
static u8 wattr[WN];
static u8 wsize[WN];
static u8 wxhi[WN];
static u8 wseen[WN];

static void world_setup(void) {
    u16 i;
    for (i = 0; i < WN; i++) {
        wx[i] = (i < 12) ? i * 18 : 600 + i;       /* players 12-17 are off screen */
        wy[i] = 40 + (i & 3) * 40;
        wtile[i] = i << 2;
        wattr[i] = 0x20;
        wsize[i] = 32;
        wxhi[i] = 1 << ((i & 3) << 1);
        wseen[i] = 0;
    }
    wx[18] = 120;                                  /* the ball: 16x16, on screen */
    wsize[18] = 16;
}

static void world_c(u16 cam_x, u16 cam_y) {
    u16 id;
    for (id = 0; id < WN; id++) {
        u16 x = wx[id] - cam_x;
        u16 y = wy[id] - cam_y;
        u16 size = wsize[id];
        u8 *oam = (u8 *)oamMemory + (id << 2);
        u8 *high = (u8 *)oamMemory + 512 + (id >> 2);
        if ((u16)(x + size) < 256 + size && (u16)(y + size) < 224 + size) {
            oam[0] = x;
            oam[1] = y;
            oam[2] = wtile[id];
            oam[3] = wattr[id];
            if (x & 0x100) *high |= wxhi[id];
            else *high &= ~wxhi[id];
            wseen[id] = 1;
        } else {
            oam[0] = 1;
            oam[1] = 240;
            *high |= wxhi[id];
            wseen[id] = 0;
        }
    }
}

void row_worldc(void) {
    u16 r;
    world_setup();
    for (r = 0; r < REPS; r++) {
#if CALLS
        world_c(r, 0);
#endif
    }
}

void row_world(void) {
    u16 r;
#if !PVS
    static OamWorldBatch big;
    static OamWorldBatch small;
    big.x = wx; big.y = wy; big.tile = wtile; big.attr = wattr; big.visible = wseen;
    big.first_id = 0; big.count = 18; big.size = 32;
    small.x = wx + 18; small.y = wy + 18; small.tile = wtile + 18; small.attr = wattr + 18;
    small.visible = wseen + 18; small.first_id = 18; small.count = 1; small.size = 16;
#endif
    world_setup();
    for (r = 0; r < REPS; r++) {
#if CALLS
#if PVS
        world_c(r, 0);
#else
        oamPlaceWorld(&big, r, 0);
        oamPlaceWorld(&small, r, 0);
#endif
#endif
    }
}

/* Six small VRAM transfers, 128 bytes each, as a game streaming sprite
 * frames makes in one VBlank (issue #165).
 *   vramc: six dmaCopyVram() calls, both SDKs.
 *   vramq: what the six cost IN VBLANK, where time is short. OpenSNES notes
 *          them during the frame (not timed: both ROMs do it) and pays one
 *          vramQueueFlush(); PVSnesLib has no queue and pays the six calls.
 *          In total the queue costs more than the calls (noting an entry is
 *          a call too); it is the VBlank it relieves. */
void row_vramc(void) {
    u16 r, i;
    for (r = 0; r < REPS; r++) {
        for (i = 0; i < 6; i++) {
#if CALLS
            dmaCopyVram(payload + (i << 7), 0x4000 + (i << 8), 128);
#endif
        }
    }
}

void row_vramq(void) {
    u16 r, i;
    for (r = 0; r < REPS; r++) {
        for (i = 0; i < 6; i++) {
#if PVS
#if CALLS
            dmaCopyVram(payload + (i << 7), 0x4000 + (i << 8), 128);
#endif
#else
            /* noted during the frame, in both ROMs: not what this row times */
            vramQueuePush(payload + (i << 7), 0x4000 + (i << 8), 128, VRAM_QUEUE_ROW);
#endif
        }
#if !PVS
#if CALLS
        vramQueueFlush();
#else
        vram_queue_count = 0;
#endif
#endif
    }
}

/* All of it in one frame, REPS frames in a row: the pad, three scrolls, 32
 * sprites placed, 20 characters, then the frame boundary — where each SDK's
 * handler sends what the calls left for it (PVSnesLib uploads the sprite
 * table every frame, OpenSNES when a sprite changed). */
void row_frame(void) {
    u16 r, i;
    for (r = 0; r < REPS; r++) {
#if CALLS
#if PVS
        res += padsCurrent(0);
        res += padsDown(0);
        consoleDrawText(2, 4 + (r & 15), "TWENTY CHARACTERS OK");
#else
        res += padHeld(0);
        res += padPressed(0);
        textPrintAt(2, 4 + (r & 15), "TWENTY CHARACTERS OK");
#endif
        bgSetScroll(0, r, r);
        bgSetScroll(1, r, 0);
        bgSetScroll(2, 0, r);
#endif
        for (i = 0; i < 32; i++) {
#if CALLS
#if PVS
            oamSet(i << 2, i << 3, r + i, 3, 0, 0, i, 0);
#else
            oamSet(i, i << 3, r + i, i, 0, 3, 0);
#endif
#endif
        }
        WaitForVBlank();
    }
}

int main(void) {
#if PVS
    consoleSetTextMapPtr(0x6800);
    consoleSetTextGfxPtr(0x3000);
    consoleSetTextOffset(0x0100);
    consoleInitText(0, 16 * 2, &tilfont, &palfont);
    bgSetGfxPtr(0, 0x2000);
    bgSetMapPtr(0, 0x6800, SC_32x32);
    setMode(BG_MODE1, 0);
#else
    textModeInit();
#endif
#if ROW == 1
    row_pad();
#elif ROW == 2
    row_scroll();
#elif ROW == 3
    row_oamset();
#elif ROW == 4
    row_oamxy();
#elif ROW == 5
    row_oamsize();
#elif ROW == 6
    row_dma();
#elif ROW == 7
    row_text();
#elif ROW == 8
    row_frame();
#elif ROW == 9
    row_worldc();
#elif ROW == 10
    row_world();
#elif ROW == 11
    row_vramc();
#elif ROW == 12
    row_vramq();
#endif
    done = 0x600D;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
