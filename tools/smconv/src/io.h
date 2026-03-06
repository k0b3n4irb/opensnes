#ifndef IO_H
#define IO_H

#include <stdbool.h>
#include <stdio.h>
#include "basetypes.h"

typedef enum {
    IO_MODE_READ,
    IO_MODE_WRITE
} io_mode_t;

typedef struct {
    FILE *fp;
    bool is_open;
    io_mode_t mode;
} io_file_t;

void io_init(io_file_t *f);
bool io_open(io_file_t *f, const char *filename, io_mode_t mode);
void io_close(io_file_t *f);

u8  io_read8(io_file_t *f);
u16 io_read16(io_file_t *f);
u32 io_read32(io_file_t *f);

void io_write8(io_file_t *f, u8 data);
void io_write16(io_file_t *f, u16 data);
void io_write32(io_file_t *f, u32 data);

void io_write_ascii(io_file_t *f, const char *str);
void io_write_asciif(io_file_t *f, const char *str, int length);
void io_zero_fill(io_file_t *f, int amount);
void io_write_align(io_file_t *f, u32 boundary);

void io_skip(io_file_t *f, int amount);
u32  io_tell(io_file_t *f);
void io_seek(io_file_t *f, u32 offset);

bool io_file_exists(const char *filename);
u32  io_file_size(const char *filename);

#endif
