#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "itloader.h"

#define ERRORRED(STRING) "\x1B[31m" STRING "\033[0m"
#define ERRORBRIGHT(STRING) "\x1B[97m" STRING "\033[0m"

/*==========================================================================
 * Pattern
 *==========================================================================*/

itl_pattern_t *itl_pattern_create(io_file_t *f)
{
    itl_pattern_t *p = calloc(1, sizeof(*p));
    p->data_length = io_read16(f);
    p->rows = io_read16(f);
    io_skip(f, 4); /* reserved */
    p->data = malloc(p->data_length);
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

static void itl_sample_load_data(itl_sample_t *s, io_file_t *f)
{
    if (!s->compressed) {
        int offset = (s->convert & 1) ? 0 : (s->data.bits16 ? -32768 : -128);
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
        printf("%s: " ERRORRED("error") ": unsupported compressed samples\n", ERRORBRIGHT("smconv"));
    }
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
    itl_sample_load_data(s, f);
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
                printf("%s: " ERRORRED("fatal error") ": Unsupported WAV format\n", ERRORBRIGHT("smconv"));
                io_close(&f);
                return sd;
            }
            num_channels = io_read16(&f);
            sd->c5speed = io_read32(&f);
            io_read32(&f);
            io_read16(&f);
            bit_depth = io_read16(&f);
            if (bit_depth != 8 && bit_depth != 16) {
                printf("%s: " ERRORRED("fatal error") ": Unsupported WAV bit depth\n", ERRORBRIGHT("smconv"));
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
                printf("%s: " ERRORRED("fatal error") ": CORRUPT WAV FILE...\n", ERRORBRIGHT("smconv"));
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
    io_open(&f, filename, IO_MODE_READ);

    if (io_read8(&f) != 'I') { io_close(&f); return m; }
    if (io_read8(&f) != 'M') { io_close(&f); return m; }
    if (io_read8(&f) != 'P') { io_close(&f); return m; }
    if (io_read8(&f) != 'M') { io_close(&f); return m; }

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
