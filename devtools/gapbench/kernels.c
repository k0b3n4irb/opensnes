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

/* ======== the rows below have the shapes shipped games have (README.md) == */

/* ---- slots: one pass over 64 object slots of 16 bytes, 56 of them empty.
 * What an empty slot costs: a game pays it for every slot, every pass. ---- */

typedef struct {
    u8 kind, state;
    u16 count;
    s16 x, y;
    u16 spare[4];
} Slot;

Slot slots[64];
u16 sl_live;

void s_slots(void) {
    u16 i, k;
    rng_state = 10;
    for (i = 0; i < 64; i++) {
        slots[i].kind = rng();
        slots[i].state = (i & 7) == 5;
        slots[i].count = rng() & 255;
        slots[i].x = rng();
        slots[i].y = rng();
        for (k = 0; k < 4; k++)
            slots[i].spare[k] = 0;
    }
    sl_live = 0;
}

void k_slots_c(void) {
    Slot *s = slots;
    u16 i, n = 0;
    for (i = 0; i < 64; i++, s++) {
        if (!s->state)
            continue;
        s->count++;
        n++;
    }
    sl_live += n;
}

u16 c_slots(void) { return sum((const u8 *)slots, sizeof(slots)) + sl_live; }

/* ---- state: 32 objects, a state 0..7 each, eight small handlers that
 * change the object and sometimes its state. A dense switch. ------------- */

typedef struct {
    u8 state, timer;
    s16 x, y;
    u16 val;
} StObj;

StObj st_obj[32];

void s_state(void) {
    u16 i;
    rng_state = 11;
    for (i = 0; i < 32; i++) {
        st_obj[i].state = rng() & 7;
        st_obj[i].timer = 1 + (rng() & 7);
        st_obj[i].x = 40 + (rng() & 15);
        st_obj[i].y = rng() & 15;
        st_obj[i].val = rng();
    }
}

void k_state_c(void) {
    u16 i;
    for (i = 0; i < 32; i++) {
        StObj *o = &st_obj[i];
        switch (o->state) {
        case 0:
            o->x += 2;
            if (o->x >= 56)
                o->state = 1;
            break;
        case 1:
            o->y += 1;
            if (--o->timer == 0) {
                o->timer = 5;
                o->state = 2;
            }
            break;
        case 2:
            o->x -= 3;
            if (o->x < 44)
                o->state = 3;
            break;
        case 3:
            o->val += o->x;
            o->state = 4;
            break;
        case 4:
            if (o->timer)
                o->timer--;
            else
                o->state = 5;
            break;
        case 5:
            o->y -= 2;
            if (o->y < 0) {
                o->y = 0;
                o->state = 6;
            }
            break;
        case 6:
            o->val ^= 0x5555;
            o->state = 7;
            break;
        default:
            o->timer = o->val & 7;
            o->state = 0;
            break;
        }
    }
}

u16 c_state(void) { return sum((const u8 *)st_obj, sizeof(st_obj)); }

/* ---- fptr: the same eight handlers, the state being the handler itself:
 * a function pointer kept in the object. --------------------------------- */

typedef struct FObj {
    void (*fn)(struct FObj *);
    s16 x, y;
    u16 val;
    u8 timer, pad;
} FObj;

FObj fp_obj[32];

void fp_h0(FObj *o);
void fp_h1(FObj *o);
void fp_h2(FObj *o);
void fp_h3(FObj *o);
void fp_h4(FObj *o);
void fp_h5(FObj *o);
void fp_h6(FObj *o);
void fp_h7(FObj *o);

void fp_h0(FObj *o) {
    o->x += 2;
    if (o->x >= 56)
        o->fn = fp_h1;
}

void fp_h1(FObj *o) {
    o->y += 1;
    if (--o->timer == 0) {
        o->timer = 5;
        o->fn = fp_h2;
    }
}

void fp_h2(FObj *o) {
    o->x -= 3;
    if (o->x < 44)
        o->fn = fp_h3;
}

void fp_h3(FObj *o) {
    o->val += o->x;
    o->fn = fp_h4;
}

void fp_h4(FObj *o) {
    if (o->timer)
        o->timer--;
    else
        o->fn = fp_h5;
}

void fp_h5(FObj *o) {
    o->y -= 2;
    if (o->y < 0) {
        o->y = 0;
        o->fn = fp_h6;
    }
}

void fp_h6(FObj *o) {
    o->val ^= 0x5555;
    o->fn = fp_h7;
}

void fp_h7(FObj *o) {
    o->timer = o->val & 7;
    o->fn = fp_h0;
}

typedef void (*FpFn)(FObj *);
const FpFn fp_c_tab[8] = {fp_h0, fp_h1, fp_h2, fp_h3, fp_h4, fp_h5, fp_h6, fp_h7};
extern const FpFn fp_asm_tab[8];    /* the assembly twin's handlers (kernels.asm) */

/* Which of the eight handlers an object holds, whichever twin wrote it. */
static u16 fp_index(const FObj *o) {
    u16 k;
    for (k = 0; k < 8; k++)
        if (o->fn == fp_c_tab[k] || o->fn == fp_asm_tab[k])
            return k;
    return 99;
}

void s_fptr(void) {
    u16 i;
    rng_state = 11;
    for (i = 0; i < 32; i++) {
        fp_obj[i].fn = fp_c_tab[rng() & 7];
        fp_obj[i].timer = 1 + (rng() & 7);
        fp_obj[i].x = 40 + (rng() & 15);
        fp_obj[i].y = rng() & 15;
        fp_obj[i].val = rng();
        fp_obj[i].pad = 0;
    }
}

/* The same objects holding the assembly twin's handlers. */
void s_fptra(void) {
    u16 i;
    s_fptr();
    for (i = 0; i < 32; i++)
        fp_obj[i].fn = fp_asm_tab[fp_index(&fp_obj[i])];
}

void k_fptr_c(void) {
    u16 i;
    for (i = 0; i < 32; i++) {
        FObj *o = &fp_obj[i];
        o->fn(o);
    }
}

u16 c_fptr(void) {
    u16 i, s = 0;
    for (i = 0; i < 32; i++)
        s = ((s << 3) | (s >> 13)) + sum((const u8 *)&fp_obj[i].x, 8) + fp_index(&fp_obj[i]);
    return s;
}

/* ---- blocks: a row of 16 blocks of 16x16 pixels expanded to two rows of 32
 * tilemap entries, from a map of words (block number in the low bits, flags
 * above) and four words a block. The row is a ring of 32 entries: where it
 * starts moves with the map column. --------------------------------------- */

#define BK_BLOCK 0x03FF
#define BK_HAZARD 0x4000
#define BK_SOLID 0x8000

FAR u16 bk_map[64 * 16];
FAR u16 bk_def[128 * 4];
u16 bk_top[32], bk_bot[32];
u16 bk_x, bk_y;

static void bk_fill(void) {
    u16 i;
    for (i = 0; i < 64 * 16; i++) {
        u16 w = rng() & 127;
        w |= (rng() & 3) << 14;
        w |= (rng() & 1) << 11;
        bk_map[i] = w;
    }
    for (i = 0; i < 128 * 4; i++)
        bk_def[i] = rng();
}

void s_blocks(void) {
    u16 i;
    rng_state = 13;
    bk_fill();
    for (i = 0; i < 32; i++)
        bk_top[i] = bk_bot[i] = 0;
    bk_x = 3;
    bk_y = 2;
}

void k_blocks_c(void) {
    u16 c, d = (bk_x * 2) & 31;
    u16 FAR *m = &bk_map[bk_y * 64 + bk_x];
    for (c = 0; c < 16; c++) {
        u16 b = (m[c] & BK_BLOCK) * 4;
        bk_top[d] = bk_def[b];
        bk_top[d + 1] = bk_def[b + 1];
        bk_bot[d] = bk_def[b + 2];
        bk_bot[d + 1] = bk_def[b + 3];
        d = (d + 2) & 31;
    }
    bk_x = (bk_x + 1) & 31;
    bk_y = (bk_y + 1) & 15;
}

u16 c_blocks(void) {
    return sum((const u8 *)bk_top, 64) + sum((const u8 *)bk_bot, 64) + bk_x + bk_y;
}

/* ---- probe: 24 points in pixels against the map of words above: the cell,
 * its flags, a result byte each and the count. The map is 64 blocks wide. -- */

u16 pr_x[24], pr_y[24];
u8 pr_hit[24];
u16 pr_hits;

void s_probe(void) {
    u16 i;
    rng_state = 14;
    bk_fill();
    for (i = 0; i < 24; i++) {
        pr_x[i] = rng() & 1023;
        pr_y[i] = rng() & 255;
        pr_hit[i] = 0;
    }
    pr_hits = 0;
}

void k_probe_c(void) {
    u16 i, n = 0;
    for (i = 0; i < 24; i++) {
        u16 x = pr_x[i];
        u16 w = bk_map[(pr_y[i] >> 4) * 64 + (x >> 4)];
        u8 r = 0;
        if (w & BK_SOLID)
            r = 1;
        else if (w & BK_HAZARD)
            r = 2;
        if (r)
            n++;
        pr_hit[i] = r;
        pr_x[i] = (x + 5) & 1023;
    }
    pr_hits += n;
}

u16 c_probe(void) { return sum(pr_hit, 24) + sum((const u8 *)pr_x, 48) + pr_hits; }

/* ---- chain: 16 actors, an update each that calls four or five functions of
 * a line or two, between a few lines of its own: the weight calls have in a
 * game's logic. The functions are global so that they stay calls. ----------------------------------------- */

#define CH_MOVE 1
#define CH_LOOP 2
#define CH_BLINK 4

typedef struct {
    u16 x, v, flags, frame, life;
    s16 y, vy;
    u16 tick, rate, score, cell;
} Act;

Act ch_act[16];
u16 ch_loops;

void s_chain(void) {
    u16 i;
    rng_state = 15;
    for (i = 0; i < 16; i++) {
        ch_act[i].x = rng() & 255;
        ch_act[i].v = rng() & 7;
        ch_act[i].flags = rng() & 3;
        ch_act[i].frame = rng() & 3;
        ch_act[i].life = 1 + (rng() & 15);
        ch_act[i].y = 20 + (rng() & 127);
        ch_act[i].vy = (s16)(rng() & 15) - 8;
        ch_act[i].tick = rng() & 3;
        ch_act[i].rate = 1 + (rng() & 3);
        ch_act[i].score = 0;
        ch_act[i].cell = 0;
    }
    ch_loops = 0;
}

u16 ch_test(u16 flags, u16 mask) { return flags & mask; }

u16 ch_clamp(u16 v, u16 max) { return v > max ? max : v; }

u16 ch_anim(u16 frame) {
    frame++;
    if (frame >= 6)
        frame = 0;
    return frame;
}

u16 ch_decay(u16 life) { return life ? life - 1 : 0; }

void ch_update(Act *a) {
    s16 y;
    if (ch_test(a->flags, CH_MOVE))
        a->x = ch_clamp(a->x + a->v, 300);
    a->frame = ch_anim(a->frame);
    if (a->frame == 0)
        a->flags ^= CH_BLINK;
    y = a->y + a->vy;
    if (y < 20 || y > 200)
        a->vy = -a->vy;
    else
        a->y = y;
    a->life = ch_decay(a->life);
    if (a->life == 0 && ch_test(a->flags, CH_LOOP)) {
        a->life = 20;
        a->x = 0;
        ch_loops++;
    }
    if (a->tick) {
        a->tick--;
    } else {
        a->tick = a->rate;
        a->score += a->frame + 1;
    }
    a->cell = (a->x >> 4) + (a->y & 0xF0) + ((a->flags & CH_BLINK) << 6);
}

void k_chain_c(void) {
    u16 i;
    for (i = 0; i < 16; i++)
        ch_update(&ch_act[i]);
}

u16 c_chain(void) { return sum((const u8 *)ch_act, sizeof(ch_act)) + ch_loops; }

/* ---- sprites: 12 objects of 1 to 4 tiles (a table of tile offsets a
 * frame), world to screen, each tile culled, the ninth x bit, written to
 * the sprite list in depth order (y ascending; the order is kept from call
 * to call and put right by one insertion pass), the rest hidden. ---------- */

#define NM 12

typedef struct {
    s16 x, y, vy;
    u8 frame, attr;
} Mob;

Mob mobs[NM];
u8 mo_order[NM];
u8 mo_oam[NM * 16];
u8 mo_hi[NM];
s16 mo_cam_x, mo_cam_y;

/* A frame: the tile count, three unused bytes, then x offset, y offset and
 * tile number of each of up to four tiles. */
const u8 mo_frames[8 * 16] = {
    1, 0, 0, 0, 0, 0, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 0, 0, 0, 0, 0, 0x02, 16, 0, 0x04, 0, 0, 0, 0, 0, 0,
    2, 0, 0, 0, 0, 0, 0x06, 0, 16, 0x26, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0x08, 16, 0, 0x0A, 0, 16, 0x28, 16, 16, 0x2A,
    3, 0, 0, 0, 8, 0, 0x0C, 0, 16, 0x2C, 16, 16, 0x2E, 0, 0, 0,
    1, 0, 0, 0, 4, 4, 0x40, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0x42, 16, 0, 0x44, 0, 16, 0x62, 16, 16, 0x64,
    2, 0, 0, 0, 0, 8, 0x46, 16, 8, 0x48, 0, 0, 0, 0, 0, 0};

void s_sprites(void) {
    u16 i;
    rng_state = 16;
    for (i = 0; i < NM; i++) {
        mobs[i].x = rng() & 255;
        mobs[i].y = 60 + (rng() & 255);
        mobs[i].vy = (s16)(rng() & 7) - 3;
        mobs[i].frame = rng() & 7;
        mobs[i].attr = rng();
        mo_order[i] = i;
    }
    mo_cam_x = 44;
    mo_cam_y = 30;
}

void k_sprites_c(void) {
    u16 i, k, o = 0;
    s16 cx = mo_cam_x, cy = mo_cam_y;
    for (i = 1; i < NM; i++) {
        u8 id = mo_order[i];
        s16 y = mobs[id].y;
        for (k = i; k > 0 && mobs[mo_order[k - 1]].y > y; k--)
            mo_order[k] = mo_order[k - 1];
        mo_order[k] = id;
    }
    for (i = 0; i < NM; i++)
        mo_hi[i] = 0;
    for (i = 0; i < NM; i++) {
        Mob *m = &mobs[mo_order[i]];
        s16 sx = m->x - cx, sy = m->y - cy;
        u16 f = m->frame * 16;
        u16 n = mo_frames[f];
        u8 attr = m->attr;
        f += 4;
        do {
            s16 tx = sx + mo_frames[f];
            s16 ty = sy + mo_frames[f + 1];
            if (tx > -16 && tx < 256 && ty > -16 && ty < 224) {
                mo_oam[o] = tx;
                mo_oam[o + 1] = ty;
                mo_oam[o + 2] = mo_frames[f + 2];
                mo_oam[o + 3] = attr;
                if (tx & 0x100)
                    mo_hi[o >> 4] |= hi_bit[(o >> 2) & 3];
                o += 4;
            }
            f += 3;
        } while (--n);
        m->y += m->vy;
    }
    for (; o < NM * 16; o += 4)
        mo_oam[o + 1] = 240;
    mo_cam_x = cx + 2;
}

u16 c_sprites(void) {
    return sum(mo_oam, NM * 16) + sum(mo_hi, NM) + sum(mo_order, NM)
         + sum((const u8 *)mobs, sizeof(mobs)) + mo_cam_x;
}

/* ---- unpack: 512 bytes out of a byte-wise LZ stream. A token under $80 is
 * that many literals plus one; a token from $80 is a copy of its low seven
 * bits plus three bytes, from the output, the next byte plus one back. ---- */

FAR u8 un_src[512];
FAR u8 un_dst[512];

void s_unpack(void) {
    u16 s = 0, out = 0, n, d, i;
    rng_state = 17;
    while (out < 512) {
        n = 1 + (rng() & 7);
        if (n > 512 - out)
            n = 512 - out;
        un_src[s++] = n - 1;
        for (i = 0; i < n; i++)
            un_src[s++] = rng();
        out += n;
        if (512 - out < 3)
            continue;
        n = 3 + (rng() & 15);
        if (n > 512 - out)
            n = 512 - out;
        d = 1 + (rng() & 255);
        if (d > out)
            d = out;
        un_src[s++] = 0x80 | (n - 3);
        un_src[s++] = d - 1;
        out += n;
    }
    for (i = 0; i < 512; i++)
        un_dst[i] = 0;
}

void k_unpack_c(void) {
    const u8 FAR *s = un_src;
    u8 FAR *d = un_dst;
    u16 left = 512, n;
    while (left) {
        u8 t = *s++;
        if (t < 0x80) {
            n = t + 1;
            left -= n;
            do {
                *d++ = *s++;
            } while (--n);
        } else {
            const u8 FAR *p = d - 1 - *s++;
            n = (t & 0x7F) + 3;
            left -= n;
            do {
                *d++ = *p++;
            } while (--n);
        }
    }
}

u16 c_unpack(void) { return sum((const u8 *)un_dst, 512); }

/* ---- entry: the driver is bench_run in kernels.asm; these are its variables */

u16 bench_i, bench_k, bench_n;
u16 bench_ptr[2];
u16 bench_sum[64];          /* one a row of bench_table (kernels.asm), room to grow */

extern void bench_run(void);

int main(void) {
    bench_run();
    return 0;
}
