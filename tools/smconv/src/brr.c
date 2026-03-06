#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "brr.h"

enum {
    MaxUnrollThreshold = 2000,
    LoopLossTolerance  = 30000,
    SAMPHEAD_END    = 1,
    SAMPHEAD_LOOP   = 2,
    SAMPHEAD_FILTER = 12,
    SAMPHEAD_RANGE  = 240
};

static int brr_clamp_nibble(int n)
{
    if (n < -8) n = -8;
    if (n > 7) n = 7;
    return n;
}

static int brr_clamp_word(int n)
{
    if (n < -32768) n = -32768;
    if (n > 32767) n = 32767;
    return n;
}

static int brr_compute_filter(int x_2, int x_1, int filter)
{
    int cp = 0;
    switch (filter) {
    case 0:
        cp = 0;
        break;
    case 1:
        cp = x_1;
        cp += -x_1 >> 4;
        break;
    case 2:
        cp = x_1 << 1;
        cp += -(x_1 + (x_1 << 1)) >> 5;
        cp += -x_2;
        cp += x_2 >> 4;
        break;
    case 3:
        cp = x_1 << 1;
        cp += -(x_1 + (x_1 << 2) + (x_1 << 3)) >> 6;
        cp += -x_2;
        cp += (x_2 + (x_2 << 1)) >> 4;
        break;
    }
    return cp;
}

static int test_gauss(int g4, int g3, int g2, const int *ls)
{
    int s;
    s  = (g4 * ls[0]) >> 11;
    s += (g3 * ls[1]) >> 11;
    s += (g2 * ls[2]) >> 11;
    return (s > 0x3FFF) || (s < -0x4000);
}

static unsigned char test_overflow(const int *ls)
{
    unsigned char r;
    r  = test_gauss(370, 1305, 374, ls);
    r |= test_gauss(366, 1305, 378, ls);
    r |= test_gauss(336, 1303, 410, ls);
    return r;
}

static void brr_compress_block(int *source, int *dest, brr_cresult_t *presult, int ffixed)
{
    int r_shift, r_half, c, s1, s2, rs1, rs2, ra, rb, cp, c_1, c_2, x;
    int block_data[16];
    int block_error;
    int block_errorb = 2147483647;
    int block_datab[16];
    int block_samp[16];
    int block_sampb[18];
    int block_rangeb = 0;
    int block_filterb = 0;
    int filter, fmin, fmax;

    if (ffixed == 4) { fmin = 0; fmax = 3; }
    else { fmin = ffixed; fmax = ffixed; }

    for (filter = fmin; filter <= fmax; filter++) {
        for (r_shift = 12; r_shift >= 0; r_shift--) {
            r_half = (1 << r_shift) >> 1;
            c_1 = source[-1];
            c_2 = source[-2];
            block_error = 0;
            for (x = 0; x < 16; x++) {
                cp = brr_compute_filter(c_2, c_1, filter);
                c = source[x] >> 1;
                s1 = (signed short)(c & 0x7FFF);
                s2 = (signed short)(c | 0x8000);
                s1 -= cp;
                s2 -= cp;
                s1 <<= 1;
                s2 <<= 1;
                s1 += r_half;
                s2 += r_half;
                s1 >>= r_shift;
                s2 >>= r_shift;
                s1 = brr_clamp_nibble(s1);
                s2 = brr_clamp_nibble(s2);
                rs1 = s1;
                rs2 = s2;
                s1 = (s1 << r_shift) >> 1;
                s2 = (s2 << r_shift) >> 1;
                if (filter >= 2) {
                    s1 = brr_clamp_word(s1 + cp);
                    s2 = brr_clamp_word(s2 + cp);
                } else {
                    s1 = s1 + cp;
                    s2 = s2 + cp;
                }
                s1 = ((signed short)(s1 << 1)) >> 1;
                s2 = ((signed short)(s2 << 1)) >> 1;
                ra = c - s1;
                rb = c - s2;
                if (ra < 0) ra = -ra;
                if (rb < 0) rb = -rb;
                if (ra < rb) {
                    block_error += ra;
                    block_data[x] = rs1;
                } else {
                    block_error += rb;
                    block_data[x] = rs2;
                    s1 = s2;
                }
                if (block_data[x] < 0)
                    block_data[x] += 16;
                c_2 = c_1;
                c_1 = s1;
                block_samp[x] = s1;
            }
            if (block_error < block_errorb) {
                block_errorb = block_error;
                block_rangeb = r_shift;
                block_filterb = filter;
                for (x = 0; x < 16; x++)
                    block_datab[x] = block_data[x];
                for (x = 0; x < 16; x++)
                    block_sampb[x + 2] = block_samp[x];
            }
        }
    }

    unsigned int overflow_bits = 0;
    block_sampb[0] = block_sampb[14 + 2];
    block_sampb[1] = block_sampb[15 + 2];
    for (x = 0; x < 16; x++)
        overflow_bits = (overflow_bits << 1) | test_overflow(block_sampb + x);

    for (x = 0; x < 16; x++)
        dest[x] = block_datab[x];
    for (x = 0; x < 16; x++)
        presult->samp[x] = block_sampb[x + 2];

    presult->range = block_rangeb;
    presult->filter = block_filterb;
    presult->samp_loss = block_errorb;
    presult->overflow = overflow_bits ? 1 : 0;
}

static inline double roundf_d(double d) { return floor(d + 0.5); }

static int generate_brr(const s16 *cdata, u8 *output, int srclength,
                        int srcloop, int srclooplen, int ffixed,
                        int loopfixed, double amp)
{
    int a, b, w;
    int w_loop = 0;
    int loop_v1 = 0;
    int use_filter0 = 0;
    int redo_loopf = 1;
    int got_loop = 0;

    (void)loopfixed;

    if (srclength == 0) return 0;

    int sbuffer[18];
    int cbuffer[16];
    brr_cresult_t cres;

    sbuffer[0] = 0;
    sbuffer[1] = 0;

    a = 0;
    w = 0;

    while (redo_loopf) {
        redo_loopf = 0;

        for (; a < srclength; a += 16) {
            for (b = 0; b < 16; b++) {
                if ((a + b) >= srclength && (a + b) >= 0)
                    sbuffer[b + 2] = 0;
                else
                    sbuffer[b + 2] = brr_clamp_word((int)roundf_d((double)cdata[a + b] * amp));
            }

            if (a != 0 && !use_filter0)
                brr_compress_block(sbuffer + 2, cbuffer, &cres, ffixed);
            else
                brr_compress_block(sbuffer + 2, cbuffer, &cres, 0);

            use_filter0 = 0;

            if (a == srcloop) {
                w_loop = w;
                loop_v1 = cres.samp[0];
            }

            if (cres.overflow)
                return 1;

            sbuffer[1] = cres.samp[15];
            sbuffer[0] = cres.samp[14];

            output[w] = (cres.range << 4) + (cres.filter << 2);
            for (b = 0; b < 8; b++)
                output[w + 1 + b] = cbuffer[b * 2] * 16 + cbuffer[b * 2 + 1];

            if (srclooplen) {
                output[w] |= SAMPHEAD_LOOP;
                if (!got_loop && a >= srcloop && a < (srcloop + srclooplen))
                    got_loop = 1;
            }
            w += 9;
        }

        w -= 9;
        output[w] = output[w] | SAMPHEAD_END;
        if (srclooplen)
            output[w] = output[w] | SAMPHEAD_LOOP;

        if (srclooplen) {
            int lc_range, lc_filter, lc_value;
            lc_filter = (output[w_loop] & SAMPHEAD_FILTER) >> 2;
            if (lc_filter == 0) {
                return 0;
            } else {
                lc_range = (output[w_loop] & SAMPHEAD_RANGE) >> 4;
                lc_value = output[w_loop + 1] >> 4;
                lc_value <<= lc_range;
                lc_value += brr_compute_filter(sbuffer[15], sbuffer[14], lc_filter);
                if (abs(lc_value - loop_v1) > LoopLossTolerance) {
                    a = srcloop;
                    w = w_loop;
                    redo_loopf = 1;
                    use_filter0 = 1;
                }
            }
        }
    }
    return 0;
}

static int unroll_count(int value, int bound)
{
    int test = value;
    int uc = 0;
    while (test % bound != 0) {
        test += value;
        uc++;
    }
    return uc;
}

static double resample_loop(s16 **data, int *length,
                            int *loopstart, int *looplength,
                            int amount)
{
    int new_looplength = *looplength + amount;
    int new_length = *length + amount;
    int new_loopstart = new_length - new_looplength;
    double factor = (double)(*length) / (double)new_length;

    s16 *newdata = malloc(new_length * sizeof(s16));
    for (int t = 0; t < new_length; t++) {
        int a_idx = (int)floor((double)t * factor);
        double sa = a_idx < *length ? (*data)[a_idx] : (*data)[a_idx - *looplength];

        int b_idx = ((int)floor((double)t * factor)) + 1;
        double sb = b_idx < *length ? (*data)[b_idx] : (*data)[b_idx - *looplength];

        double ratio = (double)t * factor;
        ratio -= floor(ratio);

        int c = (int)(floor(sa * (1 - ratio) + sb * ratio + 0.5));
        if (c < -32768) c = -32768;
        if (c > 32767) c = 32767;
        newdata[t] = c;
    }
    free(*data);
    *data = newdata;
    *length = new_length;
    *loopstart = new_loopstart;
    *looplength = new_looplength;
    return 1.0 / factor;
}

static void pad_data(s16 **data, int *length, int *loopstart,
                     int begin, int end)
{
    s16 *newdata = malloc((*length + begin + end) * sizeof(s16));
    s16 *out = newdata;
    for (int i = 0; i < begin; i++)
        *out++ = 0;
    for (int i = 0; i < *length; i++)
        *out++ = (*data)[i];
    for (int i = 0; i < end; i++)
        *out++ = 0;
    free(*data);
    *data = newdata;
    *length += begin + end;
    *loopstart += begin;
}

static void unroll_loop(s16 **data, int *length, int *loopstart,
                        int *looplength, int unroll)
{
    s16 *newdata = malloc((*length + *looplength * unroll) * sizeof(s16));
    s16 *out = newdata;
    for (int i = 0; i < *loopstart; i++)
        *out++ = (*data)[i];
    for (int uc = 0; uc < (unroll + 1); uc++)
        for (int i = *loopstart; i < *loopstart + *looplength; i++)
            *out++ = (*data)[i];
    free(*data);
    *data = newdata;
    *length += unroll * (*looplength);
    *looplength += unroll * (*looplength);
}

static void unroll_bidi_loop(s16 **data, int *length, int *loopstart,
                             int *looplength)
{
    (void)loopstart;
    s16 *newdata = malloc((*length + *looplength) * sizeof(s16));
    s16 *out = newdata;
    for (int i = 0; i < *length; i++)
        *out++ = (*data)[i];
    for (int i = 0; i < *looplength; i++)
        *out++ = (*data)[*length - 1 - i];
    free(*data);
    *data = newdata;
    *length += *looplength;
    *looplength += *looplength;
}

void brr_encode(const s8 *data8, const s16 *data16, int bits16,
                int src_length, int src_loop_start, int src_loop_end,
                int has_loop, int bidi_loop,
                u8 **out_data, int *out_length, int *out_loop,
                double *out_tuning)
{
    int length = src_length;
    int loopstart = src_loop_start;
    int looplength = src_loop_end - src_loop_start;

    *out_data = NULL;
    *out_length = 0;
    *out_loop = 0;
    *out_tuning = 1.0;

    if (length == 0) return;

    if (!has_loop)
        looplength = 0;

    if ((length > loopstart + looplength) && looplength)
        length = loopstart + looplength;

    s16 *cdata = malloc(length * sizeof(s16));
    if (bits16) {
        for (int i = 0; i < length; i++)
            cdata[i] = data16[i];
    } else {
        for (int i = 0; i < length; i++)
            cdata[i] = ((data8[i] * 32767) / 128);
    }

    if (bidi_loop)
        unroll_bidi_loop(&cdata, &length, &loopstart, &looplength);

    double tuning_factor = 1.0;

    if (looplength) {
        if (looplength & 0xF) {
            int uc = unroll_count(looplength & 0xF, 16);
            if ((looplength * (1 + uc)) < MaxUnrollThreshold)
                unroll_loop(&cdata, &length, &loopstart, &looplength, uc);
            else
                tuning_factor = resample_loop(&cdata, &length, &loopstart, &looplength, 16 - (looplength & 15));
        }
        if (loopstart & 15)
            pad_data(&cdata, &length, &loopstart, 16 - (loopstart & 15), 0);
    } else {
        if (length & 0xF)
            pad_data(&cdata, &length, &loopstart, 0, 16 - (length & 0xF));
    }

    int brr_length = length / 16 * 9;
    int brr_loop = (loopstart / 16) * 9;
    u8 *brr_data = malloc(brr_length);

    double amp = 1.0;
    u8 c_redo = 1;
    while (c_redo) {
        c_redo = generate_brr(cdata, brr_data, length, loopstart, looplength, 4, 4, amp);
        amp -= 0.01;
    }

    free(cdata);

    *out_data = brr_data;
    *out_length = brr_length;
    *out_loop = brr_loop;
    *out_tuning = tuning_factor;
}
