/* wav.c — PCM WAV parsing for opensnes-sample (see wav.h). */
#include "wav.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int wav_parse(const unsigned char *buf, size_t fsz, wav_t *w, char *why, size_t why_len)
{
    memset(w, 0, sizeof *w);
    if (fsz < 44 || memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) {
        snprintf(why, why_len, "not a RIFF/WAVE file — export it as PCM WAV");
        return 0;
    }
    int have_fmt = 0;
    uint16_t fmt = 0, channels = 0, bits = 0;
    uint32_t rate = 0, data_bytes = 0;
    const uint8_t *data = NULL;
    size_t pos = 12;
    while (pos + 8 <= fsz) {
        const uint8_t *ck = buf + pos;
        uint32_t ck_sz = rd_u32(ck + 4);
        const uint8_t *body = ck + 8;
        if (ck_sz > fsz - (pos + 8)) ck_sz = (uint32_t)(fsz - (pos + 8));   /* a lying size: clamp */
        if (memcmp(ck, "fmt ", 4) == 0 && ck_sz >= 16) {
            fmt = rd_u16(body);
            channels = rd_u16(body + 2);
            rate = rd_u32(body + 4);
            bits = rd_u16(body + 14);
            if (fmt == 0xFFFE && ck_sz >= 26) fmt = rd_u16(body + 24);   /* EXTENSIBLE: the sub-format */
            have_fmt = 1;
        } else if (memcmp(ck, "data", 4) == 0) {
            data = body;
            data_bytes = ck_sz;
        }
        pos += 8 + (size_t)ck_sz + (ck_sz & 1);   /* chunks are word-aligned */
    }
    if (!have_fmt || !data) { snprintf(why, why_len, "no fmt or data chunk — not a complete WAV"); return 0; }
    if (fmt != 1) { snprintf(why, why_len, "format %u is not PCM — export as uncompressed PCM WAV", fmt); return 0; }
    if (channels < 1 || channels > 2) { snprintf(why, why_len, "%u channels — mono or stereo only", channels); return 0; }
    if (bits != 8 && bits != 16) { snprintf(why, why_len, "%u-bit — 8- or 16-bit PCM only", bits); return 0; }

    int frame = (bits / 8) * channels;
    int count = (int)(data_bytes / (uint32_t)frame);
    if (count <= 0) { snprintf(why, why_len, "no sample data"); return 0; }

    s16 *pcm = malloc((size_t)count * sizeof(s16));
    if (!pcm) { snprintf(why, why_len, "out of memory"); return 0; }
    for (int i = 0; i < count; i++) {
        const uint8_t *fp = data + (size_t)i * (size_t)frame;
        long acc = 0;
        for (int c = 0; c < channels; c++) {
            const uint8_t *sp = fp + c * (bits / 8);
            acc += bits == 16 ? (int16_t)rd_u16(sp) : ((int)sp[0] - 128) * 256;
        }
        pcm[i] = (s16)(acc / channels);
    }
    w->pcm = pcm; w->count = count; w->rate = (int)rate; w->channels = channels; w->bits = bits;
    return 1;
}
