/*
 * B2 Phase-0 micro-benchmark: what does a bank-honouring ("far") data
 * access cost versus today's bank-blind one, per access, on THIS compiler?
 *
 * Method = devtools/benchrom: each measured loop runs N_ITER times, the
 * inner loop walks 32 elements, frames elapsed are parked in WRAM and
 * bench.py converts to ~cycles per access after subtracting the empty
 * calibration loop. Only the ratios matter.
 *
 * The far LOAD path already exists since #121 for const-qualified accesses
 * (runtime pointer -> [tcc__r9]; sym[idx] -> lda.l sym,x), so "far" here
 * is measured by reading through a `const` pointer / a ROM array — the
 * exact codegen a bank-$7E object would get under option (A) or (B) of the
 * plan. Stores have no far path yet; their near cost is recorded as the
 * baseline the future far store is compared against.
 */
#include <snes.h>

#define N_ITER 2000u

u16 r_cal_empty;
u16 r_ld_ptr_near, r_ld_ptr_far;
u16 r_ld_idx_near, r_ld_idx_far;
u16 r_st_ptr_near, r_st_idx_near;
u16 r_ld_ptr16_near, r_ld_ptr16_far;
/* Phase 2: real __far (bank $7E) objects — the B2 codegen itself. */
u16 r_ld_ptr_farram, r_ld_ptr16_farram, r_ld_idx_farram;
u16 r_st_ptr_farram, r_st_idx_farram;
u16 r_sink;
u16 r_bench_done;

static volatile u16 vi;      /* opaque outer bound */
static volatile u8  vk;      /* opaque inner bound (32) */

u8 near_buf[32];
static const u8 rom_buf[32] = {
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
    17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32 };
static const u16 rom_buf16[32] = {
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
    17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32 };
u16 near_buf16[32];

volatile u8  *vp_near;
volatile const u8  *vp_far;
volatile u16 *vp16_near;
volatile const u16 *vp16_far;

FAR u8  far_buf[32];
FAR u16 far_buf16[32];
volatile u8  FAR *vp_farram;
volatile u16 FAR *vp16_farram;

static u16 t0;
static void bench_begin(void) {
    u16 f = frame_count;
    while (frame_count == f) { }
    t0 = frame_count;
}
static u16 bench_end(void) { return (u16)(frame_count - t0); }

/* One non-static function per loop: keeps every frame under 256 bytes so
 * locals use `lda N,s` and not the [tcc__fp],y large-frame path, which would
 * otherwise dominate the numbers (main() with every loop inline did). */
u16 loop_cal(void)        { u16 i, s = 0; u8 k, n; for (i = 0; i < vi; i++) { n = vk; for (k = 0; k < n; k++) { } } return s; }
u16 loop_ld_ptr_near(void){ u16 i, s = 0; u8 k, n; for (i = 0; i < vi; i++) { u8 *p = (u8 *)vp_near; n = vk; for (k = 0; k < n; k++) s += p[k]; } return s; }
u16 loop_ld_ptr_far(void) { u16 i, s = 0; u8 k, n; for (i = 0; i < vi; i++) { const u8 *p = (const u8 *)vp_far; n = vk; for (k = 0; k < n; k++) s += p[k]; } return s; }
u16 loop_ld_idx_near(void){ u16 i, s = 0; u8 k, n; for (i = 0; i < vi; i++) { n = vk; for (k = 0; k < n; k++) s += near_buf[k]; } return s; }
u16 loop_ld_idx_far(void) { u16 i, s = 0; u8 k, n; for (i = 0; i < vi; i++) { n = vk; for (k = 0; k < n; k++) s += rom_buf[k]; } return s; }
u16 loop_ld_ptr16_near(void){ u16 i, s = 0; u8 k, n; for (i = 0; i < vi; i++) { u16 *p = (u16 *)vp16_near; n = vk; for (k = 0; k < n; k++) s += p[k]; } return s; }
u16 loop_ld_ptr16_far(void) { u16 i, s = 0; u8 k, n; for (i = 0; i < vi; i++) { const u16 *p = (const u16 *)vp16_far; n = vk; for (k = 0; k < n; k++) s += p[k]; } return s; }
u16 loop_st_ptr_near(void){ u16 i; u8 k, n; for (i = 0; i < vi; i++) { u8 *p = (u8 *)vp_near; n = vk; for (k = 0; k < n; k++) p[k] = k; } return 0; }
u16 loop_st_idx_near(void){ u16 i; u8 k, n; for (i = 0; i < vi; i++) { n = vk; for (k = 0; k < n; k++) near_buf[k] = k; } return 0; }
u16 loop_ld_ptr_farram(void)  { u16 i, s = 0; u8 k, n; for (i = 0; i < vi; i++) { u8 FAR *p = (u8 FAR *)vp_farram; n = vk; for (k = 0; k < n; k++) s += p[k]; } return s; }
u16 loop_ld_ptr16_farram(void){ u16 i, s = 0; u8 k, n; for (i = 0; i < vi; i++) { u16 FAR *p = (u16 FAR *)vp16_farram; n = vk; for (k = 0; k < n; k++) s += p[k]; } return s; }
u16 loop_ld_idx_farram(void)  { u16 i, s = 0; u8 k, n; for (i = 0; i < vi; i++) { n = vk; for (k = 0; k < n; k++) s += far_buf[k]; } return s; }
u16 loop_st_ptr_farram(void)  { u16 i; u8 k, n; for (i = 0; i < vi; i++) { u8 FAR *p = (u8 FAR *)vp_farram; n = vk; for (k = 0; k < n; k++) p[k] = k; } return 0; }
u16 loop_st_idx_farram(void)  { u16 i; u8 k, n; for (i = 0; i < vi; i++) { n = vk; for (k = 0; k < n; k++) far_buf[k] = k; } return 0; }

#define BENCH(res, fn) do { bench_begin(); s += fn(); res = bench_end(); } while (0)

int main(void) {
    u16 s = 0;

    consoleInit();
    setScreenOn();
    vi = N_ITER;
    vk = 32;
    vp_near = near_buf;      vp_far = rom_buf;
    vp16_near = near_buf16;  vp16_far = rom_buf16;
    vp_farram = far_buf;     vp16_farram = far_buf16;

    BENCH(r_cal_empty,      loop_cal);
    BENCH(r_ld_ptr_near,    loop_ld_ptr_near);
    BENCH(r_ld_ptr_far,     loop_ld_ptr_far);
    BENCH(r_ld_idx_near,    loop_ld_idx_near);
    BENCH(r_ld_idx_far,     loop_ld_idx_far);
    BENCH(r_ld_ptr16_near,  loop_ld_ptr16_near);
    BENCH(r_ld_ptr16_far,   loop_ld_ptr16_far);
    BENCH(r_st_ptr_near,    loop_st_ptr_near);
    BENCH(r_st_idx_near,    loop_st_idx_near);
    BENCH(r_ld_ptr_farram,  loop_ld_ptr_farram);
    BENCH(r_ld_ptr16_farram, loop_ld_ptr16_farram);
    BENCH(r_ld_idx_farram,  loop_ld_idx_farram);
    BENCH(r_st_ptr_farram,  loop_st_ptr_farram);
    BENCH(r_st_idx_farram,  loop_st_idx_farram);

    r_sink = s;
    r_bench_done = 0xBEEF;
    while (1) { WaitForVBlank(); }
    return 0;
}
