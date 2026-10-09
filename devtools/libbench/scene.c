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
#endif
    done = 0x600D;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
