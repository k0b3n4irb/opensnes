/**
 * @file main.c
 * @brief LZ77 decompression into RAM, screen on: lzssDecodeRam
 * @ingroup examples
 *
 * lzssDecodeVram expands a compressed block straight into VRAM, which
 * needs forced blank. lzssDecodeRam expands it into RAM instead, in any
 * bank, with the screen on and the NMI running: the game decompresses its
 * tiles while it plays, then sends them to VRAM by DMA in VBlank.
 *
 * This example decodes, with the screen on, a 2 bpp tile block that
 * opensnes-sprite compressed (32768 bytes from 17856) into bank $7F and into
 * a FAR buffer of bank $7E, and compares each copy byte for byte with the
 * same tiles converted without compression. Hand-built streams then check
 * the edge cases: literals only, long references (also into the low 8 KB,
 * the byte-loop copy), a reference cut at the end of the output,
 * overlapping references (distance 1, distance shorter than the length)
 * into $7F, $7E and the low 8 KB, a stream that ends exactly at the end of
 * bank $7F, and four refused
 * streams (length 0, length above 65535, a wrong tag, an output that would
 * cross the end of the bank), each of which must return 0 and leave its
 * destination as it was. The verdict of each test is in WRAM, for the
 * luna manifest, and on screen.
 *
 * @par SNES Concepts
 * - LZ77 (type 0x10) decompression: flag bytes, literals, back references
 * - The 128 KB of work RAM: bank $7E (FAR objects) and bank $7F
 * - MVN block moves, and why an overlapping move repeats a pattern
 * - Decompressing with the screen on: no PPU access, NMI left enabled
 *
 * @par What to Observe
 * - One line per test: its name, the bytes returned, OK or BAD
 * - ALL OK at the bottom when every test passed
 *
 * @par Modules Used
 * console, dma, text, background, sprite, lzss
 *
 * @see lzss.h, text.h
 */

#include <snes.h>
#include <snes/lzss.h>

#include "res/tiles.inc"
#include "res/tiles_lz.inc"
/* generated: tiles_tiles (32768 bytes as they are), tiles_lz_tiles (the LZ77 stream) */

/** @name Hand-built streams (streams.asm)
 *  @{ */
extern const u8 lit_lz[];       /**< 4096 literals P(k) */
extern const u8 ref_lz[];       /**< 32 literals, then 224 references of 18 bytes at distance 32 */
extern const u8 ref194_lz[];    /**< the same, 194 bytes: 32 literals, 9 references */
extern const u8 clip_lz[];      /**< a reference that runs past the 5-byte end */
extern const u8 ovl_lz[];       /**< overlapping references, 44 bytes */
extern const u8 ovl_expected[]; /**< what ovl_lz expands to */
extern const u8 ovlong_lz[];    /**< overlapping references, 330 bytes (fast loop) */
extern const u8 zero_lz[];      /**< length 0 */
extern const u8 big_lz[];       /**< length 65536 */
extern const u8 tag_lz[];       /**< tag $11 */
/** @} */

/** @brief Size of the tile block, as converted without compression */
#define TILES_SIZE 32768
/** @brief Bytes checked after each output: the decoder must not write past its end */
#define GUARD 16
/** @brief What every destination holds before a decode */
#define FILL 0xA5

/** @brief Bank $7F: the tiles at $7F:0000, the hand-built streams from $7F:9000 */
#define BANK7F(addr) ((u8 FAR *)(0x7F0000UL + (addr)))

/** @brief Bank $7E destination: a FAR buffer, outside the 8 KB band */
FAR u8 buf7e[TILES_SIZE + GUARD];
/** @brief Low-RAM destination (bank $00): the decoder's byte-loop copy */
u8 buf_low[330 + GUARD];

/** @brief Set by luna (--poke-at main --poke bench_sel=0N) to time one decode alone */
u8 bench_sel;

/** @name Results, read by testing/manifests/memory_lzss_ram.toml
 *  For each test: r_X_len = the return value, r_X_bad = bytes that differ
 *  from the expectation, guard bytes included (0 = pass).
 *  @{ */
u16 r_t7f_len, r_t7f_bad;
u16 r_t7e_len, r_t7e_bad;
u16 r_lit_len, r_lit_bad;
u16 r_ref_len, r_ref_bad;
u16 r_r194_len, r_r194_bad;
u16 r_clip_len, r_clip_bad;
u16 r_ovlong7f_len, r_ovlong7f_bad;
u16 r_ovlong7e_len, r_ovlong7e_bad;
u16 r_ovlonglo_len, r_ovlonglo_bad;
u16 r_ovl7f_len, r_ovl7f_bad;
u16 r_ovl7e_len, r_ovl7e_bad;
u16 r_ovllo_len, r_ovllo_bad;
u16 r_edge_len, r_edge_bad;
u16 r_zero_len, r_zero_bad;
u16 r_big_len, r_big_bad;
u16 r_tag_len, r_tag_bad;
u16 r_cross_len, r_cross_bad;
u16 r_failed;               /**< tests that failed */
u16 r_done;                 /**< 0xD0E5 once every test ran */
/** @} */

/** @brief Fill n bytes at d with FILL */
static void fill(u8 FAR *d, u16 n) {
    u16 i;
    for (i = 0; i < n; i++) d[i] = FILL;
}

/** @brief Bytes of d[0..n) that differ from ref[0..n) */
static u16 diff(const u8 *d, const u8 *ref, u16 n) {
    u16 i, bad = 0;
    for (i = 0; i < n; i++) {
        if (d[i] != ref[i]) bad++;
    }
    return bad;
}

/** @brief Bytes of d[0..n) that are not FILL: what a refused decode or an overrun touched */
static u16 touched(const u8 *d, u16 n) {
    u16 i, bad = 0;
    for (i = 0; i < n; i++) {
        if (d[i] != FILL) bad++;
    }
    return bad;
}

/** @brief Bytes of d[0..n) that differ from P(k mod period), P(k) = k * 37 + 11 */
static u16 diff_pattern(const u8 *d, u16 n, u16 period) {
    u16 i, bad = 0;
    u8 v = 11;
    u16 k = 0;
    for (i = 0; i < n; i++) {
        if (d[i] != v) bad++;
        v += 37;
        if (++k == period) { k = 0; v = 11; }
    }
    return bad;
}

/** @brief Decode the tiles into d; check the bytes and the guard after them */
static u16 test_tiles(u8 FAR *d, u16 *len) {
    fill(d, TILES_SIZE);
    fill(d + TILES_SIZE, GUARD);
    *len = lzssDecodeRam(tiles_lz_tiles, d);
    return diff(d, tiles_tiles, TILES_SIZE) + touched(d + TILES_SIZE, GUARD);
}

/** @brief Decode the overlap stream into d (44 bytes) */
static u16 test_ovl(u8 FAR *d, u16 *len) {
    fill(d, 44 + GUARD);
    *len = lzssDecodeRam(ovl_lz, d);
    return diff(d, ovl_expected, 44) + touched(d + 44, GUARD);
}

/** @brief Decode the long overlap stream into d: 127 x 'A', 55 x "BC", 31 x "DEF" */
static u16 test_ovlong(u8 FAR *d, u16 *len) {
    u16 i, bad = 0;
    u8 k = 0;
    fill(d, 330 + GUARD);
    *len = lzssDecodeRam(ovlong_lz, d);
    for (i = 0; i < 127; i++) {
        if (d[i] != 0x41) bad++;
    }
    for (; i < 237; i++) {
        if (d[i] != (u8)(0x42 + ((i - 127) & 1))) bad++;
    }
    for (; i < 330; i++) {
        if (d[i] != (u8)(0x44 + k)) bad++;
        if (++k == 3) k = 0;
    }
    return bad + touched(d + 330, GUARD);
}

/** @brief Decode a stream that must be refused: 0 back, nothing written */
static u16 test_refused(const u8 *s, u8 FAR *d, u16 *len) {
    fill(d, GUARD);
    *len = lzssDecodeRam(s, d);
    return touched(d, GUARD);
}

/** @brief One decode alone, for luna profile (see the README) */
static void bench(u8 which) {
    switch (which) {
    case 1: lzssDecodeRam(lit_lz, BANK7F(0x9000)); break;
    case 2: lzssDecodeRam(ref_lz, BANK7F(0x9000)); break;
    case 3: lzssDecodeRam(tiles_lz_tiles, BANK7F(0x0000)); break;
    default: break;
    }
}

/** @brief Print one result line: name, bytes returned, verdict */
static void show(u8 row, const char *name, u16 len, u16 bad) {
    textPrintAt(2, row, name);
    textSetPos(14, row);
    textPrintU16(len);
    textPrintAt(22, row, bad ? "BAD" : "OK");
    if (bad) r_failed++;
}

/** @brief Run every test with the screen on, then show the verdicts */
int main(void) {
    textModeInit();
    setColor(0, RGB(1, 3, 4));
    textPrintAt(2, 1, "LZSSDECODERAM");
    WaitForVBlank();
    setScreenOn();

    /* From here on the screen is on and the NMI runs every frame. */
    if (bench_sel) {
        bench(bench_sel);
        r_done = 0xD0E5;
        while (1) WaitForVBlank();
    }

    r_t7f_bad = test_tiles(BANK7F(0x0000), &r_t7f_len);
    r_t7e_bad = test_tiles(buf7e, &r_t7e_len);

    fill(BANK7F(0x9000), 4096 + GUARD);
    r_lit_len = lzssDecodeRam(lit_lz, BANK7F(0x9000));
    r_lit_bad = diff_pattern(BANK7F(0x9000), 4096, 4096) + touched(BANK7F(0x9000 + 4096), GUARD);

    fill(buf7e, 4064 + GUARD);
    r_ref_len = lzssDecodeRam(ref_lz, buf7e);
    r_ref_bad = diff_pattern(buf7e, 4064, 32) + touched(buf7e + 4064, GUARD);

    fill(buf_low, 194 + GUARD);
    r_r194_len = lzssDecodeRam(ref194_lz, buf_low);
    r_r194_bad = diff_pattern(buf_low, 194, 32) + touched(buf_low + 194, GUARD);

    /* 'A' then 18 x 'A' at distance 1, in a stream 5 bytes long: cut at 5 */
    fill(BANK7F(0xC000), 5 + GUARD);
    r_clip_len = lzssDecodeRam(clip_lz, BANK7F(0xC000));
    r_clip_bad = diff(BANK7F(0xC000), ovl_expected, 5) + touched(BANK7F(0xC000 + 5), GUARD);

    r_ovlong7f_bad = test_ovlong(BANK7F(0xA000), &r_ovlong7f_len);
    r_ovlong7e_bad = test_ovlong(buf7e, &r_ovlong7e_len);
    r_ovlonglo_bad = test_ovlong(buf_low, &r_ovlonglo_len);

    r_ovl7f_bad = test_ovl(BANK7F(0xA000), &r_ovl7f_len);
    r_ovl7e_bad = test_ovl(buf7e, &r_ovl7e_len);
    r_ovllo_bad = test_ovl(buf_low, &r_ovllo_len);

    /* the last 44 bytes of bank $7F: the end address wraps to $0000 */
    fill(BANK7F(0xFFD4), 44);
    r_edge_len = lzssDecodeRam(ovl_lz, BANK7F(0xFFD4));
    r_edge_bad = diff(BANK7F(0xFFD4), ovl_expected, 44);

    r_zero_bad = test_refused(zero_lz, BANK7F(0xB000), &r_zero_len);
    r_big_bad = test_refused(big_lz, BANK7F(0xB000), &r_big_len);
    r_tag_bad = test_refused(tag_lz, BANK7F(0xB000), &r_tag_len);
    /* 44 bytes from $7F:FFE0 would cross into bank $80 */
    r_cross_bad = test_refused(ovl_lz, BANK7F(0xFFE0), &r_cross_len);

    show(3, "TILES 7F", r_t7f_len, r_t7f_bad || r_t7f_len != TILES_SIZE);
    show(4, "TILES 7E", r_t7e_len, r_t7e_bad || r_t7e_len != TILES_SIZE);
    show(5, "LITERALS", r_lit_len, r_lit_bad || r_lit_len != 4096);
    show(6, "LONG REFS", r_ref_len, r_ref_bad || r_ref_len != 4064);
    show(7, "REFS LOW", r_r194_len, r_r194_bad || r_r194_len != 194);
    show(8, "CUT AT END", r_clip_len, r_clip_bad || r_clip_len != 5);
    show(9, "OVL LONG 7F", r_ovlong7f_len, r_ovlong7f_bad || r_ovlong7f_len != 330);
    show(10, "OVL LONG 7E", r_ovlong7e_len, r_ovlong7e_bad || r_ovlong7e_len != 330);
    show(11, "OVL LONG LO", r_ovlonglo_len, r_ovlonglo_bad || r_ovlonglo_len != 330);
    show(12, "OVERLAP 7F", r_ovl7f_len, r_ovl7f_bad || r_ovl7f_len != 44);
    show(13, "OVERLAP 7E", r_ovl7e_len, r_ovl7e_bad || r_ovl7e_len != 44);
    show(14, "OVERLAP LOW", r_ovllo_len, r_ovllo_bad || r_ovllo_len != 44);
    show(15, "END OF 7F", r_edge_len, r_edge_bad || r_edge_len != 44);
    show(16, "LENGTH 0", r_zero_len, r_zero_bad || r_zero_len != 0);
    show(17, "OVER 64K", r_big_len, r_big_bad || r_big_len != 0);
    show(18, "TAG 11", r_tag_len, r_tag_bad || r_tag_len != 0);
    show(19, "CROSS BANK", r_cross_len, r_cross_bad || r_cross_len != 0);
    textPrintAt(2, 21, r_failed ? "SOME FAILED" : "ALL OK");
    r_done = 0xD0E5;

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
