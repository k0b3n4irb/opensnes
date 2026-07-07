/* ROM-const struct data emission: pointer-typed fields must occupy their
 * full 4-byte slot even when their value is a CONSTANT (NULL), and 32-bit
 * numeric literals must survive intact.
 *
 * Two historic silent-corruption bugs pinned here:
 *  - chantier C.5 (2026-05-06) padded SYMBOL references (.dl <sym> = 3
 *    bytes + .dsb 1) but missed constants: `.dl 0` also writes only 3
 *    bytes in WLA-DX, shifting every later field by -1 (found via
 *    anim.h's AnimClip NULL `durations`, 2026-07-07);
 *  - worse, `.dl <n>` truncates numeric values to 24 bits, so u32
 *    literals >= 0x01000000 lost their top byte even when padded.
 * Fix: numeric DW/DL fields emit as two explicit .dw halves. */

typedef struct {
    const unsigned short *p;   /* NULL here — the 4-byte-slot case */
    unsigned char tag;
} S;

static const S s = { 0, 0xAB };

static const unsigned long big = 0x12345678;  /* > 24 bits: no truncation */

unsigned char get_tag(void) { return s.tag; }
unsigned long get_big(void) { return big; }
