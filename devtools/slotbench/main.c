/*
 * slotbench: N sprites 32x32, each with its own VRAM slot, against the
 * dynamic engine. A tick is two frames; each tick a generated table says how
 * many sprites change frame. See README.md. Contributor bench, not an example.
 *
 * BENCH_FIXED  0: oamDynamic32Draw and its NMI flush (7 frames at most)
 *              1: a C loop over the slots, vramQueuePushSprite, oamSet
 *              2: the same with the decision and the push in assembly (slots.asm)
 * BENCH_N      sprites
 * BENCH_FRAMES frames allowed in one VBlank's queue (fixed slots only)
 * BENCH_UNTIL  0: vramQueueFlush  1: vramQueueFlushUntil(BENCH_LINE)
 */
#include <snes.h>
#include <snes/vramqueue.h>
#include "scenario.h"

#define FRAME_COST (512 + 4 * VRAM_QUEUE_ENTRY_COST)
#define LINE_COST  (64 + VRAM_QUEUE_ENTRY_COST)
#define BUDGET     (BENCH_FRAMES * FRAME_COST + 2 * LINE_COST)
#define BENCH_LINE 258
#define FREEZE_TICK 1000   /* no change after it: the slots must settle */

extern void oamDynamic32Draw(u16 id);
extern u16 oamqueuenumber;

u8 slot_want[BENCH_N];
u8 slot_sent[BENCH_N];
u16 slot_vram[BENCH_N];
u16 slot_tile[BENCH_N];
u16 slot_next;
u16 pos_x[BENCH_N];
u16 pos_y[BENCH_N];

u16 scn_tick;
u16 scn_pick;
u16 scn_frame;

/* read back with `luna state --peek` */
u16 bench_frames;
u16 bench_drain;
u16 bench_drain_max;
u16 bench_done;

static const u8 map_line[64] = { 0 };

/* What the game would do: pick who changes. Not measured. */
void scenario_tick(void)
{
    u16 c, id, i;
    c = tick_changes[scn_tick & 255];
    if (scn_tick >= FREEZE_TICK) c = 0;
    if (c > BENCH_N) c = BENCH_N;
    while (c) {
        id = scn_pick;
        scn_pick += 7;
        while (scn_pick >= BENCH_N) scn_pick -= BENCH_N;
        scn_frame += 37;
        if (scn_frame >= NFRAMES) scn_frame -= NFRAMES;
        slot_want[id] = (u8)scn_frame;
#if BENCH_FIXED == 0
        OAM_SET_GFX(id, frame_src[scn_frame]);
        oambuffer[id].oamframeid = 0;
        oambuffer[id].oamrefresh = 1;
#endif
        c--;
    }
    for (i = 0; i < BENCH_N; i++) {
        pos_x[i] = (pos_x[i] + 1 + (i & 3)) & 0xDF;
#if BENCH_FIXED == 0
        oambuffer[i].oamx = (s16)pos_x[i];
#endif
    }
    scn_tick++;
}

#if BENCH_FIXED == 1
/* the decision and the queueing: what a module would replace */
void slotsUpdate(void)
{
    u16 i, n;
    u8 f;
    i = slot_next;
    n = BENCH_N;
    do {
        f = slot_want[i];
        if (f != slot_sent[i]) {
            if (vramQueueCost() + FRAME_COST > BUDGET) break;
            vramQueuePushSprite(frame_src[f], slot_vram[i], 32);
            slot_sent[i] = f;
        }
        i++;
        if (i == BENCH_N) i = 0;
    } while (--n);
    slot_next = i;
}
#elif BENCH_FIXED == 2
extern void slotsUpdate(void);
u16 slot_budget = BUDGET - FRAME_COST;
u16 slot_count = BENCH_N;
#endif

#if BENCH_FIXED
void slotsPlace(void)
{
    u16 i;
    for (i = 0; i < BENCH_N; i++)
        oamSet(i, pos_x[i], pos_y[i], slot_tile[i], 0, 3, 0);
}
#endif

/* After the sends of this VBlank: what is left? */
void bench_stats(void)
{
    u16 pending;
#if BENCH_FIXED
    u16 i;
#endif
    bench_frames++;
#if BENCH_FIXED == 0
    pending = oamqueuenumber;
#else
    pending = vram_queue_count;
    for (i = 0; i < BENCH_N; i++)
        if (slot_want[i] != slot_sent[i]) pending = 1;
#endif
    if (pending) {
        bench_drain++;
        if (bench_drain > bench_drain_max) bench_drain_max = bench_drain;
    } else {
        bench_drain = 0;
        if (scn_tick > FREEZE_TICK) bench_done = 1;
    }
}

int main(void)
{
    u16 i;

    consoleInit();
    setScreenOff();
#if BENCH_FIXED == 0
    {
        static const OamDynamicConfig dyn = {
            .vramLarge = 0x0000, .vramSmall = 0x1000,
            .slotLargeInit = 0, .slotSmallInit = 0,
            .sizeMode = OBJ_SIZE16_L32,
        };
        oamDynamicInit(&dyn);
    }
#else
    oamInit(OBJ_SIZE16_L32, OAM_DEFAULT_TILE_BASE);
#endif
    for (i = 0; i < BENCH_N; i++) {
        slot_tile[i] = i < 16 ? (u16)(((i & 3) * 4) + ((i >> 2) * 0x40))
                              : (u16)(0x100 + (i - 16) * 4);
        slot_vram[i] = (u16)(slot_tile[i] * 16);
        slot_want[i] = 0;
        pos_x[i] = (u16)(i * 12);
        pos_y[i] = (u16)((i & 7) * 24 + 8);
#if BENCH_FIXED == 0
        oambuffer[i].oamx = (s16)pos_x[i];
        oambuffer[i].oamy = (s16)pos_y[i];
        oambuffer[i].oamframeid = 0;
        oambuffer[i].oamattribute = OBJ_PRIO(3);
        oambuffer[i].oamrefresh = 0;
        OAM_SET_GFX(i, frame_src[0]);
#else
        oamSetSize(i, OBJ_LARGE);
        vramQueuePushSprite(frame_src[0], slot_vram[i], 32);
        vramQueueFlush();
#endif
    }
#if BENCH_FIXED == 0
    for (i = 0; i < BENCH_N; i++) oambuffer[i].oamrefresh = 1;
    for (i = 0; i < BENCH_N; i++) oamDynamic32Draw(i);
    oamDynamicDrainQueue();
#endif
    setScreenOn();

    for (;;) {
        WaitForVBlank();
#if BENCH_UNTIL
        vramQueueFlushUntil(BENCH_LINE);
#else
        vramQueueFlush();
#endif
        bench_stats();

        if (!(bench_frames & 1)) scenario_tick();
        vramQueuePush(map_line, 0x6000, 64, VRAM_QUEUE_ROW);
        vramQueuePush(map_line, 0x6000, 64, VRAM_QUEUE_COLUMN);
#if BENCH_FIXED == 0
        for (i = 0; i < BENCH_N; i++) oamDynamic32Draw(i);
#else
        slotsUpdate();
        slotsPlace();
        oam_update_flag = 1;
#endif
    }
    return 0;
}
