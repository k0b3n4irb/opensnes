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
