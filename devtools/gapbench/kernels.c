/* gapbench — the C side: one routine per row of the table, its data, its
 * setup and its checksum. The twin of each k_NAME_c is k_NAME_asm in
 * kernels.asm, written by hand on the same data; run.py times both on luna
 * and refuses a row whose two checksums differ.
 *
 * The C is the C a careful game programmer writes (locals for what a store
 * could alias, tables instead of variable shifts), not C bent to the
 * compiler: the gap measured is the compiler's. */
#include <snes.h>
#include <snes/string.h>

/* ---- shared helpers (never timed) ------------------------------------- */

static u16 rng_state;

static u16 rng(void) {
    rng_state = rng_state * 25173 + 13849;
    return rng_state >> 4;
}

static u16 sum(const u8 *p, u16 n) {
    u16 s = 0, i;
    for (i = 0; i < n; i++)
        s = ((s << 3) | (s >> 13)) + p[i] + 1;
    return s;
}

void s_nop(void) {}
u16 c_nop(void) { return 0; }

/* ---- copy: 128 words, low RAM to a FAR buffer -------------------------- */

u16 cp_src[128];
FAR u16 cp_dst[128];

void s_copy(void) {
    u16 i;
    rng_state = 1;
    for (i = 0; i < 128; i++) {
        cp_src[i] = rng();
        cp_dst[i] = 0;
    }
}

void k_copy_c(void) {
    u16 i;
    for (i = 0; i < 128; i++)
        cp_dst[i] = cp_src[i];
}

void k_copy_lib(void) { memcpy(cp_dst, cp_src, 256); }

u16 c_copy(void) { return sum((const u8 *)cp_dst, 256); }

/* ---- fill: 256 words of a FAR buffer with one value -------------------- */

FAR u16 fl_buf[256];
u16 fl_val;

void s_fill(void) { fl_val = 0x1234; }

void k_fill_c(void) {
    u16 i, v = fl_val;
    for (i = 0; i < 256; i++)
        fl_buf[i] = v;
    fl_val = v + 0x0101;
}

/* The same fill, unrolled by hand in the source: what a programmer tries
 * first. It must not be slower than the plain loop (it was, 2026-11-11). */
void k_fillu_c(void) {
    u16 i, v = fl_val;
    for (i = 0; i < 256; i += 8) {
        fl_buf[i] = v;
        fl_buf[i + 1] = v;
        fl_buf[i + 2] = v;
        fl_buf[i + 3] = v;
        fl_buf[i + 4] = v;
        fl_buf[i + 5] = v;
        fl_buf[i + 6] = v;
        fl_buf[i + 7] = v;
    }
    fl_val = v + 0x0101;
}

u16 c_fill(void) { return sum((const u8 *)fl_buf, 512) + fl_val; }

/* ---- oam: a sprite list built from 32 objects, culled, the rest hidden -- */

typedef struct {
    s16 x, y;
    u8 tile, attr, on, pad;
} Spr;

Spr sprs[32];
u8 oam_lo[128];
u8 oam_hi[8];
s16 cam_x, cam_y;
const u8 hi_bit[4] = {1, 4, 16, 64};

void s_oam(void) {
    u16 i;
    rng_state = 2;
    for (i = 0; i < 32; i++) {
        sprs[i].x = (rng() & 511) - 64;
        sprs[i].y = (rng() & 255) - 16;
        sprs[i].tile = rng();
        sprs[i].attr = rng();
        sprs[i].on = (rng() & 7) != 0;
        sprs[i].pad = 0;
    }
    cam_x = -40;
    cam_y = -8;
}

void k_oam_c(void) {
    u16 i, o = 0;
    s16 cx = cam_x, cy = cam_y;
    for (i = 0; i < 8; i++)
        oam_hi[i] = 0;
    for (i = 0; i < 32; i++) {
        Spr *s = &sprs[i];
        s16 sx, sy;
        if (!s->on)
            continue;
        sx = s->x - cx;
        sy = s->y - cy;
        if (sx <= -16 || sx >= 256 || sy <= -16 || sy >= 224)
            continue;
        oam_lo[o] = sx;
        oam_lo[o + 1] = sy;
        oam_lo[o + 2] = s->tile;
        oam_lo[o + 3] = s->attr;
        if (sx & 0x100)
            oam_hi[o >> 4] |= hi_bit[(o >> 2) & 3];
        o += 4;
    }
    for (; o < 128; o += 4)
        oam_lo[o + 1] = 240;
    cam_x = cx + 3;
    cam_y = cy + 1;
}

u16 c_oam(void) { return sum(oam_lo, 128) + sum(oam_hi, 8) + cam_x; }

/* ---- col / row: 32 tilemap entries from a 64x64 map of tile numbers ---- */

FAR u8 map[64 * 64];
u16 tile_tab[256];
u16 colbuf[32], rowbuf[32];
u16 map_x, map_y;

void s_map(void) {
    u16 i;
    rng_state = 3;
    for (i = 0; i < 64 * 64; i++)
        map[i] = rng();
    for (i = 0; i < 256; i++)
        tile_tab[i] = rng();
    map_x = 5;
    map_y = 9;
}

void k_col_c(void) {
    u16 r;
    u8 FAR *p = &map[map_y * 64 + map_x];
    for (r = 0; r < 32; r++) {
        colbuf[r] = tile_tab[*p];
        p += 64;
    }
    map_x = (map_x + 1) & 31;
}

void k_row_c(void) {
    u16 c;
    u8 FAR *p = &map[map_y * 64 + map_x];
    for (c = 0; c < 32; c++)
        rowbuf[c] = tile_tab[p[c]];
    map_y = (map_y + 1) & 31;
}

/* The same row and column with an index into the map instead of a walked
 * pointer: the other way to write them. */
void k_coli_c(void) {
    u16 r, o = map_y * 64 + map_x;
    for (r = 0; r < 32; r++) {
        colbuf[r] = tile_tab[map[o]];
        o += 64;
    }
    map_x = (map_x + 1) & 31;
}

void k_rowi_c(void) {
    u16 c, o = map_y * 64 + map_x;
    for (c = 0; c < 32; c++)
        rowbuf[c] = tile_tab[map[o + c]];
    map_y = (map_y + 1) & 31;
}

u16 c_col(void) { return sum((const u8 *)colbuf, 64) + map_x; }
u16 c_row(void) { return sum((const u8 *)rowbuf, 64) + map_y; }

/* ---- box: one box against 32, a flag each and the count ---------------- */

typedef struct {
    s16 x, y;
    u16 w, h;
} Box;

Box boxes[32];
Box pbox;
u8 hit[32];
u16 hits;

void s_box(void) {
    u16 i;
    rng_state = 4;
    for (i = 0; i < 32; i++) {
        boxes[i].x = (rng() & 255) - 20;
        boxes[i].y = (rng() & 255) - 20;
        boxes[i].w = 8 + (rng() & 31);
        boxes[i].h = 8 + (rng() & 31);
    }
    pbox.x = -30;
    pbox.y = 10;
    pbox.w = 24;
    pbox.h = 32;
    hits = 0;
}

void k_box_c(void) {
    u16 i, n = 0;
    s16 px = pbox.x, py = pbox.y;
    s16 px2 = px + pbox.w, py2 = py + pbox.h;
    for (i = 0; i < 32; i++) {
        Box *b = &boxes[i];
        u8 h = 0;
        if (b->x < px2 && px < b->x + (s16)b->w && b->y < py2 && py < b->y + (s16)b->h) {
            h = 1;
            n++;
        }
        hit[i] = h;
    }
    hits += n;
    px += 5;
    if (px > 250)
        px = -30;
    pbox.x = px;
    pbox.y = py + 3;
}

u16 c_box(void) { return sum(hit, 32) + hits + pbox.x; }

/* ---- fixed: 16 positions += velocity (8.8) * scale (0.8) --------------- */

s16 fx_vel[16], fx_pos[16];
u8 fx_scale;

void s_fixed(void) {
    u16 i;
    rng_state = 5;
    for (i = 0; i < 16; i++) {
        fx_vel[i] = (s16)(rng() << 4) >> 3;
        fx_pos[i] = rng();
    }
    fx_scale = 9;
}

void k_fixed_c(void) {
    u16 i, sc = fx_scale;
    for (i = 0; i < 16; i++)
        fx_pos[i] += (s16)(((s32)fx_vel[i] * sc) >> 8);
    fx_scale = sc + 7;
}

u16 c_fixed(void) { return sum((const u8 *)fx_pos, 32) + fx_scale; }

/* ---- mul: 16 products of two bytes ------------------------------------- */

u8 mu_a[16], mu_b[16];
u16 mu_p[16];

void s_mul(void) {
    u16 i;
    rng_state = 6;
    for (i = 0; i < 16; i++) {
        mu_a[i] = rng();
        mu_b[i] = rng();
    }
}

void k_mul_c(void) {
    u16 i;
    for (i = 0; i < 16; i++) {
        u8 a = mu_a[i];
        mu_p[i] = (u16)a * mu_b[i];
        mu_a[i] = a + 3;
    }
}

u16 c_mul(void) { return sum((const u8 *)mu_p, 32) + sum(mu_a, 16); }

/* ---- ents: 32 structures of 12 bytes, moved, timed, turned ------------- */

typedef struct {
    s16 x, y, vx, vy;
    u8 timer, state;
    u16 hp;
} Ent;

Ent ents[32];

void s_ents(void) {
    u16 i;
    rng_state = 7;
    for (i = 0; i < 32; i++) {
        ents[i].x = rng();
        ents[i].y = rng();
        ents[i].vx = (rng() & 15) - 8;
        ents[i].vy = (rng() & 15) - 8;
        ents[i].timer = 1 + (rng() & 7);
        ents[i].state = (rng() & 3) != 0;
        ents[i].hp = rng() & 3;
    }
}

void k_ents_c(void) {
    Ent *e = ents;
    u16 i;
    for (i = 0; i < 32; i++, e++) {
        if (!e->state)
            continue;
        e->x += e->vx;
        e->y += e->vy;
        if (--e->timer == 0) {
            e->timer = 6;
            e->vx = -e->vx;
            if (e->hp)
                e->hp--;
            else
                e->state = 0;
        }
    }
}

/* The same structures by index, ents[i].field, instead of a walked pointer. */
void k_entsi_c(void) {
    u16 i;
    for (i = 0; i < 32; i++) {
        if (!ents[i].state)
            continue;
        ents[i].x += ents[i].vx;
        ents[i].y += ents[i].vy;
        if (--ents[i].timer == 0) {
            ents[i].timer = 6;
            ents[i].vx = -ents[i].vx;
            if (ents[i].hp)
                ents[i].hp--;
            else
                ents[i].state = 0;
        }
    }
}

u16 c_ents(void) { return sum((const u8 *)ents, sizeof(ents)); }

/* ---- call: 64 calls of a three-argument function ----------------------- */

u16 ca_acc[8];

void s_call(void) {
    u16 i;
    for (i = 0; i < 8; i++)
        ca_acc[i] = i * 100;
}

u16 k_call_f(u16 a, u16 b, u16 lim) {
    u16 r = a + b;
    if (r >= lim)
        r -= lim;
    return r;
}

void k_call_c(void) {
    u16 i;
    for (i = 0; i < 64; i++)
        ca_acc[i & 7] = k_call_f(ca_acc[i & 7], i, 1000);
}

u16 c_call(void) { return sum((const u8 *)ca_acc, 16); }

/* ---- soa: 18 actors in parallel arrays, bounded, moved, recorded as
 * sprites with a depth. The shape of the heaviest routine of a real game's
 * profile (11 % of its busy time). ------------------------------------- */

#define NA 18
#define MIN_X 16
#define MAX_X 623
#define MIN_Y 24
#define MAX_Y 455

s16 ac_x[NA], ac_y[NA], ac_dx[NA], ac_dy[NA];
u16 ac_frame[NA];
u8 ac_dirty[NA], ac_flags[NA];
s16 sp_x[NA], sp_y[NA], sp_depth[NA];
u16 sp_frame[NA];
u8 sp_changed;
u16 sp_depth_changed;
const s16 frame_off[32] = {0, 1, 2, 3, 2, 1, 0, -1, -2, -3, -2, -1, 0, 4, 8, 4,
                                  0, -4, -8, -4, 0, 2, 4, 6, 8, 6, 4, 2, 0, -5, 5, 0};

void s_soa(void) {
    u16 i;
    rng_state = 8;
    for (i = 0; i < NA; i++) {
        ac_x[i] = rng() & 1023;
        ac_y[i] = rng() & 511;
        ac_dx[i] = (i & 3) ? (s16)(rng() & 7) - 3 : 0;
        ac_dy[i] = (i & 3) ? (s16)(rng() & 7) - 3 : 0;
        ac_frame[i] = rng() & 31;
        ac_dirty[i] = rng() & 1;
        ac_flags[i] = rng();
        sp_x[i] = sp_y[i] = sp_depth[i] = 0;
        sp_frame[i] = 0;
    }
    sp_changed = sp_depth_changed = 0;
}

void k_soa_c(void) {
    u16 id;
    for (id = 0; id < NA; id++) {
        u16 frame;
        if (ac_dx[id] | ac_dy[id]) {
            s16 x = ac_x[id];
            s16 y = ac_y[id];
            if (x > MAX_X) x = MAX_X;
            else if (x < MIN_X) x = MIN_X;
            if (y > MAX_Y) y = MAX_Y;
            else if (y < MIN_Y) y = MIN_Y;
            ac_x[id] = x + ac_dx[id];
            ac_y[id] = y + ac_dy[id];
        } else if (!ac_dirty[id]) {
            continue;
        }
        ac_dirty[id] = 0;
        frame = ac_frame[id];
        if (frame < 24)
            sp_frame[id] = frame;
        sp_x[id] = ac_x[id] + frame_off[frame] - 16;
        sp_y[id] = ac_y[id] - 16;
        sp_changed = 1;
        {
            s16 depth = ac_y[id] - ((ac_flags[id] & 0x10) ? 16 : 0);
            if (sp_depth[id] != depth) {
                sp_depth[id] = depth;
                sp_depth_changed++;
            }
        }
    }
}

u16 c_soa(void) {
    return sum((const u8 *)ac_x, sizeof(ac_x)) + sum((const u8 *)ac_y, sizeof(ac_y))
         + sum((const u8 *)sp_x, sizeof(sp_x)) + sum((const u8 *)sp_y, sizeof(sp_y))
         + sum((const u8 *)sp_depth, sizeof(sp_depth)) + sum((const u8 *)sp_frame, sizeof(sp_frame))
         + sum(ac_dirty, NA) + sp_changed + sp_depth_changed;
}

/* ---- glide: 48 drawn objects placed between their previous and their
 * current position through an easing table, written to the OAM mirror, the
 * ninth bit of x tracked. The shape of the sprite pass of a second real
 * game (its heaviest common loop). -------------------------------------- */

#define ND 48

u16 dl_x[ND], dl_px[ND], dl_y[ND], dl_py[ND];
u16 gl_x9[ND];
u8 gl_oam[ND * 4];
u16 gl_cam, gl_row, gl_changed;
const s8 glide_tab[2 * 97] = {
#define G(d) ((d) / 3)
#define H(d) ((d) * 2 / 3)
#define ROW8(M, d) M(d), M(d + 1), M(d + 2), M(d + 3), M(d + 4), M(d + 5), M(d + 6), M(d + 7)
    ROW8(G, -48), ROW8(G, -40), ROW8(G, -32), ROW8(G, -24), ROW8(G, -16), ROW8(G, -8),
    ROW8(G, 0), ROW8(G, 8), ROW8(G, 16), ROW8(G, 24), ROW8(G, 32), ROW8(G, 40), G(48),
    ROW8(H, -48), ROW8(H, -40), ROW8(H, -32), ROW8(H, -24), ROW8(H, -16), ROW8(H, -8),
    ROW8(H, 0), ROW8(H, 8), ROW8(H, 16), ROW8(H, 24), ROW8(H, 32), ROW8(H, 40), H(48)};

void s_glide(void) {
    u16 i;
    rng_state = 9;
    for (i = 0; i < ND; i++) {
        dl_px[i] = 40 + (rng() & 255);
        dl_py[i] = rng() & 127;
        dl_x[i] = dl_px[i] + (rng() & 127) - 60;
        dl_y[i] = dl_py[i] + (rng() & 127) - 60;
        gl_x9[i] = 0;
    }
    gl_cam = 30;
    gl_row = 0;
    gl_changed = 0;
}

void k_glide_c(void) {
    u16 i, id, d, r = gl_row, cx = gl_cam, ch = gl_changed;
    s16 sx, sy;
    for (i = 0, id = 0; i < ND; i++, id += 4) {
        d = dl_x[i] - dl_px[i] + 48;
        sx = d <= 96 ? dl_px[i] + glide_tab[r + d] : dl_x[i];
        d = dl_y[i] - dl_py[i] + 48;
        sy = d <= 96 ? dl_py[i] + glide_tab[r + d] : dl_y[i];
        sx -= cx;
        gl_oam[id] = sx;
        gl_oam[id + 1] = sy;
        if ((sx & 0x100) != gl_x9[i]) {
            gl_x9[i] = sx & 0x100;
            ch++;
        }
    }
    gl_changed = ch;
    gl_row = 97 - r;
    gl_cam = cx + 9;
}

u16 c_glide(void) { return sum(gl_oam, ND * 4) + sum((const u8 *)gl_x9, sizeof(gl_x9)) + gl_changed; }

/* ---- entry: the driver is bench_run in kernels.asm; these are its variables */

u16 bench_i, bench_k, bench_n;
u16 bench_ptr[2];
u16 bench_sum[32];

extern void bench_run(void);

int main(void) {
    bench_run();
    return 0;
}
