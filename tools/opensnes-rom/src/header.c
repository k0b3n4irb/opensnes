/* header.c — find and read the cartridge header ($FFC0-$FFDF and the
 * vectors after it), recompute the checksum the way the console's
 * licensing tools did, and digest the image. */
#include "header.h"

#include <stdio.h>
#include <string.h>

static unsigned rd16(const unsigned char *p) { return (unsigned)p[0] | ((unsigned)p[1] << 8); }

/* The 16-bit sum of the image. An image that is not a power of two is
 * summed as its largest power-of-two part plus the rest repeated to fill
 * the same length again (a 3 Mbit image is 2 Mbit + the last Mbit twice). */
static unsigned checksum(const unsigned char *rom, size_t len)
{
    size_t pow = 1;
    while (pow * 2 <= len) pow *= 2;
    unsigned long sum = 0;
    for (size_t i = 0; i < pow; i++) sum += rom[i];
    size_t rest = len - pow;
    if (rest) {
        unsigned long part = 0;
        for (size_t i = 0; i < rest; i++) part += rom[pow + i];
        size_t times = pow / rest;
        sum += part * times;
        for (size_t i = 0; i < pow % rest; i++) sum += rom[pow + i];
    }
    return (unsigned)(sum & 0xFFFF);
}

/* How much a header at `off` looks like one for the mapping `want`. */
static int score_at(const unsigned char *rom, size_t len, size_t off, unsigned want)
{
    if (off + 0x40 > len) return -1;
    const unsigned char *h = rom + off;
    int score = 0;
    if (((rd16(h + 0x1C) + rd16(h + 0x1E)) & 0xFFFF) == 0xFFFF && rd16(h + 0x1C) != rd16(h + 0x1E)) score += 4;
    unsigned map = h[0x15] & 0x0F;
    if ((h[0x15] & 0xE0) == 0x20) score += 2;
    if (want == 0 && (map == 0 || map == 2 || map == 3)) score += 2;
    if (want == 1 && map == 1) score += 2;
    if (want == 5 && map == 5) score += 2;
    if (rd16(h + 0x3C) >= 0x8000) score += 2;       /* reset vector points into ROM */
    int printable = 1;
    for (int i = 0; i < 21; i++) if (h[i] < 0x20 || h[i] > 0x7E) printable = 0;
    if (printable) score += 2;
    if (h[0x17] >= 7 && h[0x17] <= 13) score += 1;  /* 128 KB to 8 MB */
    return score;
}

static const char *chip_name(unsigned type)
{
    if ((type & 0x0F) < 3) return NULL;
    switch (type >> 4) {
    case 0x0: return "DSP";
    case 0x1: return "Super FX";
    case 0x2: return "OBC1";
    case 0x3: return "SA-1";
    case 0x4: return "S-DD1";
    case 0x5: return "S-RTC";
    case 0xE: return "other";
    case 0xF: return "custom";
    default:  return "unknown";
    }
}

static const char *region_of(unsigned country)
{
    switch (country) {
    case 0x00: case 0x01: case 0x0D: case 0x0F: case 0x10: return "NTSC";
    case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08:
    case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x11: return "PAL";
    default: return "unknown";
    }
}

int rom_header_read(const unsigned char *file, size_t flen, rom_header *h, char *why, size_t why_len)
{
    memset(h, 0, sizeof *h);
    h->copier = (flen % 1024 == 512) ? 512 : 0;
    const unsigned char *rom = file + h->copier;
    size_t len = flen - h->copier;
    if (len < 0x8000) { snprintf(why, why_len, "%lu bytes: too short to hold a cartridge header", (unsigned long)flen); return -1; }

    static const struct { size_t off; unsigned map; const char *name; int bank; } cand[] = {
        { 0x7FC0, 0, "LoROM", 0x8000 }, { 0xFFC0, 1, "HiROM", 0x10000 }, { 0x40FFC0, 5, "ExHiROM", 0x10000 },
    };
    int best = -1, best_score = -1;
    for (int i = 0; i < 3; i++) {
        int s = score_at(rom, len, cand[i].off, cand[i].map);
        if (s > best_score) { best_score = s; best = i; }
    }
    if (best < 0 || best_score < 7) {
        snprintf(why, why_len, "no cartridge header at $7FC0, $FFC0 or $40FFC0 (not a SNES ROM, or its header is damaged)");
        return -1;
    }
    const unsigned char *p = rom + cand[best].off;
    h->offset = cand[best].off + h->copier;
    memcpy(h->title, p, 21);
    h->title[21] = '\0';
    for (int i = 20; i >= 0 && h->title[i] == ' '; i--) h->title[i] = '\0';
    for (int i = 0; h->title[i]; i++) if ((unsigned char)h->title[i] < 0x20 || (unsigned char)h->title[i] > 0x7E) h->title[i] = '?';
    h->map = p[0x15]; h->type = p[0x16]; h->rom_size = p[0x17]; h->ram_size = p[0x18];
    h->country = p[0x19]; h->developer = p[0x1A]; h->version = p[0x1B];
    h->complement = rd16(p + 0x1C); h->checksum = rd16(p + 0x1E);
    h->reset = rd16(p + 0x3C);
    h->computed = checksum(rom, len);
    h->mapping = cand[best].name;
    h->bank_size = cand[best].bank;
    h->fast = (h->map & 0x10) != 0;
    h->chip = chip_name(h->type);
    unsigned kind = h->type & 0x0F;
    h->has_ram = kind == 1 || kind == 2 || kind == 4 || kind == 5;
    h->has_battery = kind == 2 || kind == 5 || kind == 6;
    h->region = region_of(h->country);
    h->declared_bytes = h->rom_size < 24 ? 1024L << h->rom_size : 0;
    unsigned ram = h->ram_size;
    if (h->chip && strcmp(h->chip, "Super FX") == 0 && p[-3] != 0xFF) ram = p[-3];   /* $FFBD: expansion RAM */
    h->ram_bytes = (ram && ram < 16) ? 1024L << ram : 0;
    return 0;
}

unsigned rom_crc32(const unsigned char *p, size_t n)
{
    static unsigned table[256];
    if (!table[1])
        for (unsigned i = 0; i < 256; i++) {
            unsigned c = i;
            for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
    unsigned c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static unsigned rol(unsigned v, int s) { return (v << s) | (v >> (32 - s)); }

static void sha1_block(unsigned st[5], const unsigned char *b)
{
    unsigned w[80];
    for (int i = 0; i < 16; i++)
        w[i] = ((unsigned)b[4 * i] << 24) | ((unsigned)b[4 * i + 1] << 16) | ((unsigned)b[4 * i + 2] << 8) | b[4 * i + 3];
    for (int i = 16; i < 80; i++) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    unsigned a = st[0], bb = st[1], c = st[2], d = st[3], e = st[4];
    for (int i = 0; i < 80; i++) {
        unsigned f, k;
        if (i < 20)      { f = (bb & c) | (~bb & d);          k = 0x5A827999u; }
        else if (i < 40) { f = bb ^ c ^ d;                    k = 0x6ED9EBA1u; }
        else if (i < 60) { f = (bb & c) | (bb & d) | (c & d); k = 0x8F1BBCDCu; }
        else             { f = bb ^ c ^ d;                    k = 0xCA62C1D6u; }
        unsigned t = rol(a, 5) + f + e + k + w[i];
        e = d; d = c; c = rol(bb, 30); bb = a; a = t;
    }
    st[0] += a; st[1] += bb; st[2] += c; st[3] += d; st[4] += e;
}

void rom_sha1(const unsigned char *p, size_t n, char hex[41])
{
    unsigned st[5] = { 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u };
    size_t i = 0;
    for (; i + 64 <= n; i += 64) sha1_block(st, p + i);
    unsigned char tail[128];
    size_t rest = n - i, tl = rest < 56 ? 64 : 128;
    memset(tail, 0, sizeof tail);
    memcpy(tail, p + i, rest);
    tail[rest] = 0x80;
    unsigned long long bits = (unsigned long long)n * 8;
    for (int k = 0; k < 8; k++) tail[tl - 1 - k] = (unsigned char)(bits >> (8 * k));
    sha1_block(st, tail);
    if (tl == 128) sha1_block(st, tail + 64);
    for (int k = 0; k < 5; k++) sprintf(hex + 8 * k, "%08x", st[k]);
}
