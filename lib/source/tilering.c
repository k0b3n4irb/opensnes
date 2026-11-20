/**
 * @file tilering.c
 * @brief A ring of character slots in VRAM for maps with more characters than VRAM holds
 *
 * See snes/tilering.h. Everything here runs outside VBlank: it reads the
 * map, keeps the counts and notes transfers in the VRAM queue, which
 * vramQueueFlush() sends. tileRingFill() alone writes VRAM, in force blank.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/dma.h>
#include <snes/tilering.h>
#include <snes/vramqueue.h>

/* DMA channel 0 and the WRAM port, written directly: dmaTransfer() is a
 * C call with five arguments, for transfers of 32 bytes. */
#define TR_DMAP0    (*(vu8 *)0x4300)
#define TR_BBAD0    (*(vu8 *)0x4301)
#define TR_A1T0     (*(vu16 *)0x4302)
#define TR_A1B0     (*(vu8 *)0x4304)
#define TR_DAS0     (*(vu16 *)0x4305)
#define TR_MDMAEN   (*(vu8 *)0x420B)
#define TR_WMADD    (*(vu16 *)0x2181)
#define TR_WMADDH   (*(vu8 *)0x2183)

#define TR_HDR_MAGIC 0x5254

/* The cell loops (tilering_cells.asm) and their context. */
u16 tileRingCellsIn(const u16 *src, u16 stride, u16 n, u16 *dst);
void tileRingCellsOut(const u16 *src, u16 stride, u16 n);
void tileRingCellsFix(u16 *dst, u16 m);
u16 tileRingLoad(u16 *dst, u16 m);
u16 FAR *tr_hot_char_of;
u16 tr_hot_slots, tr_hot_cursor, tr_hot_starved;
u16 tr_hot_load_c[64], tr_hot_load_s[64];
u16 FAR *tr_hot_slot_of;
u8 FAR *tr_hot_refs;
const u8 *tr_hot_charpal;
u16 tr_hot_first;
u16 tr_hot_resident;
u16 tr_hot_miss[64];

/* The rest of the ring's working state, copied from the TileRing by
 * frame_begin() and back by frame_end(): compiled code reaches a global in
 * one instruction, a field through the ring's pointer in five or six. One
 * ring is worked on at a time, for the duration of one public call. */
static u8 FAR *g_staging;
static const u8 *const *g_char_parts;
static u16 g_chr_vram, g_filling;
static u16 g_staged, g_run_slot, g_run_len, g_run_src, g_copy_char, g_copy_len;
static u16 g_new_chars, g_entries, g_refused;

/* the bytes of character c in ROM */
static const u8 *char_src(u16 c)
{
    return g_char_parts[c >> 10] + ((c & 1023) << 5);
}

/* the first map word of row y, and how many rows follow it in its part */
static const u16 *map_row(const TileRing *r, u16 y, u16 *left)
{
    u16 part = y / r->rows_per_part;
    u16 in = y - part * r->rows_per_part;
    *left = r->rows_per_part - in;
    return (const u16 *)(r->map->map_parts[part] + in * (r->width << 1));
}

static void push(const u8 *src, u16 addr, u16 size, u16 step)
{
    if (vramQueuePush(src, addr, size, step))
        g_entries++;
    else
        g_refused++;
}

/* the staging copy: what is gathered in g_copy_char / g_copy_len goes to WRAM */
static void copy_flush(void)
{
    u32 far;
    if (!g_copy_len)
        return;
    far = (u32)char_src(g_copy_char);
    TR_A1T0 = (u16)far;
    TR_A1B0 = (u8)(far >> 16);
    TR_DAS0 = g_copy_len << 5;
    TR_MDMAEN = 1;
    g_copy_len = 0;
}

/* the transfer gathered in g_run_slot / g_run_len */
static void run_flush(void)
{
    const u8 *src;
    u16 addr;
    if (!g_run_len)
        return;
    if (g_staging)
        src = g_staging + (g_run_src << 5);
    else
        src = char_src(g_run_src);
    addr = g_chr_vram + (g_run_slot << 4);
    if (g_filling)
        dmaCopyVram(src, addr, g_run_len << 5);
    else
        push(src, addr, g_run_len << 5, VRAM_QUEUE_ROW);
    g_run_len = 0;
}

/* character c goes to slot s this frame */
static void stage(u16 c, u16 s)
{
    g_new_chars++;
    if (g_staging) {
        if (g_staged == 0) {
            /* the frame's first character: the WRAM port at the buffer's
             * start, then every copy follows the previous one */
            u32 far = (u32)g_staging;
            TR_WMADD = (u16)far;
            TR_WMADDH = (u8)(far >> 16) & 1;
            TR_DMAP0 = 0x00;
            TR_BBAD0 = 0x80;
        }
        if (g_copy_len && c == g_copy_char + g_copy_len && (c & 1023))
            g_copy_len++;
        else {
            copy_flush();
            g_copy_char = c;
            g_copy_len = 1;
        }
        if (g_run_len && s == g_run_slot + g_run_len)
            g_run_len++;
        else {
            run_flush();
            g_run_slot = s;
            g_run_src = g_staged;
            g_run_len = 1;
        }
        g_staged++;
    } else {
        if (g_run_len && s == g_run_slot + g_run_len
            && c == g_run_src + g_run_len && (c & 1023))
            g_run_len++;
        else {
            run_flush();
            g_run_slot = s;
            g_run_src = c;
            g_run_len = 1;
        }
    }
}

/* n cells, stride bytes apart, enter the window: their entries to dst */
static void cells_in(const u16 *p, u16 stride, u16 n, u16 *dst)
{
    u16 m = tileRingCellsIn(p, stride, n, dst), loads, i;
    if (!m)
        return;
    loads = tileRingLoad(dst, m);
    for (i = 0; i < loads; i++)
        stage(tr_hot_load_c[i], tr_hot_load_s[i]);
    tileRingCellsFix(dst, m);
}

/* n cells, stride bytes apart, enter (into dst) or leave the window */
static void cells(const u16 *p, u16 stride, u16 n, u16 *dst)
{
    if (dst)
        cells_in(p, stride, n, dst);
    else
        tileRingCellsOut(p, stride, n);
}

/* VRAM word address of map cell (x, y) in the 64x32 tilemap */
static u16 cell_addr(const TileRing *r, u16 x, u16 y)
{
    return r->tilemap + ((x & 32) << 5) + ((y & 31) << 5) + (x & 31);
}

/* column x, rows cy..cy+win_h-1: enter (into col_buf) or leave. At the
 * map's right or bottom edge, with the camera at its limit on a character
 * boundary, the window's last column or row is not on screen and lies past
 * the map: it repeats the map's last one (counted, never shown). */
static void do_column(TileRing *r, u16 x, u16 in)
{
    u16 left, k = 0, n = r->win_h, y = r->cy, run;
    const u16 *p;
    if (x >= r->width)
        x = r->width - 1;
    while (k < n) {
        if (y >= r->height) {
            p = map_row(r, r->height - 1, &left) + x;
            cells(p, 0, n - k, in ? r->col_buf + k : (u16 *)0);
            return;
        }
        p = map_row(r, y, &left) + x;
        run = n - k;
        if (run > left)
            run = left;
        if (run > r->height - y)
            run = r->height - y;
        cells(p, r->width << 1, run, in ? r->col_buf + k : (u16 *)0);
        k += run;
        y += run;
    }
}

/* row y, columns cx..cx+win_w-1: enter (into row_buf) or leave */
static void do_row(TileRing *r, u16 y, u16 in)
{
    u16 left, n = r->win_w;
    const u16 *p;
    if (y >= r->height)
        y = r->height - 1;
    p = map_row(r, y, &left) + r->cx;
    if (r->cx + n > r->width)
        n = r->width - r->cx;      /* the cells past the edge repeat the last one */
    cells(p, 2, n, in ? r->row_buf : (u16 *)0);
    if (n < r->win_w)
        cells(p + n - 1, 0, r->win_w - n, in ? r->row_buf + n : (u16 *)0);
}

/* the tilemap column written from col_buf: one entry, two where the rows wrap */
static void queue_column(TileRing *r, u16 x)
{
    u16 n1 = 32 - (r->cy & 31);
    if (n1 > r->win_h)
        n1 = r->win_h;
    push((const u8 *)r->col_buf, cell_addr(r, x, r->cy), n1 << 1, VRAM_QUEUE_COLUMN);
    if (n1 < r->win_h)
        push((const u8 *)(r->col_buf + n1), cell_addr(r, x, r->cy + n1), (r->win_h - n1) << 1, VRAM_QUEUE_COLUMN);
}

/* the tilemap row written from row_buf: one entry per 32-cell screen crossed */
static void queue_row(TileRing *r, u16 y)
{
    u16 k = 0, x = r->cx;
    while (k < r->win_w) {
        u16 n = 32 - (x & 31);
        if (n > r->win_w - k)
            n = r->win_w - k;
        push((const u8 *)(r->row_buf + k), cell_addr(r, x, y), n << 1, VRAM_QUEUE_ROW);
        k += n;
        x += n;
    }
}

static void frame_begin(TileRing *r)
{
    tr_hot_slot_of = r->slot_of;
    tr_hot_refs = r->refs;
    tr_hot_charpal = r->map->charpal;
    tr_hot_first = r->first_char;
    tr_hot_resident = r->resident;
    tr_hot_char_of = r->char_of;
    g_staging = r->staging;
    g_char_parts = r->map->char_parts;
    g_chr_vram = r->chr_vram;
    tr_hot_slots = r->slots;
    tr_hot_cursor = r->cursor;
    g_filling = r->filling;
    g_staged = 0;
    g_run_len = 0;
    g_copy_len = 0;
    g_new_chars = 0;
    g_entries = 0;
    tr_hot_starved = r->starved;
    g_refused = r->refused;
}

static void frame_end(TileRing *r)
{
    if (g_staging)
        copy_flush();
    run_flush();
    r->resident = tr_hot_resident;
    r->cursor = tr_hot_cursor;
    r->last_chars = g_new_chars;
    r->last_entries = g_entries;
    r->starved = tr_hot_starved;
    r->refused = g_refused;
}

static u16 rd16(const u8 *p, u16 at)
{
    return p[at] | ((u16)p[at + 1] << 8);
}

u16 tileRingInit(TileRing *r, const TileRingMap *map, u16 chr_vram, u16 first_char,
                 u16 slots, u16 tilemap, u16 FAR *slot_of, u16 FAR *char_of,
                 u8 FAR *refs, u8 FAR *staging)
{
    const u8 *h = map->header;
    if (rd16(h, 0) != TR_HDR_MAGIC)
        return 0;
    r->map = map;
    r->width = rd16(h, 4);
    r->height = rd16(h, 6);
    r->chars = rd16(h, 8);
    r->rows_per_part = rd16(h, 10);
    r->win_w = rd16(h, 12);
    r->win_h = rd16(h, 14);
    r->worst_row = rd16(h, 16);
    r->worst_col = rd16(h, 18);
    r->worst_window = rd16(h, 20);
    if (r->win_w > 64 || r->win_h > 32)
        return 0;
    r->chr_vram = chr_vram;
    r->first_char = first_char;
    r->slots = slots;
    r->tilemap = tilemap;
    r->slot_of = slot_of;
    r->char_of = char_of;
    r->refs = refs;
    r->staging = staging;
    r->cx = 0;
    r->cy = 0;
    r->starved = 0;
    r->refused = 0;
    r->filling = 0;
    r->resident = 0;
    r->cursor = 0;
    frame_begin(r);
    return 1;
}

void tileRingFill(TileRing *r, u16 camx, u16 camy)
{
    u16 i, y;
    u8 FAR *staging = r->staging;
    for (i = 0; i < r->chars; i++)
        r->slot_of[i] = TILERING_NONE;
    for (i = 0; i < r->slots; i++) {
        r->char_of[i] = TILERING_NONE;
        r->refs[i] = 0;
    }
    r->cursor = 0;
    r->resident = 0;
    r->cx = camx >> 3;
    r->cy = camy >> 3;
    /* direct mode for the characters: each run straight from ROM to VRAM */
    r->staging = (u8 FAR *)0;
    r->filling = 1;
    for (y = r->cy; y < r->cy + r->win_h; y++) {
        u16 k = 0, x = r->cx;
        frame_begin(r);
        do_row(r, y, 1);
        frame_end(r);
        /* the tilemap row, one DMA per screen crossed */
        while (k < r->win_w) {
            u16 n = 32 - (x & 31);
            if (n > r->win_w - k)
                n = r->win_w - k;
            dmaCopyVram((const u8 *)(r->row_buf + k), cell_addr(r, x, y), n << 1);
            k += n;
            x += n;
        }
    }
    r->staging = staging;
    r->filling = 0;
    frame_begin(r);
}

u16 tileRingUpdate(TileRing *r, u16 camx, u16 camy)
{
    u16 tx = camx >> 3, ty = camy >> 3, lag = 0;
    frame_begin(r);
    if (tx != r->cx) {
        u16 x;
        if (tx > r->cx) {
            do_column(r, r->cx, 0);
            r->cx++;
            x = r->cx + r->win_w - 1;
            lag += tx - r->cx;
        } else {
            do_column(r, r->cx + r->win_w - 1, 0);
            r->cx--;
            x = r->cx;
            lag += r->cx - tx;
        }
        do_column(r, x, 1);
        queue_column(r, x);
    }
    if (ty != r->cy) {
        u16 y;
        if (ty > r->cy) {
            do_row(r, r->cy, 0);
            r->cy++;
            y = r->cy + r->win_h - 1;
            lag += ty - r->cy;
        } else {
            do_row(r, r->cy + r->win_h - 1, 0);
            r->cy--;
            y = r->cy;
            lag += r->cy - ty;
        }
        do_row(r, y, 1);
        queue_row(r, y);
    }
    frame_end(r);
    return lag;
}

u16 tileRingCost(const TileRing *r, u16 dx, u16 dy)
{
    return ((dx + 7) >> 3) * r->worst_col + ((dy + 7) >> 3) * r->worst_row;
}
