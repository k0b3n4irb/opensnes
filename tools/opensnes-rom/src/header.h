/* header.h — the cartridge header of a built ROM, its checksum, and the
 * two digests a publisher or a ROM database asks for. */
#ifndef OPENSNES_ROM_HEADER_H
#define OPENSNES_ROM_HEADER_H

#include <stddef.h>

typedef struct {
    size_t   offset;            /* file offset of $FFC0 (the title) */
    size_t   copier;            /* 512 when a copier header precedes the image, else 0 */
    char     title[22];
    unsigned map, type, rom_size, ram_size, country, developer, version;
    unsigned checksum, complement, computed;
    const char *mapping;        /* "LoROM", "HiROM", "ExHiROM" */
    int      fast;
    const char *chip;           /* NULL = none */
    int      has_ram, has_battery;
    const char *region;         /* "NTSC", "PAL", "unknown" */
    long     declared_bytes;    /* 1024 << rom_size */
    long     ram_bytes;         /* save RAM (the expansion RAM byte for a Super FX cartridge) */
    int      bank_size;         /* 0x8000 or 0x10000 */
    unsigned reset;             /* the emulation-mode reset vector */
} rom_header;

/* 0 and *h filled, or -1 with the reason in why[]. rom/len: the whole file. */
int rom_header_read(const unsigned char *rom, size_t len, rom_header *h, char *why, size_t why_len);

unsigned rom_crc32(const unsigned char *p, size_t n);
void     rom_sha1(const unsigned char *p, size_t n, char hex[41]);

#endif
