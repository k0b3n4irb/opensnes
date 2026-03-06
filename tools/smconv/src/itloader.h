#ifndef ITLOADER_H
#define ITLOADER_H

#include <stdbool.h>
#include "basetypes.h"
#include "io.h"

/*--- Pattern ---*/
typedef struct {
    u16 data_length;
    u16 rows;
    u8 *data;
} itl_pattern_t;

itl_pattern_t *itl_pattern_create(io_file_t *f);
itl_pattern_t *itl_pattern_create_empty(void);
void itl_pattern_destroy(itl_pattern_t *p);

/*--- SampleData ---*/
typedef struct {
    bool bits16;
    int length;
    int loop_start;
    int loop_end;
    int c5speed;
    int sustain_start;
    int sustain_end;
    bool loop;
    bool sustain;
    bool bidi_loop;
    bool bidi_sustain;
    union {
        s8  *data8;
        s16 *data16;
    };
} itl_sample_data_t;

void itl_sample_data_init(itl_sample_data_t *sd);
itl_sample_data_t *itl_sample_data_from_wav(const char *filename);

/*--- Envelope ---*/
typedef struct {
    u8  y;
    u16 x;
} itl_envelope_entry_t;

typedef struct {
    bool enabled;
    bool loop;
    bool sustain;
    bool is_filter;
    int length;
    int loop_start;
    int loop_end;
    int sustain_start;
    int sustain_end;
    itl_envelope_entry_t nodes[25];
} itl_envelope_t;

itl_envelope_t *itl_envelope_create(io_file_t *f);
void itl_envelope_destroy(itl_envelope_t *e);

/*--- Instrument ---*/
typedef struct {
    u8  note;
    u8  sample;
} itl_notemap_entry_t;

typedef struct {
    char name[27];
    char dos_filename[13];
    u8  new_note_action;
    u8  duplicate_check_type;
    u8  duplicate_check_action;
    u16 fadeout;
    u8  pps;
    u8  ppc;
    u8  global_volume;
    u8  default_pan;
    u8  random_volume;
    u8  random_panning;
    u16 tracker_version;
    u8  number_of_samples;
    u8  initial_filter_cutoff;
    u8  initial_filter_resonance;
    u8  midi_channel;
    u8  midi_program;
    u16 midi_bank;
    itl_notemap_entry_t notemap[120];
    itl_envelope_t *volume_envelope;
    itl_envelope_t *panning_envelope;
    itl_envelope_t *pitch_envelope;
} itl_instrument_t;

itl_instrument_t *itl_instrument_create(io_file_t *f);
void itl_instrument_destroy(itl_instrument_t *inst);

/*--- Sample ---*/
typedef struct {
    char name[27];
    char dos_filename[13];
    u8  global_volume;
    bool has_sample;
    bool stereo;
    bool compressed;
    u8  default_volume;
    u8  default_panning;
    u8  convert;
    u8  vibrato_speed;
    u8  vibrato_depth;
    u8  vibrato_form;
    u8  vibrato_rate;
    itl_sample_data_t data;
} itl_sample_t;

itl_sample_t *itl_sample_create(io_file_t *f);
void itl_sample_destroy(itl_sample_t *s);

/*--- Module ---*/
typedef struct {
    char filename[1024];
    char title[26];
    u16 pattern_highlight;
    u16 length;
    u16 instrument_count;
    u16 sample_count;
    u16 pattern_count;
    u16 cwtv;
    u16 cmwt;
    int flags;
    int special;
    u8  global_volume;
    u8  mixing_volume;
    u8  initial_speed;
    u8  initial_tempo;
    u8  sep;
    u8  pwd;
    u16 message_length;
    char *message;
    int channel_pan[64];
    int channel_volume[64];
    int orders[256];
    itl_instrument_t **instruments;
    itl_sample_t **samples;
    itl_pattern_t **patterns;
} itl_module_t;

itl_module_t *itl_module_create(const char *filename);
void itl_module_destroy(itl_module_t *m);

/*--- Bank ---*/
typedef struct {
    itl_module_t **modules;
    int module_count;
    itl_sample_data_t **sounds;
    int sound_count;
} itl_bank_t;

itl_bank_t *itl_bank_create(const char **files, int file_count);
void itl_bank_destroy(itl_bank_t *b);

#endif
