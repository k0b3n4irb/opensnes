#ifndef BRR_H
#define BRR_H

#include "basetypes.h"

/*
 * BRR audio compression for SNES SPC700.
 *
 * brr_encode() takes 16-bit PCM sample data and produces BRR-compressed
 * output suitable for the SNES DSP.
 *
 * Returns: number of BRR bytes written to output, or 0 on empty input.
 * Sets *out_tuning_factor to the resampling correction factor.
 */

typedef struct {
    int samp_loss;
    int range;
    int filter;
    int peak;
    int samp[16];
    int overflow; /* bool */
} brr_cresult_t;

/*
 * Encode a sample (from itl_sample_data_t fields) into BRR format.
 *
 * Parameters:
 *   data8/data16  - source PCM data (8-bit or 16-bit signed)
 *   bits16        - true if data is 16-bit
 *   length        - number of samples
 *   loop_start    - loop start point (samples)
 *   loop_end      - loop end point (samples)
 *   has_loop      - whether the sample loops
 *   bidi_loop     - whether the loop is bidirectional
 *   out_data      - receives malloc'd BRR data (caller frees)
 *   out_length    - receives BRR data length
 *   out_loop      - receives BRR loop offset
 *   out_tuning    - receives tuning correction factor
 */
void brr_encode(const s8 *data8, const s16 *data16, int bits16,
                int length, int loop_start, int loop_end,
                int has_loop, int bidi_loop,
                u8 **out_data, int *out_length, int *out_loop,
                double *out_tuning);

#endif
