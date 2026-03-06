#include "io.h"

void io_init(io_file_t *f)
{
    f->fp = NULL;
    f->is_open = false;
    f->mode = IO_MODE_READ;
}

bool io_open(io_file_t *f, const char *filename, io_mode_t mode)
{
    if (f->is_open)
        return false;
    f->mode = mode;
    f->fp = fopen(filename, mode == IO_MODE_WRITE ? "wb" : "rb");
    f->is_open = f->fp != NULL;
    return f->is_open;
}

void io_close(io_file_t *f)
{
    if (f->is_open) {
        fclose(f->fp);
        f->fp = NULL;
        f->is_open = false;
    }
}

u8 io_read8(io_file_t *f)
{
    if (f->mode == IO_MODE_WRITE || !f->is_open)
        return 0;
    u8 a;
    fread(&a, 1, 1, f->fp);
    return a;
}

u16 io_read16(io_file_t *f)
{
    u16 a = io_read8(f);
    a |= (u16)io_read8(f) << 8;
    return a;
}

u32 io_read32(io_file_t *f)
{
    u32 a = io_read16(f);
    a |= (u32)io_read16(f) << 16;
    return a;
}

void io_skip(io_file_t *f, int amount)
{
    if (!f->is_open)
        return;
    if (f->mode == IO_MODE_WRITE) {
        for (int i = 0; i < amount; i++)
            io_write8(f, 0);
    } else {
        fseek(f->fp, amount, SEEK_CUR);
    }
}

void io_write8(io_file_t *f, u8 data)
{
    if (f->is_open && f->mode == IO_MODE_WRITE)
        fwrite(&data, 1, 1, f->fp);
}

void io_write16(io_file_t *f, u16 data)
{
    io_write8(f, data & 0xFF);
    io_write8(f, data >> 8);
}

void io_write32(io_file_t *f, u32 data)
{
    io_write16(f, data & 0xFFFF);
    io_write16(f, data >> 16);
}

void io_write_ascii(io_file_t *f, const char *str)
{
    for (int i = 0; str[i]; i++)
        io_write8(f, str[i]);
}

void io_write_asciif(io_file_t *f, const char *str, int length)
{
    int i;
    for (i = 0; str[i] && i < length; i++)
        io_write8(f, str[i]);
    for (; i < length; i++)
        io_write8(f, 0);
}

void io_zero_fill(io_file_t *f, int amount)
{
    for (int i = 0; i < amount; i++)
        io_write8(f, 0);
}

void io_write_align(io_file_t *f, u32 boundary)
{
    int skip = io_tell(f) % boundary;
    if (skip)
        io_skip(f, boundary - skip);
}

void io_seek(io_file_t *f, u32 offset)
{
    if (f->is_open)
        fseek(f->fp, offset, SEEK_SET);
}

u32 io_tell(io_file_t *f)
{
    if (f->is_open)
        return ftell(f->fp);
    return 0;
}

bool io_file_exists(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp) {
        fclose(fp);
        return true;
    }
    return false;
}

u32 io_file_size(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
        return 0;
    fseek(fp, 0, SEEK_END);
    u32 size = ftell(fp);
    fclose(fp);
    return size;
}
