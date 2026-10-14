#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "itloader.h"
#include "report.h"


/*==========================================================================
 * Pattern
 *==========================================================================*/

itl_pattern_t *itl_pattern_create(io_file_t *f)
{
    itl_pattern_t *p = calloc(1, sizeof(*p));
    p->data_length = io_read16(f);
    p->rows = io_read16(f);
    io_skip(f, 4); /* reserved */
    /* Same rule as the sample length: a pattern cannot hold more than what
     * is left of the file. Unchecked, 65535 patterns each claiming 64 KB
     * asked for 4 GB from a 2.5 KB input (tools/fuzz, 2026-09-22). */
    if ((u32)p->data_length > io_remaining(f))
        p->data_length = (u16)io_remaining(f);
    p->data = malloc(p->data_length ? p->data_length : 1);
    for (int i = 0; i < p->data_length; i++)
        p->data[i] = io_read8(f);
    return p;
}

itl_pattern_t *itl_pattern_create_empty(void)
{
    itl_pattern_t *p = calloc(1, sizeof(*p));
    p->rows = 64;
    p->data_length = 0;
    p->data = calloc(p->rows, 1);
    return p;
}

void itl_pattern_destroy(itl_pattern_t *p)
{
    if (!p) return;
    free(p->data);
    free(p);
}

/*==========================================================================
 * Envelope
 *==========================================================================*/

itl_envelope_t *itl_envelope_create(io_file_t *f)
{
    itl_envelope_t *e = calloc(1, sizeof(*e));
    u8 flg = io_read8(f);
    e->enabled = !!(flg & 1);
    e->loop = !!(flg & 2);
    e->sustain = !!(flg & 4);
    e->is_filter = !!(flg & 128);
    e->length = io_read8(f);
    e->loop_start = io_read8(f);
    e->loop_end = io_read8(f);
    e->sustain_start = io_read8(f);
    e->sustain_end = io_read8(f);
    for (int i = 0; i < 25; i++) {
        e->nodes[i].y = io_read8(f);
        e->nodes[i].x = io_read16(f);
    }
    return e;
}

void itl_envelope_destroy(itl_envelope_t *e)
{
    free(e);
}

/*==========================================================================
 * Instrument
 *==========================================================================*/

itl_instrument_t *itl_instrument_create(io_file_t *f)
{
    itl_instrument_t *inst = calloc(1, sizeof(*inst));
    io_skip(f, 4); /* IMPI */
    inst->dos_filename[12] = 0;
    for (int i = 0; i < 12; i++)
        inst->dos_filename[i] = io_read8(f);
    io_skip(f, 1); /* 00h */
    inst->new_note_action = io_read8(f);
    inst->duplicate_check_type = io_read8(f);
    inst->duplicate_check_action = io_read8(f);
    inst->fadeout = io_read16(f);
    inst->pps = io_read8(f);
    inst->ppc = io_read8(f);
    inst->global_volume = io_read8(f);
    inst->default_pan = io_read8(f);
    inst->random_volume = io_read8(f);
    inst->random_panning = io_read8(f);
    inst->tracker_version = io_read16(f);
    inst->number_of_samples = io_read8(f);

    inst->name[26] = 0;
    for (int i = 0; i < 26; i++)
        inst->name[i] = io_read8(f);

    inst->initial_filter_cutoff = io_read8(f);
    inst->initial_filter_resonance = io_read8(f);
    inst->midi_channel = io_read8(f);
    inst->midi_program = io_read8(f);
    inst->midi_bank = io_read16(f);
    io_read8(f); /* reserved */

    for (int i = 0; i < 120; i++) {
        inst->notemap[i].note = io_read8(f);
        inst->notemap[i].sample = io_read8(f);
    }

    inst->volume_envelope = itl_envelope_create(f);
    inst->panning_envelope = itl_envelope_create(f);
    inst->pitch_envelope = itl_envelope_create(f);
    return inst;
}

void itl_instrument_destroy(itl_instrument_t *inst)
{
    if (!inst) return;
    itl_envelope_destroy(inst->volume_envelope);
    itl_envelope_destroy(inst->panning_envelope);
    itl_envelope_destroy(inst->pitch_envelope);
    free(inst);
}

/*==========================================================================
 * Sample
 *==========================================================================*/

/*--------------------------------------------------------------------------
 * IT 2.14 / 2.15 sample decompression
 *
 * Ported from modlib, itmod/itsamplecodec.go and itmod/bitstream.go
 * (github.com/mukunda-/modlib), Copyright 2025 Mukunda Johnson
 * (mukunda.com), MIT licence — permission text in ATTRIBUTION.md. That
 * code is itself a port of OpenMPT's ITCompression.cpp and GreaseMonkey's
 * munch.py. Decoder only.
 *
 * A compressed sample is a sequence of blocks: a u16 byte length, then
 * that many bytes read as a bit stream, LSB first. A block yields at most
 * 0x8000 samples (8-bit) or 0x4000 (16-bit); the bit width and the two
 * integrators start afresh at each block.
 *--------------------------------------------------------------------------*/

typedef struct {
    const u8 *data;
    u32 size;
    u32 pos;
    u32 buffer;     /* at most 24 bits held: width <= 17, refilled by bytes */
    int buffered;
} itl_bits_t;

/* Read `width` bits (1..17). false when the block has no more bits. */
static bool itl_bits_read(itl_bits_t *b, int width, u32 *out)
{
    while (b->buffered < width) {
        if (b->pos >= b->size)
            return false;
        b->buffer |= (u32)b->data[b->pos++] << b->buffered;
        b->buffered += 8;
    }
    *out = b->buffer & ((1u << width) - 1);
    b->buffer >>= width;
    b->buffered -= width;
    return true;
}

/* Decode `count` samples of one block into out8 or out16. false when the
 * stream ends early or asks for a width above the default one. */
static bool itl_decode_block(itl_bits_t *b, bool bits16, bool it215,
                             s8 *out8, s16 *out16, u32 count)
{
    const int def_width = bits16 ? 17 : 9;
    int width = def_width;
    /* unsigned on purpose: the integrators wrap, and only their low 8 or
     * 16 bits are kept */
    u32 mem1 = 0, mem2 = 0;

    for (u32 i = 0; i < count; ) {
        u32 v;
        if (!itl_bits_read(b, width, &v))
            return false;
        u32 top = 1u << (width - 1);
        u32 next = 0;   /* new width, 0 = v is a sample */

        if (width <= 6) {
            /* 1 to 6 bits: the lone top bit announces a width, read next */
            if (v == top) {
                if (!itl_bits_read(b, bits16 ? 4 : 3, &next))
                    return false;
                next++;
                if (next >= (u32)width)
                    next++;
            }
        } else if (width < def_width) {
            /* 7 bits to default-1: a band around the top bit is a width */
            u32 lowest = top - (bits16 ? 8 : 4);
            if (v >= lowest && v <= lowest + (bits16 ? 15 : 7)) {
                next = v - lowest + 1;
                if (next >= (u32)width)
                    next++;
            }
        } else if (v & top) {
            /* default width: top bit set, the rest is the width itself */
            next = (v & ~top) + 1;
        }

        if (next) {
            if (next > (u32)def_width)
                return false;
            width = (int)next;
            continue;
        }

        if (v & top)
            v -= top << 1;   /* sign-extend from `width` bits */
        mem1 += v;
        mem2 += mem1;
        u32 smp = it215 ? mem2 : mem1;
        if (bits16)
            out16[i] = (s16)(u16)smp;
        else
            out8[i] = (s8)(u8)smp;
        i++;
    }
    return true;
}

/* Decode a whole compressed sample at the file position. The data is
 * signed already (`convert & 1` does not apply); bit 2 of `convert` selects
 * IT 2.15 (second integrator), as OpenMPT and Schism Tracker read it.
 * Returns 0, or the 1-based number of the block that could not be decoded. */
static int itl_sample_decompress(itl_sample_t *s, io_file_t *f)
{
    const bool bits16 = s->data.bits16;
    const bool it215 = !!(s->convert & 4);
    const u32 block_max = bits16 ? 0x4000 : 0x8000;
    u8 *block = malloc(0x10000);
    u32 length = (u32)s->data.length;
    int failed = 0;

    for (u32 done = 0, n = 1; done < length && !failed; n++) {
        /* every block is checked against what is left of the file before
         * a byte of it is read */
        itl_bits_t bits = {block, 0, 0, 0, 0};
        if (io_remaining(f) < 2) {
            failed = (int)n;
            break;
        }
        bits.size = io_read16(f);
        if (bits.size > io_remaining(f)) {
            failed = (int)n;
            break;
        }
        for (u32 i = 0; i < bits.size; i++)
            block[i] = io_read8(f);

        u32 count = length - done < block_max ? length - done : block_max;
        if (!itl_decode_block(&bits, bits16, it215,
                              bits16 ? NULL : s->data.data8 + done,
                              bits16 ? s->data.data16 + done : NULL, count))
            failed = (int)n;
        done += count;
    }
    free(block);
    return failed;
}

/* false when the sample data could not be loaded: the sample is then empty
 * (NULL data, length 0). */
static bool itl_sample_load_data(itl_sample_t *s, io_file_t *f)
{
    if (!s->compressed) {
        int offset = (s->convert & 1) ? 0 : (s->data.bits16 ? -32768 : -128);
        /* The header's length is trusted nowhere else: a corrupt file asked
         * for a 3.7 GB buffer here (found by tools/fuzz, 2026-09-14). A
         * sample cannot hold more than what is left of the file. */
        u32 avail = io_remaining(f) / (s->data.bits16 ? 2 : 1);
        if (avail > 0x7FFFFFFFu)
            avail = 0x7FFFFFFFu;   /* length is an int */
        if ((u32)s->data.length > avail) {   /* a negative length is huge here: caught too */
            smconv_report(SMC_WARNING, NULL, "sample '%s' claims %u frames but the file has %u left — truncated",
                          s->name, (unsigned)s->data.length, (unsigned)avail);
            s->data.length = (int)avail;
        }
        if (s->data.bits16) {
            s->data.data16 = malloc(s->data.length * sizeof(s16));
            for (int i = 0; i < s->data.length; i++)
                s->data.data16[i] = io_read16(f) + offset;
        } else {
            s->data.data8 = malloc(s->data.length * sizeof(s8));
            for (int i = 0; i < s->data.length; i++)
                s->data.data8[i] = io_read8(f) + offset;
        }
    } else {
        /* Same rule again, in bits: a sample costs at least one bit, so the
         * file cannot hold more than 8 frames per byte left. Past that the
         * stream is corrupt whatever it contains; the clamp only keeps the
         * allocation honest, the decoder then fails on the block that runs
         * out. */
        u32 avail = io_remaining(f) > 0x7FFFFFFFu / 8 ? 0x7FFFFFFFu : io_remaining(f) * 8;
        if ((u32)s->data.length > avail)
            s->data.length = (int)avail;
        size_t frames = s->data.length ? (size_t)s->data.length : 1;
        if (s->data.bits16)
            s->data.data16 = malloc(frames * sizeof(s16));
        else
            s->data.data8 = malloc(frames * sizeof(s8));
        int block = itl_sample_decompress(s, f);
        if (block) {
            smconv_report(SMC_ERROR, NULL, "sample '%s': corrupt compressed data (block %d)", s->name, block);
            if (s->data.bits16)
                free(s->data.data16);
            else
                free(s->data.data8);
            s->data.data8 = NULL;   /* the union: clears data16 too */
            s->data.length = 0;
            return false;
        }
    }
    return true;
}

itl_sample_t *itl_sample_create(io_file_t *f)
{
    itl_sample_t *s = calloc(1, sizeof(*s));
    io_skip(f, 4); /* IMPS */
    s->dos_filename[12] = 0;
    for (int i = 0; i < 12; i++)
        s->dos_filename[i] = io_read8(f);
    io_skip(f, 1); /* 00h */
    s->global_volume = io_read8(f);
    u8 flags = io_read8(f);

    s->has_sample = !!(flags & 1);
    s->data.bits16 = !!(flags & 2);
    s->stereo = !!(flags & 4);
    s->compressed = !!(flags & 8);
    s->data.loop = !!(flags & 16);
    s->data.sustain = !!(flags & 32);
    s->data.bidi_loop = !!(flags & 64);
    s->data.bidi_sustain = !!(flags & 128);

    s->default_volume = io_read8(f);

    s->name[26] = 0;
    for (int i = 0; i < 26; i++)
        s->name[i] = io_read8(f);

    s->convert = io_read8(f);
    s->default_panning = io_read8(f);

    s->data.length = io_read32(f);
    s->data.loop_start = io_read32(f);
    s->data.loop_end = io_read32(f);
    s->data.c5speed = io_read32(f);
    s->data.sustain_start = io_read32(f);
    s->data.sustain_end = io_read32(f);

    u32 sample_pointer = io_read32(f);

    s->vibrato_speed = io_read8(f);
    s->vibrato_depth = io_read8(f);
    s->vibrato_rate = io_read8(f);
    s->vibrato_form = io_read8(f);

    io_seek(f, sample_pointer);
    if (!itl_sample_load_data(s, f))
        s->invalid = 1;
    return s;
}

void itl_sample_destroy(itl_sample_t *s)
{
    if (!s) return;
    if (s->data.bits16)
        free(s->data.data16);
    else
        free(s->data.data8);
    free(s);
}

/*==========================================================================
 * SampleData (WAV loading)
 *==========================================================================*/

void itl_sample_data_init(itl_sample_data_t *sd)
{
    memset(sd, 0, sizeof(*sd));
}

itl_sample_data_t *itl_sample_data_from_wav(const char *filename)
{
    itl_sample_data_t *sd = calloc(1, sizeof(*sd));
    sd->c5speed = 32000;

    u32 file_size = io_file_size(filename);
    io_file_t f;
    io_init(&f);
    io_open(&f, filename, IO_MODE_READ);

    unsigned int bit_depth = 8;
    unsigned int hasformat = 0;
    unsigned int num_channels = 0;

    io_read32(&f); /* "RIFF" */
    io_read32(&f); /* filesize-8 */
    io_read32(&f); /* "WAVE" */

    while (1) {
        if (io_tell(&f) >= file_size)
            break;

        u32 chunk_code = io_read32(&f);
        u32 chunk_size = io_read32(&f);

        switch (chunk_code) {
        case 0x20746D66: /* fmt  */
            if (io_read16(&f) != 1) {
                smconv_report(SMC_FATAL, filename, "Unsupported WAV format");
                io_close(&f);
                return sd;
            }
            num_channels = io_read16(&f);
            sd->c5speed = io_read32(&f);
            io_read32(&f);
            io_read16(&f);
            bit_depth = io_read16(&f);
            if (bit_depth != 8 && bit_depth != 16) {
                smconv_report(SMC_FATAL, filename, "Unsupported WAV bit depth");
                io_close(&f);
                return sd;
            }
            if (bit_depth == 16)
                sd->bits16 = true;
            if ((chunk_size - 0x10) > 0)
                io_skip(&f, chunk_size - 0x10);
            hasformat = 1;
            break;

        case 0x61746164: { /* data */
            if (!hasformat) {
                smconv_report(SMC_FATAL, filename, "CORRUPT WAV FILE...");
                io_close(&f);
                return sd;
            }
            u32 br = file_size - io_tell(&f);
            chunk_size = chunk_size > br ? br : chunk_size;
            sd->length = chunk_size / (bit_depth / 8) / num_channels;

            if (bit_depth == 16)
                sd->data16 = malloc(chunk_size / 2 * sizeof(s16));
            else
                sd->data8 = malloc(chunk_size * sizeof(s8));

            for (int t = 0; t < sd->length; t++) {
                int dat = 0;
                for (unsigned int c = 0; c < num_channels; c++) {
                    dat += bit_depth == 8
                        ? ((int)io_read8(&f)) - 128
                        : ((short)io_read16(&f));
                }
                dat /= (int)num_channels;
                if (bit_depth == 8)
                    sd->data8[t] = dat;
                else
                    sd->data16[t] = dat;
            }
            break;
        }
        default:
            io_skip(&f, chunk_size);
        }
    }
    io_close(&f);
    return sd;
}

/*==========================================================================
 * Module
 *==========================================================================*/

itl_module_t *itl_module_create(const char *filename)
{
    itl_module_t *m = calloc(1, sizeof(*m));
    strncpy(m->filename, filename, sizeof(m->filename) - 1);

    io_file_t f;
    io_init(&f);
    if (!io_open(&f, filename, IO_MODE_READ)) {
        smconv_report(SMC_ERROR, filename, "cannot open '%s'", filename);
        m->invalid = 1;
        return m;
    }

    /* An Impulse Tracker module starts with "IMPM". Anything else used to
     * give an empty module and a successful run — a PNG passed by mistake
     * built a silent soundbank (2026-09-26 build audit). */
    if (io_read8(&f) != 'I' || io_read8(&f) != 'M' || io_read8(&f) != 'P' || io_read8(&f) != 'M') {
        smconv_report(SMC_ERROR, filename, "'%s' is not an Impulse Tracker module (no IMPM signature)", filename);
        m->invalid = 1;
        io_close(&f);
        return m;
    }

    for (int i = 0; i < 26; i++)
        m->title[i] = io_read8(&f);

    m->pattern_highlight = io_read16(&f);
    m->length = io_read16(&f);
    m->instrument_count = io_read16(&f);
    m->sample_count = io_read16(&f);
    m->pattern_count = io_read16(&f);
    m->cwtv = io_read16(&f);
    m->cmwt = io_read16(&f);
    m->flags = io_read16(&f);
    m->special = io_read16(&f);
    m->global_volume = io_read8(&f);
    m->mixing_volume = io_read8(&f);
    m->initial_speed = io_read8(&f);
    m->initial_tempo = io_read8(&f);
    m->sep = io_read8(&f);
    m->pwd = io_read8(&f);
    m->message_length = io_read16(&f);

    u32 message_offset = io_read32(&f);
    io_skip(&f, 4); /* reserved */

    for (int i = 0; i < 64; i++)
        m->channel_pan[i] = io_read8(&f);
    for (int i = 0; i < 64; i++)
        m->channel_volume[i] = io_read8(&f);

    bool foundend = false;
    int actual_length = m->length;
    for (int i = 0; i < 256; i++) {
        m->orders[i] = i < m->length ? io_read8(&f) : 255;
        if (m->orders[i] == 255 && !foundend) {
            foundend = true;
            actual_length = i + 1;
        }
    }
    m->length = actual_length;

    /* The three offset tables (4 bytes per entry) follow the orders. Counts
     * the file cannot even hold the tables for are a corrupt header, not a
     * module: refuse them the way a bad magic is refused, before allocating
     * (tools/fuzz, 2026-09-22: 65535 of each in a 2.5 KB file exhausted
     * memory, and a 4.4 KB one took 30 s). */
    if (4u * ((u32)m->instrument_count + m->sample_count + m->pattern_count) > io_remaining(&f)) {
        smconv_report(SMC_ERROR, filename, "'%s' declares %u instruments, %u samples, %u patterns — "
                      "more offset-table entries than the file has bytes; not an IT module",
                      filename, (unsigned)m->instrument_count, (unsigned)m->sample_count, (unsigned)m->pattern_count);
        m->instrument_count = m->sample_count = m->pattern_count = 0;
        m->invalid = 1;
        io_close(&f);
        return m;
    }

    m->instruments = malloc(m->instrument_count * sizeof(itl_instrument_t *));
    m->samples = malloc(m->sample_count * sizeof(itl_sample_t *));
    m->patterns = malloc(m->pattern_count * sizeof(itl_pattern_t *));

    u32 *instr_table = malloc(m->instrument_count * sizeof(u32));
    u32 *sample_table = malloc(m->sample_count * sizeof(u32));
    u32 *pattern_table = malloc(m->pattern_count * sizeof(u32));

    for (int i = 0; i < m->instrument_count; i++)
        instr_table[i] = io_read32(&f);
    for (int i = 0; i < m->sample_count; i++)
        sample_table[i] = io_read32(&f);
    for (int i = 0; i < m->pattern_count; i++)
        pattern_table[i] = io_read32(&f);

    for (int i = 0; i < m->instrument_count; i++) {
        io_seek(&f, instr_table[i]);
        m->instruments[i] = itl_instrument_create(&f);
    }

    for (int i = 0; i < m->sample_count; i++) {
        io_seek(&f, sample_table[i]);
        m->samples[i] = itl_sample_create(&f);
        /* a sample whose compressed data is corrupt fails the module, like
         * a bad signature: no soundbank from a half-loaded file */
        if (m->samples[i]->invalid)
            m->invalid = 1;
    }

    for (int i = 0; i < m->pattern_count; i++) {
        if (pattern_table[i]) {
            io_seek(&f, pattern_table[i]);
            m->patterns[i] = itl_pattern_create(&f);
        } else {
            m->patterns[i] = itl_pattern_create_empty();
        }
    }

    if (m->message_length) {
        m->message = malloc(m->message_length + 1);
        io_seek(&f, message_offset);
        m->message[m->message_length] = 0;
        for (int i = 0; i < m->message_length; i++)
            m->message[i] = io_read8(&f);
    } else {
        m->message = NULL;
    }

    free(instr_table);
    free(sample_table);
    free(pattern_table);
    io_close(&f);
    return m;
}

void itl_module_destroy(itl_module_t *m)
{
    if (!m) return;
    for (int i = 0; i < m->instrument_count; i++)
        itl_instrument_destroy(m->instruments[i]);
    free(m->instruments);
    for (int i = 0; i < m->sample_count; i++)
        itl_sample_destroy(m->samples[i]);
    free(m->samples);
    for (int i = 0; i < m->pattern_count; i++)
        itl_pattern_destroy(m->patterns[i]);
    free(m->patterns);
    free(m->message);
    free(m);
}

/*==========================================================================
 * Bank
 *==========================================================================*/

itl_bank_t *itl_bank_create(const char **files, int file_count)
{
    itl_bank_t *b = calloc(1, sizeof(*b));
    b->modules = malloc(file_count * sizeof(itl_module_t *));
    b->module_count = file_count;
    for (int i = 0; i < file_count; i++)
        b->modules[i] = itl_module_create(files[i]);
    b->sounds = NULL;
    b->sound_count = 0;
    return b;
}

void itl_bank_destroy(itl_bank_t *b)
{
    if (!b) return;
    for (int i = 0; i < b->module_count; i++)
        itl_module_destroy(b->modules[i]);
    free(b->modules);
    for (int i = 0; i < b->sound_count; i++) {
        if (b->sounds[i]->bits16)
            free(b->sounds[i]->data16);
        else
            free(b->sounds[i]->data8);
        free(b->sounds[i]);
    }
    free(b->sounds);
    free(b);
}
