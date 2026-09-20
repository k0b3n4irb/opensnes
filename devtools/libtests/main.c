/*
 * Library runtime-assertion fixture — exercises lib/source functions with
 * known vectors and stores the results in WRAM globals, which the harness
 * (test_libtest.py, via `luna state --assert`) checks against expected
 * values. Static checks and the visual-regression hash can't prove a
 * return value or a bounds guard; only an execution check can.
 *
 * Current coverage:
 *   - math: div16/mod16 (bounded long division), mul16, sqrt16,
 *     fixMul/fixDiv/fixLerp (8.8) and fix32Mul/fix32Div (16.16) — L2b
 *   - text: cursor_y wrap — printing past row 31 must wrap to row 0
 *     instead of writing past tilemapBuffer[2048] into the RAM sections
 *     that follow it (text_config is the first casualty pre-fix)
 *   - anim: tick sequencing (loop wrap, once-hold + finished flag,
 *     continue-if-same vs switch semantics, stopped -> ANIM_NONE)
 *
 * Globals live in bank $00 WRAM (< $2000), so `--assert 00:<off>=<bytes>`
 * reads them. Values are little-endian.
 */
#include <snes.h>
#include <snes/anim.h>
#include <snes/math.h>
#include <snes/text.h>
#include <snes/audio.h>
#include <snes/fixed32.h>
#include <snes/collision.h>
#include <snes/window.h>
#include <snes/sram.h>
#include <snes/interrupt.h>
#include <snes/input.h>
#include <snes/object.h>
#include <snes/colormath.h>
#include <snes/mosaic.h>
#include <snes/profile.h>
#include <snes/registers.h>

/* --- math vectors --- */
u16 r_div_a;    /* div16(100, 7)    -> 14 */
u16 r_mod_a;    /* mod16(100, 7)    -> 2 */
u16 r_div_max;  /* div16(65535, 1)  -> 65535 (worst case of the old O(quotient) loop) */
u16 r_div_zero; /* div16(42, 0)     -> 0 (documented contract) */
u16 r_mod_zero; /* mod16(42, 0)     -> 0 (documented contract) */
u16 r_mul;      /* mul16(123, 45)   -> 5535 */
u16 r_sqrt;     /* sqrt16(144)      -> 12 */

/* --- fixed-point vectors (gaps review L2b, 2026-09-14): fixMul, fixDiv,
 * fixLerp (8.8) and fix32Mul, fix32Div (16.16) had no runtime assert —
 * the a7 / c_features ROMs prove the compiler's arithmetic, these prove
 * the lib's. Signs, fractions and the documented zero-divisor contract. */
u16 r_fmul_a;    /* fixMul(FIX(2), 128)         -> 1.0  = 0x0100 */
u16 r_fmul_neg;  /* fixMul(FIX(-3), FIX(2))     -> -6.0 = 0xFA00 */
u16 r_fmul_frac; /* fixMul(1.5, 1.5)            -> 2.25 = 0x0240 */
u16 r_fmul_nn;   /* fixMul(FIX(-1), FIX(-1))    -> 1.0  = 0x0100 */
u16 r_fdiv_a;    /* fixDiv(FIX(100), FIX(5))    -> 20.0 = 5120 */
u16 r_fdiv_frac; /* fixDiv(FIX(1), FIX(4))      -> 0.25 = 64 */
u16 r_fdiv_neg;  /* fixDiv(FIX(-6), FIX(2))     -> -3.0 = 0xFD00 */
u16 r_fdiv_zero; /* fixDiv(FIX(7), 0)           -> 0 (documented contract) */
u16 r_lerp_mid;  /* fixLerp(FIX(0), FIX(100), 128) -> 50.0 = 12800 */
u16 r_lerp_t0;   /* fixLerp(FIX(10), FIX(20), 0)   -> 10.0 = 2560 */
u16 r_lerp_down; /* fixLerp(FIX(20), FIX(10), 128) -> 15.0 = 3840 (negative delta) */
u16 r_lerp_t255; /* fixLerp(FIX(0), FIX(100), 255) -> 25600*255/256 = 25500 */
u32 r_f32mul;    /* fix32Mul(FIX32(3), FIX32(2))   -> 6.0  = 0x00060000 */
u32 r_f32mul_n;  /* fix32Mul(FIX32(-3), FIX32(2))  -> -6.0 = 0xFFFA0000 */
u32 r_f32mul_f;  /* fix32Mul(1.5, 1.5)             -> 2.25 = 0x00024000 */
u32 r_f32div;    /* fix32Div(FIX32(6), FIX32(2))   -> 3.0  = 0x00030000 */
u32 r_f32div_n;  /* fix32Div(FIX32(-6), FIX32(2))  -> -3.0 = 0xFFFD0000 */
u32 r_f32div_f;  /* fix32Div(FIX32(1), FIX32(4))   -> 0.25 = 0x00004000 */
u32 r_f32div_r;  /* fix32Div(FIX32(1), FIX32(3))   -> 0x00005555 (65536/3 truncated) */
static volatile fixed fx_half = 128, fx_15 = 0x0180;
static volatile fixed32 f32_15 = 0x00018000l;

/* --- NMI-context math vectors (#113) --- */
/* The C operators below run inside an nmiSet callback, where the
 * hardware mul/div unit reads garbage (auto-joypad window) — the
 * runtime must take its software path (in_nmi_ctx gate). Operands are
 * volatile so cproc can't constant-fold the ops away. Pre-fix, r_nmi_mul
 * read 0. */
/* regression pin for opensnes#114: an explicit (s16) cast of an
 * unsigned-derived operand must produce a SIGNED division. Pre-fix the
 * stale look-through-casts heuristic emitted __div16 (unsigned):
 * -30000/49 read 725 instead of -612. */
u16 r_sdiv_cast; /* (s16)-30000 / (s16)(op|1), op=49 -> -612 = 0xFD9C */

u16 r_nmi_mul;  /* 123 * 673 in callback -> 82779 & 0xFFFF = 17243 */
u16 r_nmi_div;  /* 33000 / 7 in callback -> 4714 (8-bit-divisor path) */
u16 r_nmi_mod;  /* 33000 % 7 in callback -> 2 */
static volatile u16 sdiv_op = 49;
static volatile u16 nmi_op_a = 123, nmi_op_b = 673;
static volatile u16 nmi_op_c = 33000, nmi_op_d = 7;
static volatile u8 nmi_math_done;

static void nmiMathProbe(void) {
    if (nmi_math_done) return;
    r_nmi_mul = nmi_op_a * nmi_op_b;
    r_nmi_div = nmi_op_c / nmi_op_d;
    r_nmi_mod = nmi_op_c % nmi_op_d;
    nmi_math_done = 1;
}

/* --- text overflow sentinels --- */
u8 s_map_width; /* text_config.map_width after 40 printed rows -> 32.
                 * Pre-fix, rows 32+ wrote past tilemapBuffer into whatever
                 * RAM section the linker placed next (layout-dependent). */
u8 s_cursor_y;  /* textGetY() after 40 newlines -> 8 (40 wraps to 40-32).
                 * The deterministic sentinel: pre-fix this read 40. */

/* --- anim vectors (see clip definitions in main) --- */
u16 r_anim_loop;    /* LOOP {10,20,30} speed 2, 7 ticks -> wrapped back to 10 */
u16 r_anim_once;    /* ONCE {5,6} speed 1, 5 ticks -> holds 6 */
u16 r_anim_done;    /* animDone after the above -> 1 */
u16 r_anim_switch;  /* play A, tick, play B -> B's frame 0 = 77 */
u16 r_anim_cont;    /* play A, tick x3 (frame 1), play A again (no-op),
                     * tick -> still frame 1 value 20 (continue-if-same) */
u16 r_anim_stop;    /* tick on a zero-init player -> ANIM_NONE (0xFFFF) */

/* --- regression pin for opensnes#99 (FIXED): u8 RMW through a pointer,
 * then re-read --- `p->f--; if (p->f == 0)` used to miscompile: the
 * post-store re-read reloaded the address in 8-bit accumulator mode
 * (stale high byte). Minimal pin: decrement 3 from 2 -> the ==0 branch
 * must be taken exactly once. A normal PASS vector since the qbe fix.
 * Faithful skeleton of animTick's shape: indexed deref through a struct
 * pointer field, an 8-bit flag test, then the u8 RMW + re-read.
 * CRITICAL trigger: the probe struct must live on the STACK ($01xx) —
 * the bad reload only corrupted the address high byte, so a zero-page
 * static ($00xx) masked the bug. */
typedef struct { const u16 *tab; u8 idx; u8 cnt; u8 fl; u8 rsv; } RmwProbe;
static const u16 rmw_tab[3] = { 100, 200, 300 };
static u16 rmw_step(RmwProbe *q) {
    u16 out = q->tab[q->idx];
    if (q->fl & 1) return out;
    q->cnt--;
    if (q->cnt == 0) {
        q->idx = q->idx + 1;
        q->cnt = 2;
    }
    return out;
}
u16 r_rmw_u8;  /* 3 steps from {idx0,cnt2}: 100,100,200 -> expected 200 */

/* --- map module: #103 regression pin — the collision getters called
 * FROM C. metatilesprop/mapadrrowlut/maptile_L1* live at $7E:3000+
 * (above the WRAM mirror); the getters used to read them with the
 * caller's DB ($00 from C) -> open bus. Real tmx2snes data (shared
 * with examples/maps/map_scroll), pinned in bank 2 (B1 path).
 * Expected values host-parsed from the committed blobs:
 * entry(1280,80) = tile 21, b16[21] = 0xFF00 (T_SOLID); (0,0) -> 0. */
extern u8 mapdata[];     /* BG1.m16        (data.asm, bank 2) */
extern u8 tilesetdef[];  /* tiledMario.t16 */
extern u8 tilesetatt[];  /* tiledMario.b16 */
u16 r_map_tile;   /* mapGetMetaTile(1280,80)      -> 21     */
u16 r_map_prop;   /* mapGetMetaTilesProp(1280,80) -> 0xFF00 */
u16 r_map_prop0;  /* mapGetMetaTilesProp(0,0)     -> 0      */

/* --- audio v2 (phase 1): driver boot + command round-trips.
 * r_audio_ready proves the whole chain: IPL boot, driver upload,
 * execute, PING handshake (seq-bit command + echo-ack). The setters
 * after it are WRAM-silent — their DSP effect is asserted by the
 * spc-dump probe (probes/audio_v2.py), not here. */
u16 r_audio_ready;  /* audioIsReady() after audioInit() -> 1   */
u16 r_audio_vol;    /* audioGetVolume() after SetVolume(100) -> 100 */

/* audio v2 phase 2: the sample pipeline end-to-end. The 9-byte beep
 * (data.asm) is streamed into ARAM via LOAD_SIZE/LOAD/DIR_SET, then
 * keyed on. ARAM/DSP side asserted by probes/audio_v2.py. */
extern u8 beep_brr[];
u16 r_audio_load;   /* audioLoadSample(0, beep, 9, 0) -> AUDIO_OK (0)  */
u16 r_audio_free;   /* audioGetFreeMemory() -> 0xC000-0x0B00-9 = 0xB4F7 */
u16 r_audio_addr;   /* AudioSample.spcAddress of slot 0 -> 0x0B00       */
u16 r_audio_voice;  /* audioPlaySampleEx(...) -> voice 0 (round-robin)  */

/* audio v2 phase 3: echo config + live voice-state readback. Echo DSP
 * registers (ESA/EDL/EFB/EVOL/FIR0/EON) asserted by the spc-dump
 * probe; here we assert the one DSP->CPU read path. */
u16 r_audio_active; /* GetVoiceState(0).active while the beep loops -> 1 */

/* --- L2c (gaps review, 2026-09-15): the lib surface no example calls.
 * collision.h (8/10 never executed), sram.h, interrupt.h's IRQ path and
 * the console region/vblank getters get execution asserts here; the
 * window module's effect is asserted on luna's PPU register view
 * (test_libtest.py reads `luna state` JSON: windows, w12sel, ...). */

/* collision: pure functions, known geometry */
u16 r_col_rect;     /* collideRect({10,10,16,16},{20,20,16,16})  -> 1 */
u16 r_col_rect_no;  /* collideRect(a,{30,10,8,8}) (a.right=26<=30) -> 0 */
u16 r_col_pt_in;    /* collidePoint(15,15,&a)                    -> 1 */
u16 r_col_pt_edge;  /* collidePoint(26,15,&a) (x == right)       -> 0 */
u16 r_col_ex;       /* collideRectEx(a,b,&ox,&oy)                -> 1 */
u16 r_col_ex_ox;    /* ox: exit-right 6 < exit-left 26 -> -6      = 0xFFFA */
u16 r_col_ex_oy;    /* oy: same on Y                              = 0xFFFA */
u16 r_col_ex_no;    /* collideRectEx(a,c,...) -> 0, ox = oy = 0 */
u16 r_col_ex_no_ox;
u16 r_col_tile;     /* collideTile(8,8,map4x4,4)   -> map[1][1] = 1 */
u16 r_col_tile0;    /* collideTile(0,0,map,4)      -> 0 */
u16 r_col_tile_neg; /* collideTile(-1,5,map,4)     -> 1 (off-map is solid) */
u16 r_col_tile_far; /* collideTile(40,0,map,4)     -> 1 (tileX 5 >= width) */
u16 r_col_tile16;   /* collideTileEx(16,16,map,4,16) -> map[1][1] = 1 */
u16 r_col_tile8x;   /* collideTileEx(24,16,map,4,8)  -> map[2][3] = 2 */
u16 r_col_rtile0;   /* collideRectTile({0,0,8,8})   -> corners all 0 -> 0 */
u16 r_col_rtile1;   /* collideRectTile({4,4,8,8})   -> (11,11) hits [1][1] -> 1 */
u16 r_rect_x, r_rect_y, r_rect_w, r_rect_h; /* rectInit(&r,5,6,7,8) */
u16 r_rect_px, r_rect_py;                   /* rectSetPos(&r,-3,9) -> 0xFFFD, 9 */
u16 r_rect_cx, r_rect_cy;                   /* rectGetCenter(&a) -> 18, 18 */
u16 r_rect_in;      /* rectContains({12,12,4,4}, &a) -> 1 */
u16 r_rect_out;     /* rectContains(&a, {12,12,4,4}) -> 0 */
static const u8 col_map[16] = {
    0, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 2,
    3, 0, 0, 0,
};

/* sram: bank $70 round trip (USE_SRAM=1 in the Makefile) */
static u8 save_buf[16];   /* i*3+1 */
u8 load_buf[16];
u8 load_buf2[8];
u16 r_sram_rt;      /* bytes equal after sramSave/sramLoad(16)     -> 16 */
u16 r_sram_off;     /* sramSaveOffset/LoadOffset(8 @ 0x100): [7]  -> 22 */
u16 r_sram_off0;    /*                                       [0]  -> 1 */
u16 r_sram_ck;      /* sramChecksum(save_buf,16) = XOR(1,4,..,46) -> 32 */
u16 r_sram_ck0;     /* sramChecksum(save_buf,0)                   -> 0 */
u16 r_sram_clear;   /* OR of 16 bytes reloaded after sramClear(16) -> 0
                     * (pre-fix 0x0F: the loop stored the offset, not 0) */

/* interrupt: V-timer IRQ counted by the raw handler in data.asm */
volatile u16 irq_count;
u16 r_irq_a;        /* irqSet + VTIMER 100, 10 frames        -> 10 */
u16 r_irq_b;        /* irqDisable, 3 more frames             -> 10 */
u16 r_irq_c;        /* irqSetBank + H|V timer, 2 frames      -> 12 */
u16 r_irq_d;        /* irqClear (default handler), 2 frames  -> 12 */

/* object engine: the two behaviours fixed on 2026-09-18.
 *
 *  - The workspace write-back. An update callback that looks at another
 *    object leaves THAT object in the single workspace; the engine then
 *    copied it over the slot being updated, so the enemy silently became a
 *    copy of what it looked at. The workspace is owner-tracked now: the
 *    edit made before the peek must survive (r_obj_edit), the peeking
 *    object must keep its own type (r_obj_type), and the object peeked at
 *    must be untouched (r_obj_other).
 *  - The null-callback guard. Type 1 below is never registered. Updating an
 *    object of that type used to dispatch to $00:0000; reaching r_obj_alive
 *    at all is the assert.
 */
u16 r_obj_type;     /* peeker's type after the update   -> 0 (not 1)      */
u16 r_obj_edit;     /* peeker's yvel, set before the peek -> 0x1234       */
u16 r_obj_other;    /* the other object's yvel            -> 0x0BAD kept  */
u16 r_obj_fr_off;   /* objCollidMap1D, friction off: xvel  -> 0x0300 kept  */
u16 r_obj_fr_x;     /* friction 0x0100: xvel 0x0300       -> 0x0200       */
u16 r_obj_fr_y;     /* friction 0x0100: yvel -0x0080      -> 0 (clamped)  */
u16 r_obj_pool;     /* objNew successes after objKillAll  -> 80 (was 79)  */
u16 r_obj_calls;    /* update callback invocations        -> 1            */
u16 r_obj_alive;    /* set after objUpdateAll returns     -> 0xA11E       */
static u16 obj_peeker, obj_other;
static u16 obj_calls;

static void objPeekUpdate(u16 idx) {
    obj_calls++;
    objWorkspace.yvel = 0x1234;        /* an edit made BEFORE the peek */
    objGetPointer(obj_other);          /* workspace now holds the other one */
    /* ...and we return without restoring it: the trap. */
}

/* --- coverage lot B (2026-09-19): public functions nothing executed. Every
 * vector asserts the function's EFFECT — a call with no observable result
 * is not a test. PPU-side effects (colormath, video, mosaic, mode7, the
 * DMA variants, oamSetTile) are asserted from luna's PPU view in
 * test_libtest.py; the rest lands in these globals. The hdma module is not
 * linked here: its wave tables take 1346 bytes of bank-$00 RAM and this
 * fixture has 1266 free — its helpers get their own fixture (lot C), which
 * also holds nmiSet. */
u16 r_fix_abs_n;    /* fixAbs(FIX(-3))                          -> 0x0300 */
u16 r_fix_abs_p;    /* fixAbs(FIX(2))                           -> 0x0200 */
u16 r_fix_clamp_lo; /* fixClamp(FIX(-9), FIX(-1), FIX(1))       -> 0xFF00 */
u16 r_fix_clamp_hi; /* fixClamp(FIX(9),  FIX(-1), FIX(1))       -> 0x0100 */
u16 r_fix_clamp_in; /* fixClamp(fx_half, FIX(-1), FIX(1))       -> 0x0080 */
u16 r_fix_sqrt;     /* fixSqrt(FIX(16))                         -> 0x0400 */
u16 r_bg_sx;        /* bgSetScrollX(1, 300); bgGetScrollX(1)    -> 300 */
u16 r_bg_sy;        /* bgSetScrollY(1, 77);  bgGetScrollY(1)    -> 77 */
u16 r_bg_init;      /* bgInit(2) after bgSetScrollX(2, 5)       -> 0 */
u16 r_text_x;       /* textGetX after "AB" on a fresh line       -> 2 */
u16 r_text_flush;   /* tilemap_update_flag right after textFlush -> 1 */
u16 r_frame_reset;  /* frame_count right after resetFrameCount   -> 0 */
u16 r_pad_raw;      /* padRaw(0) idle | padRaw(7) out of range   -> 0 */
u16 r_mouse;        /* no mouse: connected|x|y|held|pressed      -> 0 */
u16 r_mouse_sens;   /* mouseSetSensitivity(0, HIGH) is deferred to the NMI, which
                     * only talks to a mouse that is there: the getter stays 0 and
                     * the request byte (mouseRequestChangeSensitivity[0], asserted
                     * by symbol) reads 0x82 */
u16 r_scope;        /* no scope: held|down|pressed|x|y|rawx|rawy -> 0 */
u16 r_scope_delay;  /* scopeSetRepeatDelay(7): scope_repdelay    -> 7 */
u16 r_obj_grav;     /* objInitGravity(0x40,0); objCollidMap in the air: yvel -> 0x40 */
u16 r_obj_refresh;  /* objRefreshAll: the refresh callback ran   -> 1 */
u16 r_obj_cobj;     /* objCollidObj, two 8x8 objects 4 px apart  -> 1 */
u16 r_obj_cobj_no;  /* objCollidObj, 40 px apart                 -> 0
                     * (slot INDEXES, not handles: the routine shifts its
                     * arguments by 64 with no mask, so a handle's id byte
                     * lands in the offset — header fixed 2026-09-20) */
u16 r_prof_frames;  /* profileGetFrameCount == frame_count       -> 1 */
u16 r_prof_scan;    /* profileGetScanline() < 262                -> 1 */
u16 r_prof_lines;   /* profileScanlineEnd after a 200-iteration spin -> ge 1 */
u16 r_prof_lag;     /* profileGetLagFrames: reads the counter (value measured) */
u16 r_mosaic;       /* mosaicSetSize(20) clamps: mosaicGetSize   -> 15 */
static const u8 lotb_vram[32] = {
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x10,
    0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x01, 0x02,
};
static const u8 lotb_pal[4]  = { 0x1F, 0x00, 0xE0, 0x03 };   /* colours 250, 251: red, green */
static const u8 lotb_pal2[4] = { 0x00, 0x7C, 0xFF, 0x7F };   /* colours 254, 255: blue, white */
static u16 obj_refresh_calls;
static void objRefreshProbe(u16 idx) { (void)idx; obj_refresh_calls++; }
extern volatile u8 tilemap_update_flag;
extern volatile u16 frame_count;
extern u16 scope_repdelay;

/* fixed32: the C body that fixed32.h says it cannot use. The header and
 * lib/source/math.c both claim a qbe bug makes
 * `(u32)(s32)fixSin(angle) << 8` produce 0x00FF0000 instead of 0xFFFF0000
 * for sin(270 deg) = -1.0, and that this is why fix32Sin lives in asm.
 * These two vectors compute it both ways and compare. */
u32 r_f32sin_asm;   /* fix32Sin(192)                        -> 0xFFFF0000 */
u32 r_f32sin_c;     /* (u32)(s32)fixSin(192) << 8, in C     -> must agree */
static volatile u8 sin_angle = 192;   /* 270 degrees */

/* input: a connected pad reads as connected even with nothing pressed.
 * padIsConnected() used to reject $0000 as well as $FFFF, so an idle pad —
 * the state a pad is in almost every frame — reported unplugged. fullsnes:
 * the auto-joypad word's low nibble is the device signature and a standard
 * joypad's is 0, so an idle pad reads exactly $0000. luna attaches a pad to
 * port 1 by default and the fixture presses nothing, which is precisely the
 * case that was broken. */
u16 r_pad_conn;     /* padIsConnected(0) with no input -> TRUE (0xFF) */
u16 r_pad_idle;     /* padHeld(0) with no input        -> 0 */
u16 r_pad_conn4;    /* padIsConnected(4) — multitap slot, never read -> FALSE */
u16 r_pad_oob;      /* padIsConnected(9) — out of range -> FALSE */

/* console: region + vblank flag */
u16 r_region;       /* getRegion() -> 0 NTSC (1 under --force-region pal) */
u16 r_ispal;        /* isPAL()     -> FALSE 0 (TRUE = 0xFF under pal) */
u16 r_invb_in;      /* isInVBlank() right after WaitForVBlank -> TRUE (0xFF) */
u16 r_invb_out;     /* after spinning until the flag clears   -> 0 */

extern void irqTestHandler(void);   /* data.asm, bank 0 */

u16 r_done;     /* 0xBEEF once every assignment above has executed */

DECLARE_ANIM_CLIP(clip_a, ANIM_LOOP, 2, 10, 20, 30);
DECLARE_ANIM_CLIP(clip_b, ANIM_LOOP, 1, 77, 88);
DECLARE_ANIM_CLIP(clip_once, ANIM_ONCE, 1, 5, 6);

/* --- coverage lot C (2026-09-20): audio v2 stop/unload, two sprite helpers,
 * consoleInitEx. Runs last: it silences the voice the audio block left
 * playing (audio_v2.toml asserts DSP registers, not liveness). */
u16 r_aud_v0_live;   /* voice 0 before the stop: active              -> 1 */
u16 r_aud_v0_stop;   /* audioStopVoice(0), 6 frames later: active    -> 0 */
u16 r_aud_v1_live;   /* voice 1, started meanwhile, still active     -> 1 */
u16 r_aud_all_stop;  /* audioStopAll(), 6 frames later: voice 1      -> 0 */
u16 r_aud_unload;    /* audioGetSampleInfo(0) after audioUnloadSample -> AUDIO_ERR_NOT_LOADED (3) */
u16 r_aud_unfree;    /* audioGetFreeMemory(): LIFO reclaim gave the 9 bytes back -> 0xB500 */
u16 r_meta_n;        /* oamDrawMetaFlip(10, ...), two items: next free id -> 12 */
static const MetaspriteItem lotc_meta[] = {
    METASPR_ITEM(0, 0, 0, 0),
    METASPR_ITEM(8, 0, 1, 0),
    METASPR_TERM,
};

static void coverage_lot_c(void) {
    AudioVoiceState vs;
    AudioSample smp;
    u8 i;

    audioUpdate();                          /* v2 no-op, kept for source compatibility */
    audioGetVoiceState(0, &vs);
    r_aud_v0_live = vs.active;
    audioPlaySampleEx(0, 100, AUDIO_PAN_CENTER, 0x1000);   /* round-robin: voice 1 */
    audioStopVoice(0);
    for (i = 0; i < 6; i++) WaitForVBlank();
    audioGetVoiceState(0, &vs);
    r_aud_v0_stop = vs.active;
    audioGetVoiceState(1, &vs);
    r_aud_v1_live = vs.active;
    audioStopAll();
    for (i = 0; i < 6; i++) WaitForVBlank();
    audioGetVoiceState(1, &vs);
    r_aud_all_stop = vs.active;
    audioUnloadSample(0);
    r_aud_unload = audioGetSampleInfo(0, &smp);
    r_aud_unfree = audioGetFreeMemory();

    /* sprites: a two-item metasprite mirrored in a 16-px-wide box lands its
     * items swapped (dx = 16 - dx - 8) with the flip bit set; asserted on
     * luna's OAM view. oamDynamicSetSize writes the per-sprite size table. */
    r_meta_n = oamDrawMetaFlip(10, 100, 50, lotc_meta, 0, 0, 0, 1, 0, 16, 8);
    oamDynamicSetSize(0, 16);
    WaitForVBlank();
}

/* Lot B runs in its own function: main()'s frame already puts the stack
 * ~920 bytes deep during the audio driver's boot, and growing it with more
 * temporaries pushed the stack into the result globals (found by
 * --trace-writes on r_region: NmiHandler and cmd_send were the writers). */
static void coverage_lot_b(void) {
    u8 i;

    r_fix_abs_n    = (u16)fixAbs(FIX(-3));
    r_fix_abs_p    = (u16)fixAbs(FIX(2));
    r_fix_clamp_lo = (u16)fixClamp(FIX(-9), FIX(-1), FIX(1));
    r_fix_clamp_hi = (u16)fixClamp(FIX(9), FIX(-1), FIX(1));
    r_fix_clamp_in = (u16)fixClamp(fx_half, FIX(-1), FIX(1));
    r_fix_sqrt     = (u16)fixSqrt(FIX(16));

    bgSetScrollX(1, 300);
    bgSetScrollY(1, 77);
    r_bg_sx = bgGetScrollX(1);
    r_bg_sy = bgGetScrollY(1);
    bgSetScrollX(2, 5);
    bgInit(2);
    r_bg_init = bgGetScrollX(2);
    /* bgInitTileSetData: 32 bytes to VRAM word 0x6000 (byte 0xC000, unused
     * by this fixture), gfx pointer left alone (0xFF); test_libtest.py
     * dumps VRAM and compares. The two DMA bank variants land next to it
     * and in CGRAM 250-251; dmaTransfer, the raw one, in CGRAM 254-255. */
    bgInitTileSetData(0xFF, lotb_vram, 16, 0x6000);
    dmaCopyVramBank(lotb_vram + 16, (u8)((u32)(const void *)lotb_vram >> 16), 0x6008, 16);
    dmaCopyCGramBank(lotb_pal, (u8)((u32)(const void *)lotb_pal >> 16), 250, 4);
    WaitForVBlank();
    REG_CGADD = 254;
    dmaTransfer(1, 0x00, (u8)((u32)(const void *)lotb_pal2 >> 16), (u16)(u32)(const void *)lotb_pal2, 0x22, 4);
    oamSetTile(3, 0x1AB);                   /* OAM byte 14 = 0xAB, byte 15 bit 0 = 1 */
    WaitForVBlank();
    dmaCopyOam(oamMemory, 544);             /* what the NMI does, done by hand */

    textPutChar('\n');
    textPrint("AB");
    r_text_x = textGetX();
    textFlush();
    r_text_flush = tilemap_update_flag;
    resetFrameCount();
    r_frame_reset = frame_count;
    mapSetMapOptions(MAP_OPT_1WAY | MAP_OPT_BG2);   /* mapoptions ($7E) asserted by symbol */
    r_pad_raw = padRaw(0) | padRaw(7);
    r_mouse = mouseIsConnected(0) | (u16)mouseGetX(0) | (u16)mouseGetY(0)
            | mouseButtonsHeld(0) | mouseButtonsPressed(0);
    mouseSetSensitivity(0, MOUSE_SENS_HIGH);
    r_mouse_sens = mouseGetSensitivity(0);
    r_scope = scopeButtonsHeld() | scopeButtonsDown() | scopeButtonsPressed()
            | scopeGetX() | scopeGetY() | scopeGetRawX() | scopeGetRawY();
    scopeSetRepeatDelay(7);
    r_scope_delay = scope_repdelay;
    (void)scopeSinceShot();

    /* object engine: gravity, refresh, object-object collision */
    objInitEngine();
    objInitFunctions(0, 0, 0, objRefreshProbe);
    objInitGravity(0x0040, 0);
    obj_peeker = objNew(0, 16, 16);        /* in the air over the loaded map */
    objGetPointer(obj_peeker);
    objWorkspace.width = 8; objWorkspace.height = 8; objWorkspace.yvel = 0;
    objCollidMap(obj_peeker & 0xFF);
    r_obj_grav = (u16)objWorkspace.yvel;
    obj_other = objNew(0, 20, 20);          /* overlaps the first one */
    objGetPointer(obj_other);
    objWorkspace.width = 8; objWorkspace.height = 8;
    objUpdateAll();                         /* computes onscreen for both */
    obj_refresh_calls = 0;
    objRefreshAll();
    r_obj_refresh = obj_refresh_calls;
    r_obj_cobj = objCollidObj(obj_peeker & 0xFF, obj_other & 0xFF);
    objGetPointer(obj_other);
    objWorkspace.xpos[1] = 60;              /* 40 px to the right: apart */
    objUpdateAll();
    r_obj_cobj_no = objCollidObj(obj_peeker & 0xFF, obj_other & 0xFF);

    /* profile: the frame counter it reads is crt0's; a scanline is < 262 */
    profileInit();
    r_prof_frames = (profileGetFrameCount() == frame_count) ? 1 : 0;
    r_prof_scan   = (profileGetScanline() < 262) ? 1 : 0;
    r_prof_lag    = profileGetLagFrames();
    profileColorStart(2);
    profileScanlineStart();
    for (i = 0; i < 200; i++) { r_prof_lines = (u16)(r_prof_lines + i); if (i == 255) break; }
    r_prof_lines = profileScanlineEnd();
    profileColorEnd();

    /* PPU-side, asserted in test_libtest.py: mosaic, SETINI, colour math.
     * profileInit wrote CGADSUB/COLDATA above; colour math last. (Mode 7 is
     * not linked here: its sine table is 256 bytes of bank-$00 RAM this
     * fixture does not have — see the second fixture, lot C.) */
    mosaicSetSize(20);
    r_mosaic = mosaicGetSize();
    videoSetObjInterlace(1);
    videoSetOverscan(1);
    videoSetPseudoHires(1);
    videoSetPseudoHires(0);
    colorMathTransparency50(COLORMATH_BG1);
    colorMathSetCondition(COLORMATH_INSIDE);
    colorMathSetBrightness(10);
    colorMathSetChannel(COLDATA_BLUE, 20);

}

int main(void) {
    u8 i;
    AnimPlayer ap = ANIM_PLAYER_INIT;
    RmwProbe rmw;

    /* audio v2 first: audioInit blocks on the APU boot + driver upload
     * (the longest single step of the fixture — see STEPS in
     * test_libtest.py). Known DSP vectors for the spc-dump probe:
     * ADSR(15,7,7,8) packs to $FF/$E8 (the pitch_mod bow-stroke pair). */
    audioInit();
    r_audio_ready = audioIsReady();
    audioSetVolume(100);
    r_audio_vol = audioGetVolume();
    audioSetVoiceVolume(2, 80, 40);
    audioSetVoicePitch(3, 0x1234);
    audioSetADSR(1, 15, 7, 7, 8);
    audioSetGain(4, 0x5A);

    /* phase 2: stream the beep into ARAM, then key it on voice 0
     * (round-robin starts there). Probe asserts the ARAM bytes, the
     * directory entry, and the playing voice's DSP state. */
    r_audio_load = audioLoadSample(0, beep_brr, 9, 0);
    r_audio_free = audioGetFreeMemory();
    {
        AudioSample s;
        if (audioGetSampleInfo(0, &s) == AUDIO_OK) {
            r_audio_addr = s.spcAddress;
        }
    }
    r_audio_voice = audioPlaySampleEx(0, 127, AUDIO_PAN_CENTER, 0x1000);

    /* phase 3: hall on voice 0's beep + live envelope readback. The
     * echo values are arbitrary-but-distinct probe vectors. */
    audioSetEcho(3, 40, 20, 20);
    {
        static const s8 fir[8] = { 96, 0, 0, 0, 0, 0, 0, 0 };
        audioSetEchoFilter(fir);
    }
    audioEnableEcho(0x01);
    {
        AudioVoiceState vs;
        audioGetVoiceState(0, &vs);
        r_audio_active = vs.active;
    }

    r_fmul_a    = (u16)fixMul(FIX(2), fx_half);
    r_fmul_neg  = (u16)fixMul(FIX(-3), FIX(2));
    r_fmul_frac = (u16)fixMul(fx_15, fx_15);
    r_fmul_nn   = (u16)fixMul(FIX(-1), FIX(-1));
    r_fdiv_a    = (u16)fixDiv(FIX(100), FIX(5));
    r_fdiv_frac = (u16)fixDiv(FIX(1), FIX(4));
    r_fdiv_neg  = (u16)fixDiv(FIX(-6), FIX(2));
    r_fdiv_zero = (u16)fixDiv(FIX(7), 0);
    r_lerp_mid  = (u16)fixLerp(FIX(0), FIX(100), 128);
    r_lerp_t0   = (u16)fixLerp(FIX(10), FIX(20), 0);
    r_lerp_down = (u16)fixLerp(FIX(20), FIX(10), 128);
    r_lerp_t255 = (u16)fixLerp(FIX(0), FIX(100), 255);
    r_f32mul    = (u32)fix32Mul(FIX32(3), FIX32(2));
    r_f32mul_n  = (u32)fix32Mul(FIX32(-3), FIX32(2));
    r_f32mul_f  = (u32)fix32Mul(f32_15, f32_15);
    r_f32div    = (u32)fix32Div(FIX32(6), FIX32(2));
    r_f32div_n  = (u32)fix32Div(FIX32(-6), FIX32(2));
    r_f32div_f  = (u32)fix32Div(FIX32(1), FIX32(4));
    r_f32div_r  = (u32)fix32Div(FIX32(1), FIX32(3));
    r_div_a    = div16(100, 7);
    r_mod_a    = mod16(100, 7);
    r_div_max  = div16(65535, 1);
    r_div_zero = div16(42, 0);
    r_mod_zero = mod16(42, 0);
    r_mul      = mul16(123, 45);
    r_sqrt     = sqrt16(144);
    r_sdiv_cast = (u16)((s16)-30000 / (s16)(sdiv_op | 1));

    rmw.tab = rmw_tab; rmw.idx = 0; rmw.cnt = 2; rmw.fl = 0; rmw.rsv = 0;
    rmw_step(&rmw);
    rmw_step(&rmw);
    r_rmw_u8 = rmw_step(&rmw);

    /* anim: stopped player returns ANIM_NONE */
    r_anim_stop = animTick(&ap);

    /* LOOP wrap: {10,20,30} speed 2 -> tick sequence
     * 10,10,20,20,30,30,10 — 7 ticks end back on frame 0 (value 10) */
    animPlay(&ap, &clip_a);
    for (i = 0; i < 7; i++) r_anim_loop = animTick(&ap);

    /* continue-if-same: 3 more ticks land on frame 1 (20); re-play of the
     * same clip must NOT reset; the next tick stays on 20 */
    animRestart(&ap);                /* deterministic base */
    animTick(&ap);                   /* 10 (frame 0, tick 1/2) */
    animTick(&ap);                   /* 10 (frame 0, tick 2/2 -> advance) */
    animPlay(&ap, &clip_a);          /* same clip, running: must NOT reset */
    r_anim_cont = animTick(&ap);     /* frame 1 -> 20 */

    /* switch: a different clip resets immediately to its frame 0 */
    animPlay(&ap, &clip_b);
    r_anim_switch = animTick(&ap);   /* 77 */

    /* ONCE: {5,6} speed 1 -> 5 ticks: 5,6 then holds 6 */
    animPlay(&ap, &clip_once);
    for (i = 0; i < 5; i++) r_anim_once = animTick(&ap);
    r_anim_done = animDone(&ap) ? 1 : 0;

    /* map getters from C (issue #103): load the real map, consult it */
    mapLoad(mapdata, tilesetdef, tilesetatt);
    r_map_tile  = mapGetMetaTile(1280, 80);
    r_map_prop  = mapGetMetaTilesProp(1280, 80);
    r_map_prop0 = mapGetMetaTilesProp(0, 0);

    textModeInit();

    /* 40 rows of >= 7 glyphs each: rows 32-39 must wrap to rows 0-7.
     * Pre-fix they wrote at buffer offsets 2048+, i.e. over text_config
     * (glyph 6 of row 32 lands exactly on map_width). */
    for (i = 0; i < 40; i++) {
        textPrint("OVERFLOW");
        textPutChar('\n');
    }

    s_map_width = text_config.map_width;
    s_cursor_y  = textGetY();

    /* NMI-context math (#113): compute once inside the callback, then
     * wait until it ran before declaring the fixture done. */
    nmiSetBank(nmiMathProbe, (u8)((u32)(void *)nmiMathProbe >> 16));
    while (!nmi_math_done) {
        WaitForVBlank();
    }
    nmiClear();

    /* --- L2c: collision --- */
    {
        Rect a, b, c, in, r;
        s16 ox, oy;
        u8 k;
        rectInit(&a, 10, 10, 16, 16);
        rectInit(&b, 20, 20, 16, 16);
        rectInit(&c, 30, 10, 8, 8);
        rectInit(&in, 12, 12, 4, 4);
        r_col_rect    = collideRect(&a, &b);
        r_col_rect_no = collideRect(&a, &c);
        r_col_pt_in   = collidePoint(15, 15, &a);
        r_col_pt_edge = collidePoint(26, 15, &a);
        ox = oy = 99;
        r_col_ex    = collideRectEx(&a, &b, &ox, &oy);
        r_col_ex_ox = (u16)ox;
        r_col_ex_oy = (u16)oy;
        r_col_ex_no    = collideRectEx(&a, &c, &ox, &oy);
        r_col_ex_no_ox = (u16)ox | (u16)oy;
        r_col_tile     = collideTile(8, 8, col_map, 4);
        r_col_tile0    = collideTile(0, 0, col_map, 4);
        r_col_tile_neg = collideTile(-1, 5, col_map, 4);
        r_col_tile_far = collideTile(40, 0, col_map, 4);
        r_col_tile16   = collideTileEx(16, 16, col_map, 4, 16);
        r_col_tile8x   = collideTileEx(24, 16, col_map, 4, 8);
        rectInit(&r, 0, 0, 8, 8);
        r_col_rtile0 = collideRectTile(&r, col_map, 4);
        rectSetPos(&r, 4, 4);
        r_col_rtile1 = collideRectTile(&r, col_map, 4);
        rectInit(&r, 5, 6, 7, 8);
        r_rect_x = (u16)r.x; r_rect_y = (u16)r.y; r_rect_w = r.width; r_rect_h = r.height;
        rectSetPos(&r, -3, 9);
        r_rect_px = (u16)r.x; r_rect_py = (u16)r.y;
        rectGetCenter(&a, &ox, &oy);
        r_rect_cx = (u16)ox; r_rect_cy = (u16)oy;
        r_rect_in  = rectContains(&in, &a);
        r_rect_out = rectContains(&a, &in);

        /* --- L2c: sram --- */
        for (k = 0; k < 16; k++) { save_buf[k] = (u8)(k * 3 + 1); load_buf[k] = 0xAA; }
        sramSave(save_buf, 16);
        sramLoad(load_buf, 16);
        r_sram_rt = 0;
        for (k = 0; k < 16; k++) if (load_buf[k] == save_buf[k]) r_sram_rt++;
        sramSaveOffset(save_buf, 8, 0x100);
        sramLoadOffset(load_buf2, 8, 0x100);
        r_sram_off  = load_buf2[7];
        r_sram_off0 = load_buf2[0];
        r_sram_ck  = sramChecksum(save_buf, 16);
        r_sram_ck0 = sramChecksum(save_buf, 0);
        sramClear(16);
        sramLoad(load_buf, 16);
        r_sram_clear = 0;
        for (k = 0; k < 16; k++) r_sram_clear |= load_buf[k];
    }

    /* --- object engine: owner-tracked workspace + null-callback guard ---
     * Runs after mapLoad() above, so the camera globals the update pass
     * culls against are initialised. */
    objInitEngine();
    objInitFunctions(0, 0, objPeekUpdate, 0);   /* typed: no casts */
    /* type 1 is deliberately never registered */
    obj_peeker = objNew(0, 16, 16);
    objGetPointer(obj_peeker);
    objWorkspace.width = 8; objWorkspace.height = 8;
    obj_other = objNew(1, 32, 16);
    objGetPointer(obj_other);
    objWorkspace.yvel = 0x0BAD;
    objGetPointer(obj_peeker);         /* reloading flushes the edit above */
    objUpdateAll();
    r_obj_alive = 0xA11E;
    r_obj_calls = obj_calls;
    objGetPointer(obj_peeker);
    r_obj_type = objWorkspace.type;
    r_obj_edit = (u16)objWorkspace.yvel;
    objGetPointer(obj_other);
    r_obj_other = (u16)objWorkspace.yvel;

    /* --- objCollidMap1D friction: off by default, opt-in, clamped at 0 --- */
    objGetPointer(obj_peeker);
    objWorkspace.xvel = 0x0300; objWorkspace.yvel = (s16)-0x0080;
    objCollidMap1D(obj_peeker & 0xFF);
    r_obj_fr_off = (u16)objWorkspace.xvel;          /* untouched: 0x0300 */
    objInitFriction1D(0x0100);
    objCollidMap1D(obj_peeker & 0xFF);
    r_obj_fr_x = (u16)objWorkspace.xvel;            /* 0x0200 */
    r_obj_fr_y = (u16)objWorkspace.yvel;            /* -0x80 + 0x100 clamps to 0 */

    /* --- objKillAll must give the whole pool back. Slot 0 (type 0) is
     * killed before slot 1 (type 1), so slot 1 ends up ahead of slot 0 on
     * the free list — the case the old forced head-reset leaked. --- */
    objInitEngine();
    objNew(0, 16, 16);
    objNew(1, 32, 16);
    objKillAll();
    r_obj_pool = 0;
    while (objNew(1, 16, 16) != 0) r_obj_pool++;

    /* --- fixed32: the asm sine against the C expression it replaced --- */
    r_f32sin_asm = (u32)fix32Sin(sin_angle);
    r_f32sin_c   = (u32)((u32)(s32)fixSin(sin_angle) << 8);

    /* --- input: the idle-pad connection test (see the comment above) --- */
    r_pad_conn  = padIsConnected(0);
    r_pad_idle  = padHeld(0);
    r_pad_conn4 = padIsConnected(4);
    r_pad_oob   = padIsConnected(9);

    /* --- L2c: console region + vblank flag --- */
    r_region = getRegion();
    r_ispal  = isPAL();
    WaitForVBlank();
    r_invb_in = isInVBlank();
    while (isInVBlank()) { }
    r_invb_out = isInVBlank();

    /* --- L2c: IRQ path. Enable at the top of VBlank so the V-timer at
     * line 100 fires exactly once per waited frame. --- */
    irq_count = 0;
    irqSet((void *)irqTestHandler);        /* bank-0 handler: plain irqSet */
    irqSetVTimer(100);
    WaitForVBlank();
    irqEnable(IRQ_VTIMER);
    for (i = 0; i < 10; i++) WaitForVBlank();
    r_irq_a = irq_count;
    irqDisable();
    for (i = 0; i < 3; i++) WaitForVBlank();
    r_irq_b = irq_count;
    irqSetBank((void *)irqTestHandler, (u8)((u32)(void *)irqTestHandler >> 16));
    irqSetHTimer(64);
    WaitForVBlank();
    irqEnable(IRQ_HTIMER | IRQ_VTIMER);    /* once per frame at (64, 100) */
    for (i = 0; i < 2; i++) WaitForVBlank();
    r_irq_c = irq_count;
    irqClear();                            /* default handler: ack only */
    for (i = 0; i < 2; i++) WaitForVBlank();
    r_irq_d = irq_count;
    irqDisable();

    coverage_lot_b();
    coverage_lot_c();

    /* --- L2c: window registers, asserted from luna's PPU view. Left in
     * their final state: nothing below touches $2123-$212F. --- */
    windowInit();
    windowDisableAll();
    windowSetPos(WINDOW_1, 40, 200);
    windowSetPos(WINDOW_2, 8, 16);
    windowEnable(WINDOW_1, WINDOW_BG1 | WINDOW_OBJ);   /* w12sel 02, wobjsel 02 */
    windowEnable(WINDOW_2, WINDOW_BG3 | WINDOW_MATH);  /* w34sel 08, wobjsel 82 */
    windowSetInvert(WINDOW_1, WINDOW_BG1, 1);          /* w12sel 03 */
    windowSetLogic(WINDOW_BG2, WINDOW_LOGIC_XOR);      /* wbglog 08 */
    windowSetLogic(WINDOW_OBJ, WINDOW_LOGIC_AND);      /* wobjlog 01 */
    windowSetMainMask(WINDOW_BG1 | WINDOW_OBJ);        /* tmw 11 */
    windowSetSubMask(WINDOW_BG3);                      /* tsw 04 */
    windowDisable(WINDOW_2, WINDOW_MATH);              /* wobjsel 02 */
    windowSplit(100);                                  /* W1 0..99, W2 100..255 */
    windowCentered(WINDOW_2, 64);                      /* W2 96..159 */

    r_done      = 0xBEEF;

    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
