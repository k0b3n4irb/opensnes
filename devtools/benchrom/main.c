/*
 * benchrom — the C1 audit's measuring instrument.
 *
 * For each measured function: run it in a tight N-iteration loop and
 * record how many NMI frames elapsed (frame_count is incremented by
 * the NMI handler regardless of WaitForVBlank). The python runner
 * (bench.py) converts frames -> ~CPU cycles/call after subtracting
 * the calibration loop (same loop shape, empty body), so loop
 * overhead and the NMI handler's own per-frame cost cancel out.
 *
 * Relative precision is the point: the SAME harness measures the ASM
 * original and its C port — the +-10 % migration rule of the C1 audit
 * (.claude/notes/chantiers/c1_asm_audit.md) compares two numbers
 * produced by identical machinery.
 *
 * Results are WRAM globals read by symbol name (luna state --peek),
 * libtest-style. r_bench_done = 0xBEEF marks completion.
 */

#include <snes.h>
#include <snes/mode7.h>
#include <snes/map.h>
#include <snes/sprite.h>
#include <snes/anim.h>

/* Iterations per measured function. Chosen so cheap fns still span
 * >= ~20 frames (quantization < 5 %). volatile so the loop counter
 * compare can't be folded. */
#define N_ITER 20000u

/* --- results: frames elapsed per N_ITER-loop --- */
u16 r_cal_empty;     /* calibration: empty loop                    */
u16 r_m7_setangle;   /* mode7SetAngle(a++)                         */
u16 r_m7_setscale;   /* mode7SetScale(s, s)                        */
u16 r_m7_setcenter;  /* mode7SetCenter(x, y)                       */
u16 r_m7_setmatrix;  /* mode7SetMatrix(a, b, c, d)                 */
u16 r_m7_transform;  /* mode7Transform(deg, scale) — the heavy one */

/* map.asm measurement points (real map_scroll data, loaded once).
 * mapVblank is called OUTSIDE VBlank on purpose: the PPU ignores the
 * VRAM writes but the CPU work — the thing we measure — is identical. */
u16 r_map_getmeta;   /* mapGetMetaTile(1280, 80)                    */
u16 r_map_getprop;   /* mapGetMetaTilesProp(1280, 80)               */
u16 r_map_camera;    /* mapUpdateCamera(sweep x, 0)                 */
u16 r_map_update;    /* mapUpdate() after a camera move             */
u16 r_map_vblankf;   /* mapVblank() with pending scroll work        */

/* C-port probe for the map migration assessment: a C mapGetMetaTile
 * modelling the full-port architecture — small scalars in C statics
 * (bank 0), bulk state (row LUT at $7E, map data in its ROM bank)
 * through cached far pointers. Correctness pinned against the ASM
 * (same query must yield tile 21, as in libtest). */
u16 r_map_getmeta_c; /* the C model's cycles                        */
u16 r_map_c_val;     /* c_getmetatile(1280,80) -> 21 (correctness)  */

/* sprite_dynamic.asm measurement points (16x16 sheet, engine inited
 * as in the dynamic_sprite example). The flush is the NMI-time path,
 * called here from the main thread: the CPU work measured is the
 * VBlank-budget work. */
u16 r_sd_draw;       /* draw+EndFrame pair, oamrefresh=0 (steady
                      * per-frame cycle — draws NEED EndFrame: the
                      * slot allocator resets there)                 */
u16 r_sd_draw_rf;    /* refresh+draw+EndFrame+flush (full lifecycle)*/
u16 r_sd_flush_idle; /* NmiFlush with empty queue (per-frame floor) */
u16 r_sd_draw_c;     /* C model of the steady 16Draw + ASM EndFrame  */

/* const-path measurement points (chantier A9, 2026-09-07). Every one of
 * these reads ROM through a `const` pointer or a const table: exactly the
 * accesses QBE's optimiser used to pin as if volatile (never forwarded,
 * never promoted out of their alloca). benchrom's other functions are
 * ASM paths and cannot see that class; these are its instrument. Each
 * loop body is a small non-inlined function so the frame stays under
 * 256 bytes (large-frame [tcc__fp],y addressing would dominate). */
u16 r_c_anim_tick;   /* animTick(): lib anim, clip/frames/durations walk  */
u16 r_c_anim_meta;   /* animTickMeta(): + const pointer table indexed     */
u16 r_c_walk8;       /* sum of 32 bytes through `const u8 *p++`           */
u16 r_c_walk16;      /* sum of 32 words through `const u16 *p++`          */
u16 r_c_copy;        /* 32-byte `*dest++ = *src++`, const src (mycopy)    */
u16 r_c_fields;      /* const struct fields through `const Rec *r`        */
u16 r_c_index;       /* tab[i] on a const table (indexed-long fusion)     */
u16 r_c_val;         /* correctness: walk8 sum (528) | walk16 sum & 0xFF  */
u16 r_bench_done;    /* 0xBEEF when every result above is written  */

static volatile u16 vi;   /* opaque loop bound (defeats folding) */
static volatile u16 vc;   /* N_ITER/8 for the 32-element const walks   */

/* --- C-port probe: far-access mapGetMetaTile model --- */
extern u16 mapadrrowlut[];        /* $7E RAMSECTION (map.asm) */
extern u8 mapdata[], tilesetdef[], tilesetatt[];
static const u16 *lutp;           /* far pointer, cached once  */
static const u8 *mapp;

extern void oamDynamicNmiFlush(void);          /* rtl ASM, jsl-able */
extern void oamInitDynamicSpriteEndFrame(void);
extern u8 spr16_tiles[];

/* --- C-port probe: steady oamDynamic16Draw model ---
 * Real engine state via externs (all bank-0 WRAM: the module's DB=$7E
 * trick reads the SAME memory through the WRAM mirror). The tiny
 * mask/size LUTs are computed arithmetically; the tile-number LUT is
 * modelled as a RAM static, as a full port would (mode7 precedent). */
extern u16 oamnumberperframe, oamnumberspr0, oamnumberspr1;
extern u16 spr16addrgfx, spr0addrgfx, spr1addrgfx;
static u16 c_lkup16idT[64];

static void c_draw16_steady(u16 id) {
    u16 x = oamnumberperframe;
    u16 slot;
    u8 sh;
    u16 hy;

    if (spr16addrgfx == spr0addrgfx) {
        slot = oamnumberspr0;
        oamnumberspr0 = (u16)(slot + 1);
    } else {
        slot = oamnumberspr1;
        oamnumberspr1 = (u16)(slot + 1);
    }
    oamMemory[x + 2] = (u8)c_lkup16idT[slot & 63];
    oamMemory[x] = (u8)oambuffer[id].oamx;
    oamMemory[x + 1] = (u8)oambuffer[id].oamy;
    oamMemory[x + 3] = oambuffer[id].oamattribute;

    hy = (u16)(512 + (x >> 4));
    sh = (u8)((x >> 1) & 6);
    if ((u16)oambuffer[id].oamx & 0x100) {
        oamMemory[hy] |= (u8)(1 << sh);
    } else {
        oamMemory[hy] &= (u8)~(u8)(1 << sh);
    }
    if (spr16addrgfx != spr1addrgfx) {
        oamMemory[hy] |= (u8)(2 << sh);
    } else {
        oamMemory[hy] &= (u8)~(u8)(2 << sh);
    }
    oamnumberperframe = (u16)(x + 4);
}

static u16 c_getmetatile(u16 x, u16 y) {
    u16 off = lutp[((y >> 2) & 0xFFFE) >> 1] + ((x >> 2) & 0xFFFE);
    return *(const u16 *)(mapp + off) & 0x03FF;
}

/* --- const-path workloads (A9 instrument) --- */
typedef struct { u8 a; u8 b; u16 w; u32 l; } CRec;
static const u8  c_tab8[32]  = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
                                 17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32 };
static const u16 c_tab16[32] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
                                 17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32 };
static const CRec c_recs[8] = {
    {1,2,300,40000UL},{3,4,500,60000UL},{5,6,700,80000UL},{7,8,900,100000UL},
    {9,10,1100,120000UL},{11,12,1300,140000UL},{13,14,1500,160000UL},{15,16,1700,180000UL} };
static const u16 c_frames[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
static const u8  c_durs[8]   = { 1, 2, 1, 3, 1, 2, 1, 1 };
static const AnimClip c_clip = { c_frames, c_durs, 8, 1, ANIM_LOOP, 0 };
static const u16 *const c_meta[8] = { c_tab16, c_tab16+1, c_tab16+2, c_tab16+3,
                                      c_tab16+4, c_tab16+5, c_tab16+6, c_tab16+7 };
static AnimPlayer c_player = ANIM_PLAYER_INIT;
static u8 c_dst[32];
static volatile u8 c_n;          /* opaque inner bound (32) */

u16 c_walk8(void)  { const u8 *p = c_tab8;  u16 s = 0; u8 k, n = c_n; for (k = 0; k < n; k++) s += *p++; return s; }
u16 c_walk16(void) { const u16 *p = c_tab16; u16 s = 0; u8 k, n = c_n; for (k = 0; k < n; k++) s += *p++; return s; }
void c_copy(void)  { u8 *d = c_dst; const u8 *p = c_tab8; u8 k, n = c_n; for (k = 0; k < n; k++) *d++ = *p++; }
u16 c_fields(u8 i) { const CRec *r = &c_recs[i & 7]; return (u16)(r->a + r->b + r->w + (u16)r->l); }
u16 c_index(u8 i)  { return (u16)(c_tab8[i & 31] + c_tab16[i & 31]); }

/* Frame bracket helpers */
static u16 t0;
static void bench_begin(void) {
    u16 f = frame_count;
    while (frame_count == f) { }      /* align to a frame edge */
    t0 = frame_count;
}
static u16 bench_end(void) {
    return (u16)(frame_count - t0);
}

int main(void) {
    u16 i;
    u8 a = 0;
    s16 x = 12;

    consoleInit();
    mode7Init();
    setScreenOn();

    vi = N_ITER;

    /* calibration — identical loop shape, empty body */
    bench_begin();
    for (i = 0; i < vi; i++) { }
    r_cal_empty = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        mode7SetAngle(a);
        a++;
    }
    r_m7_setangle = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        mode7SetScale(0x0100, 0x0100);
    }
    r_m7_setscale = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        mode7SetCenter(x, (s16)(x + 3));
    }
    r_m7_setcenter = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        mode7SetMatrix(256, 0, 0, 256);
    }
    r_m7_setmatrix = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        mode7Transform(i & 511, 100);
    }
    r_m7_transform = bench_end();

    /* --- map module (real data) --- */
    mapLoad(mapdata, tilesetdef, tilesetatt);
    lutp = mapadrrowlut;          /* far pointers for the C model */
    mapp = mapdata + 8;           /* +8: past the m16 header, as mapLoad */

    bench_begin();
    for (i = 0; i < vi; i++) {
        mapGetMetaTile(1280, 80);
    }
    r_map_getmeta = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        mapGetMetaTilesProp(1280, 80);
    }
    r_map_getprop = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        mapUpdateCamera(i & 511, 0);   /* pans back and forth: streams */
    }
    r_map_camera = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        mapUpdateCamera(i & 511, 0);
        mapUpdate();
    }
    r_map_update = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        mapUpdateCamera(i & 511, 0);
        mapUpdate();
        mapVblank();
    }
    r_map_vblankf = bench_end();

    r_map_c_val = c_getmetatile(1280, 80);
    bench_begin();
    for (i = 0; i < vi; i++) {
        c_getmetatile(1280, 80);
    }
    r_map_getmeta_c = bench_end();

    /* --- sprite_dynamic (16x16 engine, as the dynamic_sprite example) --- */
    {
        static const OamDynamicConfig dyn_cfg = {
            .vramLarge     = 0x0000,
            .vramSmall     = 0x1000,
            .slotLargeInit = 0,
            .slotSmallInit = 0,
            .sizeMode      = OBJ_SIZE8_L16,
        };
        oamDynamicInit(&dyn_cfg);
    }
    oambuffer[0].oamx = 100;
    oambuffer[0].oamy = 100;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamattribute = OBJ_PRIO(3);
    oambuffer[0].oamrefresh = 1;
    OAM_SET_GFX(0, spr16_tiles);
    oamDynamicDraw(0);
    oamInitDynamicSpriteEndFrame();
    oamDynamicNmiFlush();          /* settle: first upload drained */

    bench_begin();
    for (i = 0; i < vi; i++) {
        oamDynamicDraw(0);         /* oamrefresh stays 0 */
        oamInitDynamicSpriteEndFrame();
    }
    r_sd_draw = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        oambuffer[0].oamrefresh = 1;
        oamDynamicDraw(0);         /* queues the tile upload */
        oamInitDynamicSpriteEndFrame();
        oamDynamicNmiFlush();      /* drains it — full refresh lifecycle */
    }
    r_sd_draw_rf = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        oamDynamicNmiFlush();      /* empty queue: the per-frame floor */
    }
    r_sd_flush_idle = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        c_draw16_steady(0);        /* C model + real ASM EndFrame */
        oamInitDynamicSpriteEndFrame();
    }
    r_sd_draw_c = bench_end();

    /* --- const paths (A9 instrument) --- */
    c_n = 32;
    vc = N_ITER / 8;
    animPlay(&c_player, &c_clip);
    bench_begin();
    for (i = 0; i < vi; i++) {
        animTick(&c_player);
    }
    r_c_anim_tick = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        c_dst[0] = (u8)*animTickMeta(&c_player, c_meta);
    }
    r_c_anim_meta = bench_end();

    bench_begin();
    for (i = 0; i < vc; i++) {
        c_walk8();
    }
    r_c_walk8 = bench_end();

    bench_begin();
    for (i = 0; i < vc; i++) {
        c_walk16();
    }
    r_c_walk16 = bench_end();

    bench_begin();
    for (i = 0; i < vc; i++) {
        c_copy();
    }
    r_c_copy = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        c_fields((u8)i);
    }
    r_c_fields = bench_end();

    bench_begin();
    for (i = 0; i < vi; i++) {
        c_index((u8)i);
    }
    r_c_index = bench_end();

    r_c_val = (u16)(c_walk8() | (c_walk16() & 0xFF));   /* 528 | 16 = 528 */

    r_bench_done = 0xBEEF;

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
