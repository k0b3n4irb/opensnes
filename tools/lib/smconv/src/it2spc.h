#ifndef IT2SPC_H
#define IT2SPC_H

#include <stdbool.h>
#include "basetypes.h"
#include "io.h"
#include "itloader.h"

/*--- Source (BRR-compressed sample) ---*/
typedef struct {
    u16 length;
    u16 loop;
    u8 *data;
    double tuning_factor;
    char id[256];
} spc_source_t;

spc_source_t *spc_source_create(const itl_sample_data_t *src);
void spc_source_destroy(spc_source_t *s);
bool spc_source_compare(const spc_source_t *a, const spc_source_t *b);
void spc_source_export(const spc_source_t *s, io_file_t *f, bool spc_direct);

/*--- Sample ---*/
typedef struct {
    u8  default_volume;
    u8  global_volume;
    u16 pitch_base;
    u8  directory_index;
    u8  set_pan;
} spc_sample_t;

spc_sample_t *spc_sample_create(const itl_sample_t *src, int directory, double tuning);
void spc_sample_destroy(spc_sample_t *s);
void spc_sample_export(const spc_sample_t *s, io_file_t *f);

/*--- Envelope Node ---*/
typedef struct {
    u8  y;
    u8  duration;
    s16 delta;
} spc_envelope_node_t;

/*--- Instrument ---*/
typedef struct {
    u8  fadeout;
    u8  sample_index;
    u8  global_volume;
    u8  set_pan;
    u8  envelope_length;
    u8  envelope_sustain;
    u8  envelope_loop_start;
    u8  envelope_loop_end;
    spc_envelope_node_t *envelope_data;
} spc_instrument_t;

spc_instrument_t *spc_instrument_create(const itl_instrument_t *src);
void spc_instrument_destroy(spc_instrument_t *inst);
int  spc_instrument_export_size(const spc_instrument_t *inst);
void spc_instrument_export(const spc_instrument_t *inst, io_file_t *f);

/*--- Pattern ---*/
typedef struct {
    u8  rows;
    u8 *data;
    int data_size;
    int data_cap;
} spc_pattern_t;

spc_pattern_t *spc_pattern_create(itl_pattern_t *src);
void spc_pattern_destroy(spc_pattern_t *p);
int  spc_pattern_export_size(const spc_pattern_t *p);
void spc_pattern_export(const spc_pattern_t *p, io_file_t *f);

/*--- Module ---*/
typedef struct {
    u8  initial_volume;
    u8  initial_tempo;
    u8  initial_speed;
    u8  initial_channel_volume[8];
    u8  initial_channel_panning[8];
    u8  echo_volume_l;
    u8  echo_volume_r;
    u8  echo_delay;
    u8  echo_feedback;
    u8  echo_fir[8];
    u8  echo_enable;
    u8  sequence[200];

    spc_pattern_t **patterns;
    int pattern_count;
    spc_instrument_t **instruments;
    int instrument_count;
    spc_sample_t **samples;
    int sample_count;

    char id[256];
    u16 *source_list;
    int source_list_count;
    u32 totalsize;
} spc_module_t;

spc_module_t *spc_module_create(const itl_module_t *mod,
                                const u16 *source_list, int source_list_count,
                                const u8 *directory, int directory_count,
                                spc_source_t **sources, int source_total);
void spc_module_destroy(spc_module_t *m);
void spc_module_export(const spc_module_t *m, io_file_t *f, bool write_header);

/*--- Bank ---*/
typedef struct {
    spc_module_t **modules;
    int module_count;
    spc_source_t **sources;
    int source_count;
    bool hirom;
} spc_bank_t;

spc_bank_t *spc_bank_create(const itl_bank_t *bank, bool hirom, bool chksfx);
void spc_bank_destroy(spc_bank_t *b);
void spc_bank_export(const spc_bank_t *b, const char *output,
                     bool no_header, const char *symbol_prefix, int banknum);
void spc_bank_make_spc(const spc_bank_t *b, const char *spcfile);
void spc_bank_set_verbose(bool v);

/*--- Helpers ---*/
void spc_path2id(const char *prefix, const char *source, char *out, int out_size);
void spc_validate_id(char *id);

#endif
