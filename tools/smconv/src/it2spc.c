#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "it2spc.h"
#include "brr.h"
#include "spc_program.h"

#define ERRORRED(STRING) "\x1B[31m" STRING "\033[0m"
#define ERRORBRIGHT(STRING) "\x1B[97m" STRING "\033[0m"

static u8 g_current_instrument = 0;
static const u8 patch_byte = 0x3C;
static const u8 max_instruments = 64;
static const u8 max_length = 200;
static const u8 max_patterns = 64;
static const u8 max_samples = 64;
static const u16 header_size = 616;
static const u16 driver_base = 0x400;
static const u16 module_base = 0x18ca;
static const u16 sample_table_offset = 0x200;
static u16 spc_ram_size_g = 0;
static u16 totalsizem1 = 0;
static u32 totalitsize = 0;
static u32 totabanksize = 0;
static bool g_chksfx = false;
static bool g_verbose = false;
static int g_banknum = 5;
static bool g_no_header = false;
static const char *g_symbol_prefix = "SOUNDBANK__";

static int spc_program_size;

enum {
    SPC_RAM_SIZE = 58000,
    MODULE_BASE  = 0x1A00
};

/*==========================================================================
 * Helpers
 *==========================================================================*/

void spc_validate_id(char *id)
{
    for (int i = 0; id[i]; i++) {
        char c = id[i];
        if ((c >= 1 && c <= 47) ||
            (c >= 58 && c <= 64) ||
            (c >= 91 && c <= 96) ||
            ((unsigned char)c >= 123)) {
            id[i] = '_';
        }
    }
}

void spc_path2id(const char *prefix, const char *source, char *out, int out_size)
{
    char buf[256] = {0};
    int len = (int)strlen(source);
    if (len > 255) len = 255;
    memcpy(buf, source, len);

    /* uppercase and normalize slashes */
    for (int i = 0; buf[i]; i++) {
        buf[i] = toupper((unsigned char)buf[i]);
        if (buf[i] == '\\') buf[i] = '/';
    }

    /* find last slash */
    const char *start = strrchr(buf, '/');
    start = start ? start + 1 : buf;

    /* find first dot */
    char *dot = strchr((char *)start, '.');
    if (dot) *dot = '\0';

    snprintf(out, out_size, "%s%s", prefix, start);
    spc_validate_id(out);
}

/* Dynamic array push for spc_pattern_t data */
static void pat_push(spc_pattern_t *p, u8 val)
{
    if (p->data_size >= p->data_cap) {
        p->data_cap = p->data_cap ? p->data_cap * 2 : 256;
        p->data = realloc(p->data, p->data_cap);
    }
    p->data[p->data_size++] = val;
}

/*==========================================================================
 * Source
 *==========================================================================*/

spc_source_t *spc_source_create(const itl_sample_data_t *src)
{
    spc_source_t *s = calloc(1, sizeof(*s));
    brr_encode(src->data8, src->data16, src->bits16,
               src->length, src->loop_start, src->loop_end,
               src->loop, src->bidi_loop,
               &s->data, (int *)&s->length, (int *)&s->loop,
               &s->tuning_factor);
    return s;
}

void spc_source_destroy(spc_source_t *s)
{
    if (!s) return;
    free(s->data);
    free(s);
}

bool spc_source_compare(const spc_source_t *a, const spc_source_t *b)
{
    if (a->length != b->length) return false;
    if (a->loop != b->loop) return false;
    for (int i = 0; i < a->length; i++)
        if (a->data[i] != b->data[i])
            return false;
    return true;
}

void spc_source_export(const spc_source_t *s, io_file_t *f, bool spc_direct)
{
    if (!spc_direct) {
        io_write16(f, s->length);
        io_write16(f, s->loop);
    }
    for (int i = 0; i < s->length; i++)
        io_write8(f, s->data[i]);
    if (!spc_direct) {
        if (s->length & 1)
            io_write8(f, 0); /* align by 2 */
    }
}

/*==========================================================================
 * Sample
 *==========================================================================*/

spc_sample_t *spc_sample_create(const itl_sample_t *src, int directory, double tuning)
{
    spc_sample_t *s = calloc(1, sizeof(*s));
    s->default_volume = src->default_volume;
    s->global_volume = src->global_volume;
    s->set_pan = src->default_panning ^ 128;

    double a = (double)src->data.c5speed * tuning;
    s->pitch_base = (u16)(int)floor(log(a / 8363.0) / log(2.0) * 768.0 + 0.5);
    s->directory_index = directory;
    return s;
}

void spc_sample_destroy(spc_sample_t *s)
{
    free(s);
}

void spc_sample_export(const spc_sample_t *s, io_file_t *f)
{
    io_write8(f, s->default_volume);
    io_write8(f, s->global_volume);
    io_write16(f, s->pitch_base);
    io_write8(f, s->directory_index);
    io_write8(f, s->set_pan);
}

/*==========================================================================
 * Instrument
 *==========================================================================*/

spc_instrument_t *spc_instrument_create(const itl_instrument_t *src)
{
    spc_instrument_t *inst = calloc(1, sizeof(*inst));
    int a = src->fadeout / 4;
    inst->fadeout = a > 255 ? 255 : a;
    inst->sample_index = src->notemap[60].sample - 1;
    inst->global_volume = src->global_volume;
    inst->set_pan = src->default_pan;

    const itl_envelope_t *e = src->volume_envelope;
    inst->envelope_length = e->enabled ? e->length * 4 : 0;
    g_current_instrument++;

    if (inst->envelope_length) {
        inst->envelope_sustain = e->sustain ? e->sustain_start * 4 : 0xFF;
        inst->envelope_loop_start = e->loop ? e->loop_start * 4 : 0xFF;
        inst->envelope_loop_end = e->loop ? e->loop_end * 4 : 0xFF;

        int n = inst->envelope_length / 4;
        inst->envelope_data = malloc(n * sizeof(spc_envelope_node_t));
        for (int i = 0; i < n; i++) {
            spc_envelope_node_t *ed = &inst->envelope_data[i];
            ed->y = e->nodes[i].y;
            if (i != n - 1) {
                int duration = e->nodes[i + 1].x - e->nodes[i].x;
                ed->duration = duration;
                if (duration > 0) {
                    ed->delta = ((e->nodes[i + 1].y - e->nodes[i].y) * 256 + duration / 2) / duration;
                } else {
                    ed->delta = 64;
                    ed->duration = 255;
                    printf("\nsmconv: warning 'Volume envelope for instrument %i must have more\n               than one node to play properly'\n\n", g_current_instrument);
                }
            } else {
                ed->delta = 0;
                ed->duration = 0;
            }
        }
    } else {
        inst->envelope_data = NULL;
    }
    return inst;
}

void spc_instrument_destroy(spc_instrument_t *inst)
{
    if (!inst) return;
    free(inst->envelope_data);
    free(inst);
}

int spc_instrument_export_size(const spc_instrument_t *inst)
{
    /*
     * NOTE: This replicates a C++ operator precedence bug in the original:
     *   return 5 + (!EnvelopeLength) ? 0 : (3 + (EnvelopeLength/4) * 4);
     * which parses as (5 + (!EL)) ? 0 : ... and ALWAYS returns 0.
     * The totalsize reported in soundbank.h depends on this behavior.
     */
    return (5 + (!inst->envelope_length)) ? 0 : (3 + (inst->envelope_length / 4) * 4);
}

void spc_instrument_export(const spc_instrument_t *inst, io_file_t *f)
{
    io_write8(f, inst->fadeout);
    io_write8(f, inst->sample_index);
    io_write8(f, inst->global_volume);
    io_write8(f, inst->set_pan);
    io_write8(f, inst->envelope_length);
    if (inst->envelope_length) {
        io_write8(f, inst->envelope_sustain);
        io_write8(f, inst->envelope_loop_start);
        io_write8(f, inst->envelope_loop_end);
        for (int i = 0; i < inst->envelope_length / 4; i++) {
            io_write8(f, inst->envelope_data[i].y);
            io_write8(f, inst->envelope_data[i].duration);
            io_write16(f, inst->envelope_data[i].delta);
        }
    }
}

/*==========================================================================
 * Pattern
 *==========================================================================*/

spc_pattern_t *spc_pattern_create(itl_pattern_t *source)
{
    spc_pattern_t *p = calloc(1, sizeof(*p));
    p->rows = (u8)(source->rows - 1);

    u8 *read = source->data;

    if (source->data_length != 0) {
        /* Temporary row buffer */
        u8 *row_buf = NULL;
        int row_buf_size = 0;
        int row_buf_cap = 0;

        u8 spc_hints = 0;
        u8 data_bits = 0;
        u8 p_maskvar[8] = {0};

        #define ROWBUF_PUSH(val) do { \
            if (row_buf_size >= row_buf_cap) { \
                row_buf_cap = row_buf_cap ? row_buf_cap * 2 : 64; \
                row_buf = realloc(row_buf, row_buf_cap); \
            } \
            row_buf[row_buf_size++] = (val); \
        } while(0)

        for (int row = 0; row < source->rows;) {
            u8 chvar = *read++;

            if (chvar == 0) {
                pat_push(p, 0xFF); /* spc_hints placeholder — will be spc_hints */
                /* Actually, in the C++ code, 0xFF is pushed, then data_bits.
                   But looking more carefully, the spc_hints is not written to Data
                   in the original — it's discarded. Let me re-read... */
                /* Actually: Data.push_back(0xFF) then Data.push_back(data_bits).
                   The 0xFF pushed is literally the value 0xFF (not spc_hints).
                   Wait, looking at the original more carefully:
                   Data.push_back(0xFF); // spc_hints — this comment is misleading
                   No, the comment says "// spc_hints" but 0xFF is pushed as the value.

                   Actually wait — re-reading the C++ code:
                   Data.push_back(0xFF); // spc_hints
                   Hmm, but spc_hints variable is separate. Let me look at the pattern
                   more carefully. The spc_hints seems to be unused in the final output
                   except the 0xFF marker. Let me just replicate exactly. */

                /* Replace the last push — the C++ pushes 0xFF literally */
                /* Actually, I already pushed 0xFF above. Now push data_bits. */
                pat_push(p, data_bits);

                for (int i = 0; i < row_buf_size; i++)
                    pat_push(p, row_buf[i]);
                row_buf_size = 0;

                data_bits = 0;
                spc_hints = 0;
                row++;
                continue;
            }

            int channel = (chvar - 1) & 63;
            data_bits |= 1 << channel;
            if (channel > 7) {
                printf("%s: " ERRORRED("error") ": More than 8 channels. Found channel %i\n", ERRORBRIGHT("smconv"), channel + 1);
                free(row_buf);
                return p;
            }

            u8 maskvar;
            if (chvar & 128) {
                maskvar = *read++;
                maskvar = ((maskvar >> 4) | (maskvar << 4)) & 0xFF;
                maskvar |= maskvar >> 4;
                p_maskvar[channel] = maskvar;
            } else {
                maskvar = p_maskvar[channel];
            }

            ROWBUF_PUSH(maskvar);

            if (maskvar & 16) /* note */
                ROWBUF_PUSH(*read++);
            if (maskvar & 32) /* instr */
                ROWBUF_PUSH(*read++);
            if (maskvar & 64) /* vcmd */
                ROWBUF_PUSH(*read++);

            u8 cmd = 0, param = 0;
            if (maskvar & 128) { /* cmd+param */
                cmd = *read++;
                ROWBUF_PUSH(cmd);
                param = *read++;
                ROWBUF_PUSH(param);
            }

            if (maskvar & 1) {
                spc_hints |= 1 << channel;
                if (maskvar & 128) {
                    if ((cmd == 7) || (cmd == 19 && (param & 0xF0) == 0xD0)) {
                        spc_hints &= ~(1 << channel);
                    }
                }
            }
        }

        #undef ROWBUF_PUSH
        free(row_buf);
    } else {
        /* empty pattern */
        for (int i = 0; i < source->rows; i++) {
            pat_push(p, 0);
            pat_push(p, 0);
        }
    }

    return p;
}

void spc_pattern_destroy(spc_pattern_t *p)
{
    if (!p) return;
    free(p->data);
    free(p);
}

int spc_pattern_export_size(const spc_pattern_t *p)
{
    return 1 + p->data_size;
}

void spc_pattern_export(const spc_pattern_t *p, io_file_t *f)
{
    io_write8(f, p->rows);
    for (int i = 0; i < p->data_size; i++)
        io_write8(f, p->data[i]);
}

/*==========================================================================
 * Module — SNESMOD tag parsing
 *==========================================================================*/

static int search_for_snesmod_tag(const char *message)
{
    const char *test = "[[SNESMOD]]";
    int matches = 0;
    for (int i = 0; message[i]; i++) {
        if (message[i] == test[matches])
            matches++;
        else
            matches = 0;
        if (matches == 11)
            return i - 10;
    }
    return -1;
}

static int seek_next_line(const char *text, int offset)
{
    while (text[offset] != 0 && text[offset] != '\n' && text[offset] != '\r')
        offset++;
    while (text[offset] == '\n' || text[offset] == '\r')
        offset++;
    return offset;
}

static bool is_ws(char c) { return c == ' ' || c == '\t'; }
static bool not_term(char c) { return c != 0 && c != '\r' && c != '\n'; }

static int minmax(int value, int lo, int hi)
{
    value = value < lo ? lo : value;
    return value > hi ? hi : value;
}

static void parse_sm_option(spc_module_t *m, const char *text)
{
    if (!text || !text[0]) return;

    /* Parse space-separated args from one line */
    #define MAX_ARGS 16
    #define MAX_ARG_LEN 64
    char args[MAX_ARGS][MAX_ARG_LEN];
    int argc = 0;

    int offs = 0;
    while (is_ws(text[offs])) offs++;

    while (not_term(text[offs]) && argc < MAX_ARGS) {
        int len;
        for (len = 0; not_term(text[offs + len]); len++)
            if (is_ws(text[offs + len])) break;
        if (len == 0) { offs++; continue; }
        if (len >= MAX_ARG_LEN) len = MAX_ARG_LEN - 1;
        memcpy(args[argc], text + offs, len);
        args[argc][len] = '\0';
        argc++;
        offs += len;
        while (is_ws(text[offs])) offs++;
    }

    if (argc == 0) return;

    /* lowercase first arg */
    for (int i = 0; args[0][i]; i++)
        if (args[0][i] >= 'A' && args[0][i] <= 'Z')
            args[0][i] += 'a' - 'A';

    #define TESTCMD(cmd) (strcmp(args[0], cmd) == 0)

    if (TESTCMD("edl")) {
        if (argc < 2) return;
        m->echo_delay = minmax(atoi(args[1]), 0, 15);
    } else if (TESTCMD("efb")) {
        if (argc < 2) return;
        m->echo_feedback = minmax(atoi(args[1]), -128, 127);
    } else if (TESTCMD("evol")) {
        if (argc < 2) return;
        if (argc < 3) {
            m->echo_volume_l = minmax(atoi(args[1]), -128, 127);
            m->echo_volume_r = m->echo_volume_l;
        } else {
            m->echo_volume_l = minmax(atoi(args[1]), -128, 127);
            m->echo_volume_r = minmax(atoi(args[2]), -128, 127);
        }
    } else if (TESTCMD("efir")) {
        for (int i = 0; i < 8; i++) {
            if (argc <= (int)(1 + i)) return;
            m->echo_fir[i] = minmax(atoi(args[1 + i]), -128, 127);
        }
    } else if (TESTCMD("eon")) {
        for (int i = 1; i < argc; i++) {
            int c = atoi(args[i]);
            if (c >= 1 && c <= 8)
                m->echo_enable |= (1 << (c - 1));
        }
    }

    #undef TESTCMD
    #undef MAX_ARGS
    #undef MAX_ARG_LEN
}

static void parse_sm_options(spc_module_t *m, const itl_module_t *mod)
{
    m->echo_volume_l = 0;
    m->echo_volume_r = 0;
    m->echo_delay = 0;
    m->echo_feedback = 0;
    m->echo_fir[0] = 127;
    for (int i = 1; i < 8; i++) m->echo_fir[i] = 0;
    m->echo_enable = 0;

    if (mod->message) {
        int offset = search_for_snesmod_tag(mod->message);
        if (offset != -1) {
            offset = seek_next_line(mod->message, offset);
            while (mod->message[offset] != 0) {
                parse_sm_option(m, mod->message + offset);
                offset = seek_next_line(mod->message, offset);
            }
        }
    }
}

/*==========================================================================
 * Module
 *==========================================================================*/

spc_module_t *spc_module_create(const itl_module_t *mod,
                                const u16 *source_list, int source_list_count,
                                const u8 *directory, int directory_count,
                                spc_source_t **sources, int source_total)
{
    (void)source_total;
    spc_module_t *m = calloc(1, sizeof(*m));
    spc_path2id("MOD_", mod->filename, m->id, sizeof(m->id));

    m->initial_volume = mod->global_volume;
    m->initial_tempo = mod->initial_tempo;
    m->initial_speed = mod->initial_speed;

    m->source_list = malloc(source_list_count * sizeof(u16));
    memcpy(m->source_list, source_list, source_list_count * sizeof(u16));
    m->source_list_count = source_list_count;

    for (int i = 0; i < 8; i++)
        m->initial_channel_volume[i] = mod->channel_volume[i];
    for (int i = 0; i < 8; i++)
        m->initial_channel_panning[i] = mod->channel_pan[i];

    parse_sm_options(m, mod);

    for (int i = 0; i < max_length; i++)
        m->sequence[i] = i < mod->length ? mod->orders[i] : 255;

    /* Patterns */
    m->pattern_count = mod->pattern_count;
    m->patterns = malloc(m->pattern_count * sizeof(spc_pattern_t *));
    for (int i = 0; i < mod->pattern_count; i++)
        m->patterns[i] = spc_pattern_create(mod->patterns[i]);

    /* Instruments */
    m->instrument_count = mod->instrument_count;
    m->instruments = malloc(m->instrument_count * sizeof(spc_instrument_t *));
    for (int i = 0; i < mod->instrument_count; i++)
        m->instruments[i] = spc_instrument_create(mod->instruments[i]);

    /* Samples */
    m->sample_count = directory_count;
    m->samples = malloc(m->sample_count * sizeof(spc_sample_t *));
    for (int i = 0; i < directory_count; i++) {
        m->samples[i] = spc_sample_create(
            mod->samples[i],
            directory[i],
            sources[source_list[directory[i]]]->tuning_factor);
    }

    /* Calculate totalsize */
    {
        int pattsize = 0;
        for (int i = 0; i < mod->pattern_count; i++)
            pattsize += spc_pattern_export_size(m->patterns[i]);
        int sampsize = 0;
        for (int i = 0; i < source_list_count; i++)
            sampsize += sources[source_list[i]]->length;
        int instrsize = 0;
        for (int i = 0; i < m->instrument_count; i++)
            instrsize += spc_instrument_export_size(m->instruments[i]);
        int envsize = 0;
        for (int i = 0; i < m->instrument_count; i++)
            envsize += spc_instrument_export_size(m->instruments[i]);

        u32 echosize = m->echo_delay * 2048;
        m->totalsize = pattsize + sampsize + instrsize + envsize + echosize;
        spc_ram_size_g = 65535 - module_base - header_size;
        u32 bytesfree = spc_ram_size_g - m->totalsize - header_size;
        totabanksize += m->totalsize;

        if (g_verbose) {
            if (g_chksfx && (totalsizem1 != 0)) {
                printf(
                    "Conversion report:\n"
                    "    Pattern data: [%5i bytes]   Module Length: [%i/%i]\n"
                    "     Sample data: [%5i bytes]        Patterns: [%i/%i]\n"
                    " Instrument data: [%5i bytes]     Instruments: [%i/%i]\n"
                    "   Envelope data: [%5i bytes]         Samples: [%i/%i]\n"
                    "     Echo region: [%5i bytes]\n"
                    "           Total: [%5i bytes]   *%i bytes free* *%i bytes free with 1st module*\n",
                    pattsize, mod->length, max_length,
                    sampsize, mod->pattern_count, max_patterns,
                    instrsize, mod->instrument_count, max_instruments,
                    envsize, mod->sample_count, max_samples,
                    echosize,
                    m->totalsize, bytesfree, bytesfree - totalsizem1);
            } else {
                printf(
                    "Conversion report:\n"
                    "    Pattern data: [%5i bytes]   Module Length: [%i/%i]\n"
                    "     Sample data: [%5i bytes]        Patterns: [%i/%i]\n"
                    " Instrument data: [%5i bytes]     Instruments: [%i/%i]\n"
                    "   Envelope data: [%5i bytes]         Samples: [%i/%i]\n"
                    "     Echo region: [%5i bytes]\n"
                    "           Total: [%5i bytes]   *%i bytes free*\n",
                    pattsize, mod->length, max_length,
                    sampsize, mod->pattern_count, max_patterns,
                    instrsize, mod->instrument_count, max_instruments,
                    envsize, mod->sample_count, max_samples,
                    echosize,
                    m->totalsize, bytesfree);
            }

            if (m->totalsize > spc_ram_size_g)
                printf("%s: " ERRORRED("error") ": Module is too big. Maximum is %i bytes\n", ERRORBRIGHT("smconv"), spc_ram_size_g);
        }

        if (totalsizem1 == 0)
            totalsizem1 = m->totalsize;
    }

    return m;
}

void spc_module_destroy(spc_module_t *m)
{
    if (!m) return;
    for (int i = 0; i < m->pattern_count; i++)
        spc_pattern_destroy(m->patterns[i]);
    free(m->patterns);
    for (int i = 0; i < m->instrument_count; i++)
        spc_instrument_destroy(m->instruments[i]);
    free(m->instruments);
    for (int i = 0; i < m->sample_count; i++)
        spc_sample_destroy(m->samples[i]);
    free(m->samples);
    free(m->source_list);
    free(m);
}

void spc_module_export(const spc_module_t *m, io_file_t *f, bool write_header)
{
    int header_start = io_tell(f);

    if (write_header) {
        io_write16(f, 0xaaaa); /* reserve for module size */
        io_write16(f, m->source_list_count);
        for (int i = 0; i < m->source_list_count; i++)
            io_write16(f, m->source_list[i]);
    }

    int module_start = io_tell(f);

    io_write8(f, m->initial_volume);
    io_write8(f, m->initial_tempo);
    io_write8(f, m->initial_speed);

    for (int i = 0; i < 8; i++)
        io_write8(f, m->initial_channel_volume[i]);
    for (int i = 0; i < 8; i++)
        io_write8(f, m->initial_channel_panning[i]);

    io_write8(f, m->echo_volume_l);
    io_write8(f, m->echo_volume_r);
    io_write8(f, m->echo_delay);
    io_write8(f, m->echo_feedback);

    for (int i = 0; i < 8; i++)
        io_write8(f, m->echo_fir[i]);

    io_write8(f, m->echo_enable);

    for (int i = 0; i < max_length; i++)
        io_write8(f, m->sequence[i]);

    u32 start_of_tables = io_tell(f);

    /* reserve space for pointers */
    for (int i = 0; i < (max_patterns + max_instruments + max_samples); i++)
        io_write16(f, 0xBAAA);

    u16 *pattern_ptr = calloc(m->pattern_count, sizeof(u16));
    u16 *instrument_ptr = calloc(m->instrument_count, sizeof(u16));
    u16 *sample_ptr = calloc(m->sample_count, sizeof(u16));

    for (int i = 0; i < m->pattern_count; i++) {
        u32 ptr = io_tell(f) - module_start;
        if (ptr > SPC_RAM_SIZE)
            printf("\nERROR: Module is too big.");
        pattern_ptr[i] = (u16)ptr;
        spc_pattern_export(m->patterns[i], f);
    }

    for (int i = 0; i < m->instrument_count; i++) {
        u32 ptr = io_tell(f) - module_start;
        if (ptr > SPC_RAM_SIZE)
            printf("\nERROR: Module is too big.");
        instrument_ptr[i] = (u16)ptr;
        spc_instrument_export(m->instruments[i], f);
    }

    for (int i = 0; i < m->sample_count; i++) {
        u32 ptr = io_tell(f) - module_start;
        if (ptr > SPC_RAM_SIZE)
            printf("\nERROR: Module is too big.");
        sample_ptr[i] = (u16)ptr;
        spc_sample_export(m->samples[i], f);
    }

    u32 end_of_mod = io_tell(f);

    if (write_header) {
        io_seek(f, header_start);
        io_write16(f, (end_of_mod - module_start + 1) >> 1);
    }

    io_seek(f, start_of_tables);

    for (int i = 0; i < max_patterns; i++)
        io_write8(f, i < m->pattern_count ? ((pattern_ptr[i] + MODULE_BASE) & 0xFF) : 0xFF);
    for (int i = 0; i < max_patterns; i++)
        io_write8(f, i < m->pattern_count ? ((pattern_ptr[i] + MODULE_BASE) >> 8) : 0xFF);

    for (int i = 0; i < max_instruments; i++)
        io_write8(f, i < m->instrument_count ? ((instrument_ptr[i] + MODULE_BASE) & 0xFF) : 0xFF);
    for (int i = 0; i < max_instruments; i++)
        io_write8(f, i < m->instrument_count ? ((instrument_ptr[i] + MODULE_BASE) >> 8) : 0xFF);

    for (int i = 0; i < max_samples; i++)
        io_write8(f, i < m->sample_count ? ((sample_ptr[i] + MODULE_BASE) & 0xFF) : 0xFF);
    for (int i = 0; i < max_samples; i++)
        io_write8(f, i < m->sample_count ? ((sample_ptr[i] + MODULE_BASE) >> 8) : 0xFF);

    free(pattern_ptr);
    free(instrument_ptr);
    free(sample_ptr);

    io_seek(f, end_of_mod);

    if (write_header) {
        if (io_tell(f) & 1)
            io_write8(f, 0); /* align by 2 */
    }
}

/*==========================================================================
 * Bank
 *==========================================================================*/

static int bank_add_source(spc_bank_t *b, spc_source_t *s)
{
    /* Check for duplicate */
    for (int i = 0; i < b->source_count; i++) {
        if (spc_source_compare(b->sources[i], s)) {
            spc_source_destroy(s);
            return i;
        }
    }
    b->sources = realloc(b->sources, (b->source_count + 1) * sizeof(spc_source_t *));
    b->sources[b->source_count] = s;
    return b->source_count++;
}

static void bank_add_module(spc_bank_t *b, const itl_module_t *mod)
{
    int size = io_file_size(mod->filename);

    if (g_verbose) {
        printf("-----------------------------------------------------------------------\n");
        printf("Adding module, Filename: <%s>\n", mod->filename);
        printf("             Title: <%s>\n", mod->title);
        printf("             IT Size: [%5i bytes]\n", size);
        fflush(stdout);
    }
    totalitsize += size;

    u16 *source_list = calloc(mod->sample_count, sizeof(u16));
    int source_list_count = 0;
    u8 *directory = calloc(mod->sample_count, sizeof(u8));

    for (int i = 0; i < mod->sample_count; i++) {
        spc_source_t *s = spc_source_create(&mod->samples[i]->data);

        /* Set source ID from sample name */
        if (mod->samples[i]->name[0] != '\0')
            spc_path2id("SFX_", mod->samples[i]->name, s->id, sizeof(s->id));

        int index = bank_add_source(b, s);
        bool exists = false;

        for (int j = 0; j < source_list_count; j++) {
            if (source_list[j] == (u16)index) {
                directory[i] = j;
                exists = true;
                break;
            }
        }

        if (!exists) {
            directory[i] = source_list_count;
            source_list[source_list_count++] = index;
        }
    }

    spc_module_t *m = spc_module_create(mod, source_list, source_list_count,
                                         directory, mod->sample_count,
                                         b->sources, b->source_count);

    b->modules = realloc(b->modules, (b->module_count + 1) * sizeof(spc_module_t *));
    b->modules[b->module_count++] = m;

    free(source_list);
    free(directory);
}

spc_bank_t *spc_bank_create(const itl_bank_t *bank, bool hirom, bool chksfx)
{
    spc_bank_t *b = calloc(1, sizeof(*b));
    b->hirom = hirom;
    g_chksfx = chksfx;

    /* Reset globals for fresh conversion */
    g_current_instrument = 0;
    totalsizem1 = 0;
    totalitsize = 0;
    totabanksize = 0;

    spc_program_size = sizeof(spc_program);

    for (int i = 0; i < bank->module_count; i++)
        bank_add_module(b, bank->modules[i]);

    if (g_verbose) {
        printf("-----------------------------------------------------------------------\n");
        printf("  Total Modules Size: [%6i bytes]\n", totabanksize);
        printf("       Total IT Size: [%6i bytes]\n", totalitsize);
        fflush(stdout);
    }

    return b;
}

void spc_bank_destroy(spc_bank_t *b)
{
    if (!b) return;
    for (int i = 0; i < b->module_count; i++)
        spc_module_destroy(b->modules[i]);
    free(b->modules);
    for (int i = 0; i < b->source_count; i++)
        spc_source_destroy(b->sources[i]);
    free(b->sources);
    free(b);
}

/*==========================================================================
 * Export
 *==========================================================================*/

void spc_bank_make_spc(const spc_bank_t *b, const char *spcfile)
{
    io_file_t f;
    io_init(&f);
    io_open(&f, spcfile, IO_MODE_WRITE);

    io_write_ascii(&f, "SNES-SPC700 Sound File Data v0.30");
    io_write8(&f, 26);
    io_write8(&f, 26);
    io_write8(&f, 26);
    io_write8(&f, 30);

    /* SPC700 registers */
    io_write16(&f, driver_base);
    io_write8(&f, 0);    /* A */
    io_write8(&f, 0);    /* X */
    io_write8(&f, 0);    /* Y */
    io_write8(&f, 0);    /* PSW */
    io_write8(&f, 0xEF); /* SP */
    io_write16(&f, 0);   /* reserved */

    /* ID666 tag */
    io_write_asciif(&f, "<INSERT SONG TITLE>", 32);
    io_write_asciif(&f, "<INSERT GAME TITLE>", 32);
    io_write_asciif(&f, "NAME OF DUMPER", 16);
    io_write_asciif(&f, "comments...", 32);
    io_write_asciif(&f, "", 11);
    io_write_asciif(&f, "180", 3);
    io_write_asciif(&f, "5000", 5);
    io_write_asciif(&f, "<INSERT SONG ARTIST>", 32);
    io_write8(&f, 0);
    io_write8(&f, '0');
    io_zero_fill(&f, 45);

    /* SPC memory */
    io_zero_fill(&f, driver_base);

    int sample_table_pos = io_tell(&f) - sample_table_offset;

    for (int i = 0; i < spc_program_size; i++) {
        if (i == patch_byte || i == patch_byte + 1)
            io_write8(&f, 0);
        else
            io_write8(&f, spc_program[i]);
    }

    io_zero_fill(&f, module_base - driver_base - spc_program_size);

    int start_of_module = io_tell(&f);
    spc_module_export(b->modules[0], &f, false);

    u16 source_table[128];
    memset(source_table, 0, sizeof(source_table));

    for (int i = 0; i < b->modules[0]->source_list_count; i++) {
        source_table[i * 2 + 0] = io_tell(&f) - start_of_module + MODULE_BASE;
        source_table[i * 2 + 1] = source_table[i * 2] + b->sources[b->modules[0]->source_list[i]]->loop;
        spc_source_export(b->sources[b->modules[0]->source_list[i]], &f, true);
    }

    int end_of_data = io_tell(&f);
    io_seek(&f, sample_table_pos);
    for (int i = 0; i < 128; i++)
        io_write16(&f, source_table[i]);

    io_seek(&f, end_of_data);
    io_zero_fill(&f, 65792 - end_of_data);

    /* DSP registers */
    for (int i = 0; i < 128; i++) io_write8(&f, 0);
    /* unused/ipl rom */
    for (int i = 0; i < 128; i++) io_write8(&f, 0);

    io_close(&f);
}

static void export_asm(const spc_bank_t *b, const char *inputfile, const char *outputfile)
{
    FILE *fp = fopen(outputfile, "w");
    int size = io_file_size(inputfile);

    fprintf(fp,
            ";************************************************\n"
            "; snesmod soundbank data                        *\n"
            "; total size: %10i bytes                  *\n"
            ";************************************************\n\n",
            size);

    if (!g_no_header)
        fprintf(fp, ".include \"hdr.asm\"\n\n");

    int banksize = b->hirom ? 65536 : 32768;

    if (size <= banksize) {
        fprintf(fp,
                ".BANK %i\n"
                ".SECTION \"SOUNDBANK\" ; need dedicated bank(s)\n\n"
                "%s:\n",
                g_banknum, g_symbol_prefix);

        /* Normalize backslashes */
        char foo[1024];
        strncpy(foo, inputfile, sizeof(foo) - 1);
        foo[sizeof(foo) - 1] = '\0';
        for (int i = 0; foo[i]; i++)
            if (foo[i] == '\\') foo[i] = '/';

        fprintf(fp, ".incbin \"%s\"\n", foo);
        fprintf(fp, ".ENDS\n");
    } else {
        u32 lastbank = size / banksize;
        for (u32 j = 0; j <= lastbank; j++) {
            fprintf(fp,
                    ".BANK %i\n"
                    ".SECTION \"SOUNDBANK%i\" ; need dedicated bank(s)\n\n"
                    "%s%i:\n",
                    g_banknum + (int)j, (int)j, g_symbol_prefix, (int)j);

            char foo[1024];
            strncpy(foo, inputfile, sizeof(foo) - 1);
            foo[sizeof(foo) - 1] = '\0';
            for (int i = 0; foo[i]; i++)
                if (foo[i] == '\\') foo[i] = '/';

            if (b->hirom) {
                if (j == 0)
                    fprintf(fp, ".incbin \"%s\" read $10000\n", foo);
                else if (j == lastbank)
                    fprintf(fp, ".incbin \"%s\" skip $%x\n", foo, j * 0x10000);
                else
                    fprintf(fp, ".incbin \"%s\" skip $%x read $10000\n", foo, j * 0x10000);
            } else {
                if (j == 0)
                    fprintf(fp, ".incbin \"%s\" read $8000\n", foo);
                else if (j == lastbank)
                    fprintf(fp, ".incbin \"%s\" skip $%x\n", foo, j * 0x8000);
                else
                    fprintf(fp, ".incbin \"%s\" skip $%x read $8000\n", foo, j * 0x8000);
            }

            fprintf(fp, ".ENDS\n\n");
        }
    }

    fclose(fp);
}

static void export_inc(const spc_bank_t *b, const char *output)
{
    FILE *fp = fopen(output, "w");

    fprintf(fp,
            "// snesmod soundbank definitions\n\n"
            "#ifndef __SOUNDBANK_DEFINITIONS__\n"
            "#define __SOUNDBANK_DEFINITIONS__\n\n");

    for (int i = 0; i < b->module_count; i++) {
        if (b->modules[i]->id[0] != '\0') {
            fprintf(fp, "#define %-32s\t%i\n", b->modules[i]->id, i);
            char size_id[512];
            snprintf(size_id, sizeof(size_id), "%s_SIZE", b->modules[i]->id);
            fprintf(fp, "#define %-32s\t%i\n", size_id, b->modules[i]->totalsize);
        }
    }
    fprintf(fp, "\n");

    for (int i = 0; i < b->source_count; i++) {
        if (b->sources[i]->id[0] != '\0')
            fprintf(fp, "#define %-32s\t%i\n", b->sources[i]->id, i);
    }

    fprintf(fp, "\n#endif // __SOUNDBANK_DEFINITIONS__\n");
    fclose(fp);
}

void spc_bank_export(const spc_bank_t *b, const char *output,
                     bool no_header, const char *symbol_prefix, int banknum)
{
    g_no_header = no_header;
    g_symbol_prefix = symbol_prefix;
    g_banknum = banknum;

    char bin_output[1024];
    snprintf(bin_output, sizeof(bin_output), "%s.bnk", output);

    io_file_t f;
    io_init(&f);
    io_open(&f, bin_output, IO_MODE_WRITE);

    io_write16(&f, b->source_count);
    io_write16(&f, b->module_count);

    /* reserve space for tables */
    for (int i = 0; i < 3 * 128; i++)
        io_write8(&f, 0xAA);
    for (int i = 0; i < b->source_count * 3; i++)
        io_write8(&f, 0xAA);

    u32 *module_ptr = calloc(b->module_count, sizeof(u32));
    u32 *source_ptr = calloc(b->source_count, sizeof(u32));

    for (int i = 0; i < b->module_count; i++) {
        module_ptr[i] = io_tell(&f);
        spc_module_export(b->modules[i], &f, true);
    }

    for (int i = 0; i < b->source_count; i++) {
        source_ptr[i] = io_tell(&f);
        spc_source_export(b->sources[i], &f, false);
    }

    /* export module pointers */
    io_seek(&f, 4);
    for (int i = 0; i < 128; i++) {
        int addr, bank;
        if (i < b->module_count) {
            if (b->hirom) {
                addr = module_ptr[i] & 65535;
                bank = module_ptr[i] >> 16;
            } else {
                addr = 0x8000 + (module_ptr[i] & 32767);
                bank = module_ptr[i] >> 15;
            }
        } else {
            bank = addr = 0;
        }
        io_write16(&f, addr);
        io_write8(&f, bank);
    }

    /* export source pointers */
    for (int i = 0; i < b->source_count; i++) {
        int addr, bank;
        if (b->hirom) {
            addr = source_ptr[i] & 65535;
            bank = source_ptr[i] >> 16;
        } else {
            addr = 0x8000 + (source_ptr[i] & 32767);
            bank = source_ptr[i] >> 15;
        }
        io_write16(&f, addr);
        io_write8(&f, bank);
    }

    free(module_ptr);
    free(source_ptr);
    io_close(&f);

    char asm_out[1024];
    snprintf(asm_out, sizeof(asm_out), "%s.asm", output);
    export_asm(b, bin_output, asm_out);

    char inc_out[1024];
    snprintf(inc_out, sizeof(inc_out), "%s.h", output);
    export_inc(b, inc_out);
}

void spc_bank_set_verbose(bool v)
{
    g_verbose = v;
}
