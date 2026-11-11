/*
 * memcpy, memmove and memset against an NMI callback that calls them too
 * (stress fixture).
 *
 * The block move takes its two banks as operands of the instruction, and the
 * CPU fetches that instruction again for every byte it moves; the library
 * writes it on the stack for each call (string.asm), so that a call made by
 * an interrupt cannot rewrite the one a running call is executing.
 *
 * The main thread never waits for VBlank, so every NMI is a lag frame and
 * runs the nmiSetLag callback wherever the main thread is: most of the time
 * inside one of its long block moves ($7E -> $7F, a backward move in $7F, a
 * fill of $7F). The callback does its own block moves between other banks
 * (ROM -> plain RAM, backward in plain RAM, a fill of FAR RAM) and checks
 * them; the main thread checks each of its results. With the instruction at
 * a fixed address instead of on the stack, this ROM crashes at frame 78 (the
 * main thread's move resumed with the callback's banks).
 *
 * Read by string_nmi.toml: bad counts 0, iterations, and the number of
 * callbacks that landed while the main thread was inside a call.
 */
#include <snes.h>
#include <snes/string.h>

#define ITERS 600u
#define MAIN_N 1200u
#define CB_N 40u
#define STEP 5u

extern const u8 sn_rom[];

u16 r_bad_cpy;      /* main: wrong bytes after memcpy                 -> 0 */
u16 r_bad_mov;      /* main: wrong bytes after memmove (backward)     -> 0 */
u16 r_bad_set;      /* main: wrong bytes after memset                 -> 0 */
u16 r_iters;        /* main iterations run                       -> ITERS */
u16 r_cb_bad;       /* callback: wrong results                        -> 0 */
u16 r_cb_runs;      /* callbacks run (one a frame) */
u16 r_cb_inside;    /* callbacks that landed inside a main thread's call */
u16 r_done;         /* 0xC0DE at the end */

static FAR u8 src7e[MAIN_N + 64];
#define DST7F ((u8 FAR *)0x7F2000UL)
#define MOV7F ((u8 FAR *)0x7F8000UL)

static volatile u16 in_call;
static volatile u16 cb_runs, cb_inside, cb_bad;
static u8 cb_lo[CB_N + 8];
static FAR u8 cb_far[CB_N + 8];

static void tick(void)
{
    u16 i;
    cb_runs++;
    if (in_call) cb_inside++;
    memcpy(cb_lo + 1, sn_rom + (cb_runs & 7), CB_N);
    for (i = 0; i < CB_N; i++)
        if (cb_lo[1 + i] != sn_rom[(cb_runs & 7) + i]) { cb_bad++; break; }
    memmove(cb_lo + 3, cb_lo + 1, CB_N - 2);     /* backward, MVP */
    for (i = 0; i < CB_N - 2; i++)
        if (cb_lo[3 + i] != sn_rom[(cb_runs & 7) + i]) { cb_bad++; break; }
    memset(cb_far + 1, (u8)cb_runs, CB_N);
    for (i = 0; i < CB_N; i++)
        if (cb_far[1 + i] != (u8)cb_runs) { cb_bad++; break; }
}

int main(void)
{
    u16 it, i, n, o, k;
    u16 bad_cpy = 0, bad_mov = 0, bad_set = 0;

    consoleInit();
    for (i = 0; i < sizeof(src7e); i++) src7e[i] = (u8)(i * 3 + 1);
    nmiSetLag(tick);
    setScreenOn();

    for (it = 0; it < ITERS; it++) {
        n = MAIN_N - (it & 31);         /* lengths and parities move, so */
        o = it & 1;                     /* the NMI lands everywhere */
        k = it & 15;

        in_call = 1;
        memcpy(DST7F + o, src7e + k, n);
        in_call = 0;
        /* every STEP-th byte and the last: a wrong bank or a wrong count
         * spoils a run of bytes, not one */
        for (i = 0; i < n; i += STEP)
            if (DST7F[o + i] != src7e[k + i]) { bad_cpy++; break; }
        if (DST7F[o + n - 1] != src7e[k + n - 1]) bad_cpy++;

        in_call = 1;
        memcpy(MOV7F, src7e + k, n + 4);
        memmove(MOV7F + 3, MOV7F + o, n);           /* backward, MVP */
        in_call = 0;
        for (i = 0; i < n; i += STEP)
            if (MOV7F[3 + i] != src7e[k + o + i]) { bad_mov++; break; }
        if (MOV7F[3 + n - 1] != src7e[k + o + n - 1]) bad_mov++;

        in_call = 1;
        memset(DST7F + o, (u8)(it ^ 0x5A), n);
        in_call = 0;
        for (i = 0; i < n; i += STEP)
            if (DST7F[o + i] != (u8)(it ^ 0x5A)) { bad_set++; break; }
        if (DST7F[o + n - 1] != (u8)(it ^ 0x5A)) bad_set++;
    }
    nmiSetLag(0);
    r_bad_cpy = bad_cpy;
    r_bad_mov = bad_mov;
    r_bad_set = bad_set;
    r_iters = it;
    r_cb_bad = cb_bad;
    r_cb_runs = cb_runs;
    r_cb_inside = cb_inside;
    r_done = 0xC0DE;
    while (1) { }
    return 0;
}
