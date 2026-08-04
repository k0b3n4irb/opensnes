/*
 * wav2brr - WAV -> SNES BRR sample converter for OpenSNES
 *
 * Turns a PCM .wav file (a jump, a hit, a UI blip, a voice clip) into a
 * .brr sample loadable at runtime with audioLoadSample(). smconv already
 * covers music (IT modules); this fills the one-shot / sound-effect gap.
 *
 * The heavy lifting - BRR block compression, 16-sample padding, loop
 * unrolling/resampling, overflow-driven amplitude backoff - is the exact
 * encoder smconv uses on its own samples (tools/smconv/src/brr.c), so a
 * .brr from here is bit-identical in format to one baked into a soundbank.
 *
 * Usage:
 *   wav2brr [options] input.wav [output.brr]
 *   --loop START END   mark a sample-index loop (default: one-shot, no loop)
 *   -v, --verbose      print the input/output breakdown
 *   -h, --help         this help
 *
 * (C) OpenSNES. BRR encoder (C) 2009 Mukunda Johnson (smconv), reused here.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "brr.h"   /* from tools/smconv/src, via -I */

#ifndef VERSION
#define VERSION "1.0.0"
#endif

/*----------------------------------------------------------------------------
 * Little-endian readers (WAV is always little-endian, host may not be)
 *--------------------------------------------------------------------------*/
static uint16_t rd_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/*----------------------------------------------------------------------------
 * WAV parsing: RIFF/WAVE, PCM only, 8- or 16-bit, mono or stereo.
 * On success fills *out_pcm (malloc'd s16 mono), *out_count, *out_rate,
 * *out_channels, *out_bits and returns 0. Returns non-zero with a message
 * on stderr otherwise.
 *--------------------------------------------------------------------------*/
static int parse_wav(const char *path, s16 **out_pcm, int *out_count,
                     int *out_rate, int *out_channels, int *out_bits)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "wav2brr: cannot open %s\n", path); return 1; }

    fseek(f, 0, SEEK_END);
    long fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsz < 44) {
        fprintf(stderr, "wav2brr: %s is too small to be a WAV file\n", path);
        fclose(f);
        return 1;
    }
    uint8_t *buf = malloc((size_t)fsz);
    if (!buf || fread(buf, 1, (size_t)fsz, f) != (size_t)fsz) {
        fprintf(stderr, "wav2brr: read error on %s\n", path);
        free(buf);
        fclose(f);
        return 1;
    }
    fclose(f);

    if (memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) {
        fprintf(stderr, "wav2brr: %s is not a RIFF/WAVE file\n", path);
        free(buf);
        return 1;
    }

    /* Walk the chunk list looking for 'fmt ' and 'data'. */
    int have_fmt = 0;
    uint16_t fmt = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    const uint8_t *data = NULL;
    uint32_t data_bytes = 0;

    long pos = 12;
    while (pos + 8 <= fsz) {
        const uint8_t *ck = buf + pos;
        uint32_t ck_sz = rd_u32(ck + 4);
        const uint8_t *body = ck + 8;
        if ((long)(pos + 8 + ck_sz) > fsz)
            ck_sz = (uint32_t)(fsz - (pos + 8));   /* clamp a bad size */

        if (memcmp(ck, "fmt ", 4) == 0 && ck_sz >= 16) {
            fmt = rd_u16(body);
            channels = rd_u16(body + 2);
            rate = rd_u32(body + 4);
            bits = rd_u16(body + 14);
            /* WAVE_FORMAT_EXTENSIBLE: real format is in the subformat GUID */
            if (fmt == 0xFFFE && ck_sz >= 26)
                fmt = rd_u16(body + 24);
            have_fmt = 1;
        } else if (memcmp(ck, "data", 4) == 0) {
            data = body;
            data_bytes = ck_sz;
        }
        pos += 8 + ck_sz + (ck_sz & 1);   /* chunks are word-aligned */
    }

    if (!have_fmt || !data) {
        fprintf(stderr, "wav2brr: %s missing fmt/data chunk\n", path);
        free(buf);
        return 1;
    }
    if (fmt != 1) {
        fprintf(stderr, "wav2brr: %s is not PCM (format %u) - export as "
                        "uncompressed PCM WAV\n", path, fmt);
        free(buf);
        return 1;
    }
    if (channels < 1 || channels > 2) {
        fprintf(stderr, "wav2brr: %s has %u channels (need mono or stereo)\n",
                path, channels);
        free(buf);
        return 1;
    }
    if (bits != 8 && bits != 16) {
        fprintf(stderr, "wav2brr: %s is %u-bit (need 8- or 16-bit PCM)\n",
                path, bits);
        free(buf);
        return 1;
    }

    int bytes_per_samp = bits / 8;
    int frame = bytes_per_samp * channels;
    int count = (int)(data_bytes / (uint32_t)frame);
    if (count <= 0) {
        fprintf(stderr, "wav2brr: %s has no sample data\n", path);
        free(buf);
        return 1;
    }

    s16 *pcm = malloc((size_t)count * sizeof(s16));
    for (int i = 0; i < count; i++) {
        const uint8_t *fp = data + (size_t)i * frame;
        long acc = 0;
        for (int c = 0; c < channels; c++) {
            const uint8_t *sp = fp + c * bytes_per_samp;
            int s;
            if (bits == 16)
                s = (int16_t)rd_u16(sp);              /* signed LE */
            else
                s = ((int)sp[0] - 128) << 8;          /* 8-bit is unsigned */
            acc += s;
        }
        pcm[i] = (s16)(acc / channels);              /* downmix to mono */
    }

    free(buf);
    *out_pcm = pcm;
    *out_count = count;
    *out_rate = (int)rate;
    *out_channels = channels;
    *out_bits = bits;
    return 0;
}

/* Derive a C identifier stem from a filename: "res/jump.brr" -> "jump". */
static void ident_from_path(const char *path, char *out, size_t n)
{
    const char *base = path;
    for (const char *p = path; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    size_t j = 0;
    for (const char *p = base; *p && *p != '.' && j + 1 < n; p++) {
        char c = *p;
        out[j++] = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') ? c : '_';
    }
    if (j == 0) out[j++] = 's';
    out[j] = '\0';
}

static void usage(FILE *o)
{
    fprintf(o,
        "wav2brr " VERSION " - WAV -> SNES BRR sample converter\n\n"
        "Usage: wav2brr [options] input.wav [output.brr]\n\n"
        "  --loop START END   loop between sample indices START..END\n"
        "  -v, --verbose      print the input/output breakdown\n"
        "  -h, --help         show this help\n\n"
        "Output feeds audioLoadSample() at runtime. With no output path,\n"
        "input.wav is written alongside as input.brr.\n");
}

int main(int argc, char **argv)
{
    const char *in = NULL, *out = NULL;
    int verbose = 0, has_loop = 0, loop_start = 0, loop_end = 0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(stdout); return 0; }
        else if (!strcmp(a, "-v") || !strcmp(a, "--verbose")) verbose = 1;
        else if (!strcmp(a, "--loop")) {
            if (i + 2 >= argc) { fprintf(stderr, "wav2brr: --loop needs START END\n"); return 1; }
            has_loop = 1;
            loop_start = atoi(argv[++i]);
            loop_end = atoi(argv[++i]);
        } else if (a[0] == '-' && a[1]) {
            fprintf(stderr, "wav2brr: unknown option %s\n", a);
            usage(stderr);
            return 1;
        } else if (!in) in = a;
        else if (!out) out = a;
        else { fprintf(stderr, "wav2brr: too many arguments\n"); return 1; }
    }

    if (!in) { usage(stderr); return 1; }

    /* Default output: input.wav -> input.brr */
    char outbuf[1024];
    if (!out) {
        size_t len = strlen(in);
        const char *dot = strrchr(in, '.');
        size_t base = dot ? (size_t)(dot - in) : len;
        if (base + 5 >= sizeof(outbuf)) { fprintf(stderr, "wav2brr: path too long\n"); return 1; }
        memcpy(outbuf, in, base);
        strcpy(outbuf + base, ".brr");
        out = outbuf;
    }

    s16 *pcm = NULL;
    int count = 0, rate = 0, channels = 0, bits = 0;
    if (parse_wav(in, &pcm, &count, &rate, &channels, &bits) != 0)
        return 1;

    if (has_loop) {
        if (loop_start < 0 || loop_end > count || loop_start >= loop_end) {
            fprintf(stderr, "wav2brr: --loop %d %d out of range for %d samples\n",
                    loop_start, loop_end, count);
            free(pcm);
            return 1;
        }
    }

    u8 *brr = NULL;
    int brr_len = 0, brr_loop = 0;
    double tuning = 1.0;
    brr_encode(NULL, pcm, 1, count,
               has_loop ? loop_start : 0,
               has_loop ? loop_end : 0,
               has_loop, 0,
               &brr, &brr_len, &brr_loop, &tuning);
    free(pcm);

    if (!brr || brr_len == 0) {
        fprintf(stderr, "wav2brr: encoding produced no output\n");
        free(brr);
        return 1;
    }

    FILE *fo = fopen(out, "wb");
    if (!fo) { fprintf(stderr, "wav2brr: cannot write %s\n", out); free(brr); return 1; }
    if (fwrite(brr, 1, (size_t)brr_len, fo) != (size_t)brr_len) {
        fprintf(stderr, "wav2brr: write error on %s\n", out);
        fclose(fo);
        free(brr);
        return 1;
    }
    fclose(fo);

    char ident[256];
    ident_from_path(out, ident, sizeof(ident));

    if (verbose) {
        printf("wav2brr: %s -> %s\n", in, out);
        printf("  input : %d Hz, %s, %d-bit, %d samples (%.2f s)\n",
               rate, channels == 1 ? "mono" : "stereo (downmixed)", bits,
               count, rate ? (double)count / rate : 0.0);
        printf("  output: %d bytes (%d BRR blocks), ", brr_len, brr_len / 9);
        if (has_loop) printf("loop at byte %d\n", brr_loop);
        else printf("no loop\n");
        if (tuning != 1.0)
            printf("  tuning: %.4f (loop was resampled; adjust playback pitch)\n", tuning);
        if (rate > 32000)
            printf("  note  : %d Hz exceeds the ~32 kHz DSP rate; it will play "
                   "back higher-pitched unless resampled\n", rate);
        printf("  load  : extern u8 %s_brr[], %s_brr_end[];\n", ident, ident);
        printf("          audioLoadSample(id, %s_brr, %s_brr_end - %s_brr, %d);\n",
               ident, ident, ident, brr_loop);
    } else {
        printf("wav2brr: wrote %s (%d bytes, %d blocks%s)\n",
               out, brr_len, brr_len / 9, has_loop ? ", looping" : "");
    }

    free(brr);
    return 0;
}
