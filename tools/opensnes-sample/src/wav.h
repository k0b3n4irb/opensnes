/* wav.h — PCM WAV parsing for opensnes-sample: in memory, so the fuzz
 * harness (tools/fuzz/fuzz_wav.c) feeds it bytes directly. */
#ifndef OPENSNES_SAMPLE_WAV_H
#define OPENSNES_SAMPLE_WAV_H

#include <stddef.h>
#include "basetypes.h"   /* s16, from tools/smconv/src */

typedef struct {
    s16 *pcm;        /* mono, signed 16-bit, malloc'd (caller frees) */
    int  count;      /* samples */
    int  rate, channels, bits;
} wav_t;

/* RIFF/WAVE, PCM only, 8- or 16-bit, mono or stereo (downmixed to mono).
 * Returns 1 and fills *w, or 0 with the reason in why[] (what is wrong,
 * what to do). */
int wav_parse(const unsigned char *buf, size_t len, wav_t *w, char *why, size_t why_len);

#endif
