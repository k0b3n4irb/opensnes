/*
 * oamPlaceWorld() against a C model of its contract (stress fixture).
 *
 * Random batches, each followed by a compare of the WHOLE sprite table
 * (544 bytes), of the batch's visible[] array, of oam_max_id, of the update
 * flag and of the data bank register:
 *   - the four input arrays in plain RAM (bank $00, below $2000), in FAR RAM
 *     ($7E:2000+) or in ROM (an asset bank), from a random offset;
 *   - visible[] absent, in plain RAM or in FAR RAM;
 *   - order absent, in plain RAM (random bytes, some at or above the count)
 *     or in ROM (a permutation by 37);
 *   - first_id 0..127, sometimes 128..130 (refused), count 0..128, a batch
 *     running past sprite 127 (clamped), size 8, 16, 32 or 64;
 *   - positions put on the culling limits (-size - 1 .. -size + 1, -1, 0, 1,
 *     223, 224, 255, 256, 257) or anywhere;
 *   - the table filled with random bytes first, so that the size bits and
 *     the other sprites' x bits are there to be kept.
 * A hidden entry's tile and attribute bytes are not part of the contract
 * (the routine may leave anything there): they are not compared.
 *
 * Read by world_place.toml: r_bad = 0, r_batches, r_placed, r_hidden,
 * r_done.
 */
#include <snes.h>

#define N 144
#define BATCHES 10000

extern const s16 wp_rom_x[];
extern const s16 wp_rom_y[];
extern const u8 wp_rom_tile[];
extern const u8 wp_rom_attr[];
extern const u8 wp_rom_order[];
extern u16 wp_dbr(void);
extern void wp_quiet(void);
extern void wp_loud(void);
extern u8 oam_max_id;

/* Results, read by the manifest. */
u16 r_bad;          /* wrong bytes, flags, DBR                        -> 0 */
u16 r_batches;      /* batches run */
u16 r_placed;       /* sprites the model placed (proves the mix) */
u16 r_hidden;       /* sprites the model hid */
u16 r_first_batch;  /* the first failing batch (diagnosis) */
u16 r_first_what;   /* 0x0xxx table byte, 0x1xxx visible, 0x2000 max id, 0x2001 flag, 0x2002 DBR */
u16 r_done;         /* 0xC0DE at the end */

static s16 lo_x[N];
static s16 lo_y[N];
static u8 lo_tile[N];
static u8 lo_attr[N];
static u8 lo_vis[N];
static u8 lo_order[N];
static FAR s16 fa_x[N];
static FAR s16 fa_y[N];
static FAR u8 fa_tile[N];
static FAR u8 fa_attr[N];
static FAR u8 fa_vis[N];
static FAR u8 exp_oam[544];
static FAR u8 exp_vis[N];
static FAR u8 dont_care[128];   /* 1: entry hidden, its tile and attr not compared */

static OamWorldBatch b;
static u16 seed = 0xACE1;
static u16 src, vsrc, osrc, off, size, cam_x, cam_y, dbr0;
static u8 exp_max;

static u16 rnd(void)
{
    /* xorshift16 */
    seed ^= seed << 7;
    seed ^= seed >> 9;
    seed ^= seed << 8;
    return seed;
}

static s16 edge(u16 limit)
{
    u16 r = rnd() & 15;
    switch (r) {
    case 0: return -(s16)size - 1;
    case 1: return -(s16)size;
    case 2: return -(s16)size + 1;
    case 3: return -1;
    case 4: return 0;
    case 5: return 1;
    case 6: return (s16)limit - 1;
    case 7: return (s16)limit;
    case 8: return (s16)limit + 1;
    case 9: return 255;
    case 10: return 256;
    case 11: return 257;
    default: return (s16)(rnd() % 600) - 200;
    }
}

static s16 get_x(u16 i)
{
    if (src == 0) return lo_x[off + i];
    if (src == 1) return fa_x[off + i];
    return wp_rom_x[off + i];
}

static s16 get_y(u16 i)
{
    if (src == 0) return lo_y[off + i];
    if (src == 1) return fa_y[off + i];
    return wp_rom_y[off + i];
}

static u8 get_tile(u16 i)
{
    if (src == 0) return lo_tile[off + i];
    if (src == 1) return fa_tile[off + i];
    return wp_rom_tile[off + i];
}

static u8 get_attr(u16 i)
{
    if (src == 0) return lo_attr[off + i];
    if (src == 1) return fa_attr[off + i];
    return wp_rom_attr[off + i];
}

static u8 get_order(u16 i)
{
    if (osrc == 1) return lo_order[i];
    return wp_rom_order[i];
}

static u8 get_vis(u16 i)
{
    if (vsrc == 1) return lo_vis[i];
    return fa_vis[i];
}

static void fail(u16 what)
{
    if (!r_bad) {
        r_first_batch = r_batches;
        r_first_what = what;
    }
    r_bad++;
}

static void model_hide(u16 id)
{
    u16 o = id << 2;
    exp_oam[o] = 0x01;
    exp_oam[o + 1] = 0xF0;
    exp_oam[512 + (id >> 2)] |= (u8)(1 << ((id & 3) << 1));
    dont_care[id] = 1;
    r_hidden++;
}

static void model(void)
{
    u16 first = b.first_id, given = b.count, cc, i, id, spr, o, ty, tx, xs;
    u8 bit;
    if (first >= 128) return;
    cc = given;
    if (first + cc > 128) cc = 128 - first;
    if (!cc) return;
    for (i = 0; i < cc; i++) {
        id = first + i;
        if (osrc) {
            spr = get_order(i);
            if (spr >= given) {
                model_hide(id);
                continue;
            }
        } else {
            spr = i;
        }
        ty = (u16)get_y(spr) - (u16)(cam_y - size);
        tx = (u16)get_x(spr) - (u16)(cam_x - size);
        if (ty == 0 || ty >= 224 + size || tx == 0 || tx >= 256 + size) {
            model_hide(id);
            if (vsrc) exp_vis[spr] = 0;
            continue;
        }
        o = id << 2;
        xs = tx - size;
        bit = (u8)(1 << ((id & 3) << 1));
        exp_oam[o] = (u8)xs;
        exp_oam[o + 1] = (u8)(ty - size);
        exp_oam[o + 2] = get_tile(spr);
        exp_oam[o + 3] = get_attr(spr);
        if (xs & 0x8000) exp_oam[512 + (id >> 2)] |= bit;
        else exp_oam[512 + (id >> 2)] &= (u8)~bit;
        if (vsrc) exp_vis[spr] = 1;
        r_placed++;
    }
    if ((u8)(first + cc - 1) >= exp_max) exp_max = (u8)(first + cc - 1);
}

static void one_batch(void)
{
    u16 i, r, count, limit_y = 224;

    /* the shape of the batch */
    r = rnd();
    src = r % 3;
    vsrc = (r >> 2) % 3;
    osrc = (r >> 4) % 3;
    size = 8 << ((r >> 6) & 3);
    off = (r >> 8) & 15;
    r = rnd();
    if ((r & 31) == 0) b.first_id = (u8)(128 + ((r >> 5) % 3));
    else b.first_id = (u8)((r >> 5) & 127);
    r = rnd();
    if ((r & 15) == 0) count = 0;
    else if (r & 16) count = 1 + ((r >> 5) & 7);
    else count = (r >> 5) % (128 - off + 1);
    b.count = (u8)count;
    b.size = (u8)size;
    cam_x = rnd() % 1024;
    cam_y = rnd() % 512;

    /* the arrays */
    if (src == 2) {
        /* fixed in ROM: put the camera on a limit of one of its sprites */
        if (count && (rnd() & 1)) {
            i = off + rnd() % count;
            cam_x = (u16)wp_rom_x[i] - (u16)edge(256);
            cam_y = (u16)wp_rom_y[i] - (u16)edge(limit_y);
        }
        b.x = wp_rom_x + off;
        b.y = wp_rom_y + off;
        b.tile = wp_rom_tile + off;
        b.attr = wp_rom_attr + off;
    } else {
        for (i = 0; i < count; i++) {
            s16 x = (s16)cam_x + edge(256);
            s16 y = (s16)cam_y + edge(limit_y);
            u8 t = (u8)rnd(), a = (u8)rnd();
            if (src == 0) {
                lo_x[off + i] = x; lo_y[off + i] = y;
                lo_tile[off + i] = t; lo_attr[off + i] = a;
            } else {
                fa_x[off + i] = x; fa_y[off + i] = y;
                fa_tile[off + i] = t; fa_attr[off + i] = a;
            }
        }
        if (src == 0) {
            b.x = lo_x + off; b.y = lo_y + off;
            b.tile = lo_tile + off; b.attr = lo_attr + off;
        } else {
            b.x = (const s16 *)(fa_x + off); b.y = (const s16 *)(fa_y + off);
            b.tile = (const u8 *)(fa_tile + off); b.attr = (const u8 *)(fa_attr + off);
        }
    }
    for (i = 0; i < N; i++) {
        u8 v = (u8)(0xA0 + i);
        lo_vis[i] = v;
        fa_vis[i] = v;
        exp_vis[i] = v;
    }
    if (vsrc == 0) b.visible = 0;
    else if (vsrc == 1) b.visible = lo_vis;
    else b.visible = (u8 *)fa_vis;
    if (osrc == 0) {
        b.order = 0;
    } else if (osrc == 1) {
        for (i = 0; i < N; i++) {
            r = rnd();
            lo_order[i] = (u8)((r & 7) == 0 ? count + (r >> 8) % 8 : (count ? (r >> 3) % count : 0));
        }
        b.order = lo_order;
    } else {
        b.order = wp_rom_order;
    }

    /* the table: random every 8 batches, else as the last batch left it */
    if ((r_batches & 7) == 0) {
        for (i = 0; i < 544; i++) {
            u8 v = (u8)rnd();
            oamMemory[i] = v;
            exp_oam[i] = v;
        }
    }
    for (i = 0; i < 128; i++) dont_care[i] = 0;
    oam_max_id = (u8)(rnd() & 127);
    exp_max = oam_max_id;
    oam_update_flag = 0;

    model();
    oamPlaceWorld(&b, cam_x, cam_y);

    /* compare */
    for (i = 0; i < 544; i++) {
        if (i < 512 && dont_care[i >> 2] && (i & 3) >= 2) {
            exp_oam[i] = oamMemory[i];
            continue;
        }
        if (oamMemory[i] != exp_oam[i]) {
            fail(i);
            exp_oam[i] = oamMemory[i];
        }
    }
    if (vsrc) {
        for (i = 0; i < N; i++) {
            if (get_vis(i) != exp_vis[i]) {
                fail(0x1000 | i);
                break;
            }
        }
    }
    if (oam_max_id != exp_max) fail(0x2000);
    if (b.first_id < 128 && count && oam_update_flag != 1) fail(0x2001);
    if ((b.first_id >= 128 || !count) && oam_update_flag != 0) fail(0x2001);
    if (wp_dbr() != dbr0) fail(0x2002);
    r_batches++;
}

int main(void)
{
    u16 n;
    consoleInit();
    wp_quiet();
    dbr0 = wp_dbr();
    for (n = 0; n < BATCHES; n++) one_batch();
    wp_loud();
    r_done = 0xC0DE;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
