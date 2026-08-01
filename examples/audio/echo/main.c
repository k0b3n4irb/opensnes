/**
 * @file main.c
 * @brief Family 8 (Audio) · rung 8.9 — echo / reverb from the S-DSP
 * @ingroup examples
 *
 * Isolates one lesson the soundboard buries among many: the S-DSP's hardware
 * echo unit. A short "pop" fires on a timer; the echo turns each pop into a
 * decaying tail. Press START to toggle echo off and on — that A/B *is* the
 * point, because you cannot hear reverb without the dry sound next to it.
 *
 * @par SNES Concepts
 * - audioSetEcho(delay, feedback, volL, volR) sizes the echo ring and sets how
 *   long the tail rings (feedback) and how loud the wet signal is — call it
 *   BEFORE enabling
 * - audioSetEchoFilter(fir[8]) is the 8-tap FIR on the echo path; tap 0 = 127
 *   passes the echo straight through
 * - audioEnableEcho(voiceMask) routes voices into the echo; audioDisableEcho()
 *   turns it off
 * - Echo costs ARAM — the ring buffer is real memory, sized by the delay
 *
 * @par What to Observe
 * - A pop roughly twice a second with a reverb tail; press START and the tail
 *   vanishes (dry), press again and the hall returns. The backdrop tints
 *   purple (wet) vs grey (dry).
 *
 * @par Modules Used
 * console, audio, input
 *
 * @see audio.h (audioSetEcho / audioEnableEcho), audio/soundboard
 */

#include <snes.h>
#include <snes/audio.h>
#include <snes/input.h>

/** @brief A short percussive "pop" one-shot (data.asm; shared with soundboard) */
extern u8 brr_pop[], brr_pop_end[];

/** @brief Sample slot for the pop */
#define SMP_POP 0

int main(void) {
    u16 frame = 0;
    u8  echo_on = 1;
    u16 keys;

    consoleInit();
    audioInit();

    /* Stream the one-shot pop into APU RAM. */
    audioLoadSample(SMP_POP, brr_pop, (u16)(brr_pop_end - brr_pop), 0);

    /* Turn the echo on: a long, obvious hall. Feedback high so the tail
     * rings; FIR tap 0 open so the echo passes through unfiltered. Order
     * matters — configure, then enable. */
    {
        static const s8 hall_fir[8] = { 127, 0, 0, 0, 0, 0, 0, 0 };
        audioSetEcho(6, 90, 40, 40);   /* delay, feedback, volL, volR */
        audioSetEchoFilter(hall_fir);
        audioEnableEcho(0xFF);
    }

    setColor(0, RGB(6, 4, 16));        /* wet: purple backdrop */
    setScreenOn();

    while (1) {
        WaitForVBlank();
        keys = padPressed(0);

        /* Fire the pop about every 40 frames (~0.7 s) so the decaying tail
         * is clearly audible between hits. */
        if ((frame++ % 40) == 0) {
            audioPlaySample(SMP_POP);
        }

        /* START toggles the echo — the dry/wet A/B that teaches what it does. */
        if (keys & KEY_START) {
            if (echo_on) {
                audioDisableEcho();
                echo_on = 0;
                setColor(0, RGB(4, 4, 4));    /* dry: grey */
            } else {
                audioEnableEcho(0xFF);
                echo_on = 1;
                setColor(0, RGB(6, 4, 16));   /* wet: purple */
            }
        }
    }

    return 0;
}
