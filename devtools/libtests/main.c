/*
 * Library runtime-assertion fixture — exercises lib/source functions with
 * known vectors and stores the results in WRAM globals, which the harness
 * (test_libtest.py, via `luna state --assert`) checks against expected
 * values. Static checks and the visual-regression hash can't prove a
 * return value or a bounds guard; only an execution check can.
 *
 * Current coverage:
 *   - math: div16/mod16 (bounded long division), mul16, sqrt16
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

/* --- math vectors --- */
u16 r_div_a;    /* div16(100, 7)    -> 14 */
u16 r_mod_a;    /* mod16(100, 7)    -> 2 */
u16 r_div_max;  /* div16(65535, 1)  -> 65535 (worst case of the old O(quotient) loop) */
u16 r_div_zero; /* div16(42, 0)     -> 0 (documented contract) */
u16 r_mod_zero; /* mod16(42, 0)     -> 0 (documented contract) */
u16 r_mul;      /* mul16(123, 45)   -> 5535 */
u16 r_sqrt;     /* sqrt16(144)      -> 12 */

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
 * with examples/maps/mapscroll), pinned in bank 2 (B1 path).
 * Expected values host-parsed from the committed blobs:
 * entry(1280,80) = tile 21, b16[21] = 0xFF00 (T_SOLID); (0,0) -> 0. */
extern u8 mapdata[];     /* BG1.m16        (data.asm, bank 2) */
extern u8 tilesetdef[];  /* tiledMario.t16 */
extern u8 tilesetatt[];  /* tiledMario.b16 */
u16 r_map_tile;   /* mapGetMetaTile(1280,80)      -> 21     */
u16 r_map_prop;   /* mapGetMetaTilesProp(1280,80) -> 0xFF00 */
u16 r_map_prop0;  /* mapGetMetaTilesProp(0,0)     -> 0      */

u16 r_done;     /* 0xBEEF once every assignment above has executed */

DECLARE_ANIM_CLIP(clip_a, ANIM_LOOP, 2, 10, 20, 30);
DECLARE_ANIM_CLIP(clip_b, ANIM_LOOP, 1, 77, 88);
DECLARE_ANIM_CLIP(clip_once, ANIM_ONCE, 1, 5, 6);

int main(void) {
    u8 i;
    AnimPlayer ap = ANIM_PLAYER_INIT;
    RmwProbe rmw;

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

    r_done      = 0xBEEF;

    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
