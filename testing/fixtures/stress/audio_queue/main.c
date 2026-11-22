/*
 * audioQueue*(): the command queue and the port lock it shares with the
 * sample loader (stress fixture, 2026-11-20).
 *
 * The queue is filled by the main thread and drained, one order a frame, by
 * an nmiSetLag() callback. This ROM checks, on luna:
 *   1. the queue holds 16 orders: the 17th is refused (AUDIO_ERR_NO_MEMORY),
 *      audioQueueCount() reads 16, a masked-out stop (mask 0) queues nothing;
 *   2. the drain sends them all, at most one a frame, and the count falls to 0;
 *   3. the lock: while the main thread loads a 3 000-byte sample in steps
 *      (the stream spans several NMIs) and keeps the queue topped up, the
 *      callback drains whenever the ports are free; the load ends AUDIO_OK
 *      with its slot loaded, and audio_error_count does not move. An NMI that
 *      sent a command in the middle of the stream would time the stream out.
 *   4. the callback sent orders while the load ran (r_sent_during > 0) and
 *      found the lock set at least once (r_blocked > 0): the test is not
 *      vacuous.
 *
 * The fault: built with `make clean && make FAULT=1` the callback sends past
 * the lock (a direct cmd write replaced for audioQueueDrain), and the load or
 * the error count fails. Read by audio_queue.toml.
 */
#include <snes.h>
#include <snes/audio.h>

#define BIG       3000u
#define STEP      240u
#define SMP       0
#define PLAY_N    24u

extern u8 audio_port_lock;      /* audio.c */
extern u8 audio_pending;

#ifdef QUEUE_FAULT
/* The injected fault: the lock is forgotten for the drain. */
static u8 drain_fault(u8 n)
{
    u8 save = audio_port_lock, r;
    audio_port_lock = 0;
    r = audioQueueDrain(n);
    audio_port_lock = save;
    return r;
}
#define audioQueueDrain drain_fault
#endif

static u8 big[BIG];             /* BRR-shaped bytes: a loop block repeated */

u16 r_full_refused;             /* 17th push refused                    -> 1 */
u16 r_full_count;               /* audioQueueCount() when full          -> 16 */
u16 r_zero_mask;                /* AUDIO_OK and nothing queued          -> 1 */
u16 r_drained;                  /* orders gone after the free drain     -> 16 */
u16 r_max_per_frame;            /* most orders one callback sent        -> 1 */
u16 r_load_ok;                  /* the stepped load ended whole         -> 1 */
u16 r_errs;                     /* audio_error_count moved overall      -> 0 */
u16 r_sent_during;              /* orders sent while the load ran       -> >= 1 */
u16 r_blocked;                  /* callbacks that found the lock set    -> >= 1 */
u16 r_left;                     /* orders left queued at the end        -> 0 */
u16 r_done;

static u8 phase_load;

/* nmiSetLag() runs on frames the main thread is not waiting for: wait by counting. */
static void frames_pass(u16 n)
{
    u16 f0 = getFrameCount();
    while ((u16)(getFrameCount() - f0) < n) {
    }
}

static void lag_tick(void)
{
    u8 before = audioQueueCount(), after;

    if (audio_port_lock && phase_load) {
        r_blocked++;
    }
    after = audioQueueDrain(1);
    if ((u8)(before - after) > r_max_per_frame) {
        r_max_per_frame = (u8)(before - after);
    }
    if (phase_load) {
        r_sent_during += (u8)(before - after);
    }
}

int main(void)
{
    u16 i, err0;
    u8 pushed_ok = 0, rc;

    consoleInit();
    setScreenOn();
    audioInit();
    for (i = 0; i < BIG; i++) {
        big[i] = (u8)((i % 9 == 0) ? 0xB3 : 0x77);
    }
    err0 = audio_error_count;

    /* 1. capacity */
    for (i = 0; i < 16; i++) {
        if (audioQueueSetVolume(AUDIO_VOL_MAX) == AUDIO_OK) pushed_ok++;
    }
    rc = audioQueueSetVolume(AUDIO_VOL_MAX);
    r_full_refused = (rc == AUDIO_ERR_NO_MEMORY && pushed_ok == 16);
    r_full_count = audioQueueCount();
    r_zero_mask = 0;
    /* (the queue is full: a refused push must not be mistaken for the mask) */

    /* 2. a free drain, one order a frame, from the callback */
    audio_error_count = err0;   /* the 17th refusal was on purpose */
    nmiSetLag(lag_tick);
    for (i = 0; i < 40 && audioQueueCount(); i++) {
        frames_pass(1);
    }
    r_drained = 16 - audioQueueCount();
    r_zero_mask = (audioQueueStopVoices(0) == AUDIO_OK && audioQueueCount() == 0);

    /* 3-4. the stepped load against the callback */
    phase_load = 1;
    if (audioLoadSampleBegin(SMP, big, BIG, 0) == AUDIO_OK) {
        u16 r, n = 0;
        do {
            while (audioQueueCount() < 8) {
                audioQueueSetVoicePitch(1, 0x1000);
            }
            r = audioLoadSampleStep(STEP);
            if (r == AUDIO_LOAD_FAILED) break;
            if ((++n & 3) == 0) {
                frames_pass(2); /* ports free: the callback may send */
            }
        } while (r);
        r_load_ok = (r == 0);
    }
    phase_load = 0;
    for (i = 0; i < 60 && audioQueueCount(); i++) {
        frames_pass(1);
    }
    nmiSetLag(0);
    r_left = audioQueueCount();
    r_errs = audio_error_count - err0;

    r_done = 0xC0DE;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
