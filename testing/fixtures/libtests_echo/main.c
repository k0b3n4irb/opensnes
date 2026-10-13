/**
 * @file main.c
 * @brief Library fixture: the echo ring takes what its delay needs, at the top.
 *
 * Three "samples" of 16 KB each are loaded (a fill, never played): the
 * sample area then ends at $CB00. A delay of 7 (14 KB, ring at $C700) does
 * not fit and is refused with nothing changed; a delay of 6 (ring at $CF00)
 * fits. Two samples are given back and the delay goes from 6 to 1: the
 * S-DSP keeps the old ring's length until it reaches its end, and from the
 * new start ($F700) a 12 KB ring passes $FFFF and goes on over the driver.
 * The driver waits with the ring's writes off; it must still answer after.
 *
 * Read by test_libtest_echo.py.
 */
#include <snes.h>
#include <snes/audio.h>

#define CHUNK 0x4000

extern const u8 echo_fill[];   /* data.asm: CHUNK bytes, none of them 0 */

u16 r_init;          /* audioInit()                                  -> 0 */
u16 r_load;          /* the three loads, or'ed                       -> 0 */
u16 r_free_full;     /* no echo: 0xFFC0 - 0xCB00                     -> 0x34C0 */
u16 r_refuse7;       /* audioSetEcho(7): AUDIO_ERR_NO_MEMORY         -> 1 */
u16 r_free_refused;  /* nothing changed                              -> 0x34C0 */
u16 r_ok6;           /* audioSetEcho(6)                              -> 0 */
u16 r_free6;         /* 0xCF00 - 0xCB00                              -> 0x0400 */
u16 r_ok1;           /* audioSetEcho(1) after two unloads            -> 0 */
u16 r_free1;         /* 0xF700 - 0x4B00: the shorter ring gave back  -> 0xAC00 */
u16 r_alive;         /* a command twenty frames later: answered      -> 0 */
u16 r_ok7;           /* audioSetEcho(7) now that it fits             -> 0 */
u16 r_free7;         /* 0xC700 - 0x4B00                              -> 0x7C00 */
u16 r_done;          /* 0xBEEF */

int main(void) {
    u8 i;

    consoleInit();
    r_init = audioInit();
    r_load = audioLoadSample(0, echo_fill, CHUNK, 0);
    r_load |= audioLoadSample(1, echo_fill, CHUNK, 0);
    r_load |= audioLoadSample(2, echo_fill, CHUNK, 0);
    r_free_full = audioGetFreeMemory();

    r_refuse7 = audioSetEcho(7, 40, 20, 20);
    r_free_refused = audioGetFreeMemory();
    r_ok6 = audioSetEcho(6, 40, 20, 20);
    r_free6 = audioGetFreeMemory();
    audioEnableEcho(0xFF);

    audioUnloadSample(2);
    audioUnloadSample(1);
    r_ok1 = audioSetEcho(1, 40, 20, 20);
    r_free1 = audioGetFreeMemory();
    for (i = 0; i < 20; i++) {
        WaitForVBlank();
    }
    r_alive = audioSetVoiceVolume(0, 64, 64);
    r_alive |= audioSetVoiceVolume(0, 64, 64);   /* waits for the ack of the one before */
    r_ok7 = audioSetEcho(7, 40, 20, 20);
    r_free7 = audioGetFreeMemory();

    r_done = 0xBEEF;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
