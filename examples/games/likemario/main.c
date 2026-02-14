/**
 * LikeMario - Side-scrolling platformer
 *
 * Port of PVSnesLib's likemario example to OpenSNES.
 * Demonstrates: tile-based map scrolling, sprite animation,
 * physics with gravity, tile collision, camera tracking.
 *
 * Controls:
 *   Left/Right - Move Mario
 *   A          - Jump (hold UP for high jump)
 */

#include <snes.h>
#include <snes/snesmod.h>
#include "soundbank.h"

/*============================================================================
 * External Data (from data.asm)
 *============================================================================*/

extern u8 tiles_til[], tiles_tilend[];
extern u8 tiles_pal[];
extern u8 mario_sprite_til[];
extern u8 mario_sprite_pal[];
extern u8 mapmario[];
extern u8 tilesetatt[];

/*============================================================================
 * Constants
 *============================================================================*/

/* VRAM Layout */
#define VRAM_SPR_LARGE  0x0000
#define VRAM_SPR_SMALL  0x1000
#define VRAM_BG_TILES   0x2000
#define VRAM_BG_MAP     0x6800

/* Physics (from PVSnesLib) */
#define GRAVITY         48
#define MARIO_MAXACCEL  0x0140
#define MARIO_ACCEL     0x0038
#define MARIO_JUMPING   0x0394
#define MARIO_HIJUMPING 0x0594

/* Tile properties (b16 format: u16 per tile) */
#define T_EMPTY  0x0000
#define T_SOLID  0xFF00

/* Mario states */
#define ACT_STAND  0
#define ACT_WALK   1
#define ACT_JUMP   2
#define ACT_FALL   3

/* Sprite frame indices */
#define FRAME_JUMP   1
#define FRAME_WALK0  2
#define FRAME_WALK1  3
#define FRAME_STAND  6

/*============================================================================
 * Global State
 *============================================================================*/

static u16 map_width;
static u16 map_height;
static u16 *map_data;
static u16 *tile_props;

/* Precomputed row pointers for fast tile lookup (avoids multiply) */
static u16 *map_row_ptrs[32];

static s16 camera_x;
static s16 last_tile_x;

static s16 mario_x, mario_y;
static u8  mario_xfrac, mario_yfrac;
static s16 mario_xvel, mario_yvel;
static u8  mario_action;
static u8  mario_anim_idx;
static u8  anim_tick;

/* Cached max X for boundary checks */
static s16 map_max_x;
static s16 cam_max_x;

/* Column buffer for deferred VRAM write */
static u16 col_buffer[32];
static u16 col_vram_base;
static u8  col_pending;

/*============================================================================
 * Map Engine
 *============================================================================*/

static u16 map_get_tile_prop(s16 px, s16 py) {
    u16 tx, ty;

    if (px < 0 || py < 0) return T_SOLID;

    tx = (u16)px >> 3;
    ty = (u16)py >> 3;

    if (tx >= map_width || ty >= map_height) return T_EMPTY;

    return tile_props[map_row_ptrs[ty][tx] & 0x03FF];
}

static void write_vram_column(u16 map_col, u16 vram_col) {
    u16 base, row, tile;

    if (map_col >= map_width) return;

    if (vram_col < 32)
        base = VRAM_BG_MAP + vram_col;
    else
        base = VRAM_BG_MAP + 0x0400 + (vram_col - 32);

    /* Caller must ensure force blank or VBlank */
    REG_VMAIN = 0x81;
    REG_VMADDL = base & 0xFF;
    REG_VMADDH = base >> 8;

    for (row = 0; row < map_height; row++) {
        tile = map_row_ptrs[row][map_col];
        REG_VMDATAL = tile & 0xFF;
        REG_VMDATAH = tile >> 8;
    }
    for (; row < 32; row++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0;
    }

    REG_VMAIN = 0x80;
}

static void map_load(void) {
    u16 col, row;
    u16 *hdr, *ptr;

    hdr = (u16 *)mapmario;
    map_width = hdr[0] >> 3;
    map_height = hdr[1] >> 3;
    map_data = &hdr[3];

    tile_props = (u16 *)tilesetatt;

    /* Build row pointer table for fast access */
    ptr = map_data;
    for (row = 0; row < map_height; row++) {
        map_row_ptrs[row] = ptr;
        ptr += map_width;
    }

    map_max_x = (s16)(map_width * 8 - 16);
    cam_max_x = (s16)(map_width * 8 - 256);

    col_pending = 0;

    /* Force blank is already set by consoleInit — safe for VRAM writes */
    for (col = 0; col < 64; col++) {
        if (col < map_width)
            write_vram_column(col, col);
    }

    camera_x = 0;
    last_tile_x = 0;
}

static void map_prepare_column(u16 map_col, u16 vram_col) {
    u16 row;

    if (map_col >= map_width) return;

    /* Fill buffer during active display (RAM-only, no VRAM access) */
    for (row = 0; row < map_height; row++)
        col_buffer[row] = map_row_ptrs[row][map_col];
    for (; row < 32; row++)
        col_buffer[row] = 0;

    if (vram_col < 32)
        col_vram_base = VRAM_BG_MAP + vram_col;
    else
        col_vram_base = VRAM_BG_MAP + 0x0400 + (vram_col - 32);

    col_pending = 1;
}

static void map_flush_column(void) {
    if (!col_pending) return;

    /* Set VRAM address and vertical column mode (increment by 32 words) */
    REG_VMAIN = 0x81;
    REG_VMADDL = col_vram_base & 0xFF;
    REG_VMADDH = col_vram_base >> 8;

    /* DMA channel 1: transfer col_buffer (64 bytes) to VRAM
     * Using DMA instead of C loop — the compiled register-write loop
     * takes ~57,600 master cycles (exceeds VBlank), while DMA takes ~512. */
    REG_DMAP(1) = 0x01;            /* 2-register word mode ($2118/$2119) */
    REG_BBAD(1) = 0x18;            /* Destination: VMDATAL */
    REG_A1TL(1) = (u16)col_buffer & 0xFF;
    REG_A1TH(1) = ((u16)col_buffer >> 8) & 0xFF;
    REG_A1B(1)  = 0x7E;            /* Source bank: WRAM */
    REG_DASL(1) = 64;              /* 32 words = 64 bytes */
    REG_DASH(1) = 0;
    REG_MDMAEN  = 0x02;            /* Fire DMA channel 1 */

    REG_VMAIN = 0x80;
    col_pending = 0;
}

static void map_update(void) {
    s16 tile_x;
    u16 new_col, vram_col;

    tile_x = camera_x >> 3;

    /* Prepare at most 1 column per frame (buffered for VBlank flush).
     * Stream column at +32 (one ahead of visible edge) so partial
     * tiles at the right edge are always ready. */
    if (last_tile_x < tile_x) {
        last_tile_x++;
        new_col = last_tile_x + 32;
        if (new_col < map_width) {
            vram_col = new_col & 63;
            map_prepare_column(new_col, vram_col);
        }
    } else if (last_tile_x > tile_x) {
        last_tile_x--;
        new_col = last_tile_x;
        vram_col = new_col & 63;
        map_prepare_column(new_col, vram_col);
    }
}

/*============================================================================
 * Mario - Split into small functions to keep stack frames manageable
 *============================================================================*/

static void mario_init(void) {
    mario_x = 48;
    mario_y = 96;       /* Ground level (row 14 = tile 40 SOLID at y=112, so feet at 112 → y=96) */
    mario_xfrac = 0;
    mario_yfrac = 0;
    mario_xvel = 0;
    mario_yvel = 0;
    mario_action = ACT_STAND;
    mario_anim_idx = 0;
    anim_tick = 0;

    oambuffer[0].oamframeid = FRAME_STAND;
    oambuffer[0].oamrefresh = 1;
    oambuffer[0].oamattribute = OBJ_PRIO(3) | 0x40;
    OAM_SET_GFX(0, mario_sprite_til);
}

static void mario_handle_input(void) {
    u16 pad;

    pad = padHeld(0);

    if (pad & KEY_LEFT) {
        oambuffer[0].oamattribute = OBJ_PRIO(3);
        if (mario_action == ACT_STAND)
            mario_action = ACT_WALK;
        mario_xvel -= MARIO_ACCEL;
        if (mario_xvel < -MARIO_MAXACCEL)
            mario_xvel = -MARIO_MAXACCEL;
    } else if (pad & KEY_RIGHT) {
        oambuffer[0].oamattribute = OBJ_PRIO(3) | 0x40;
        if (mario_action == ACT_STAND)
            mario_action = ACT_WALK;
        mario_xvel += MARIO_ACCEL;
        if (mario_xvel > MARIO_MAXACCEL)
            mario_xvel = MARIO_MAXACCEL;
    } else {
        if (mario_xvel > 0) {
            mario_xvel -= MARIO_ACCEL;
            if (mario_xvel < 0) mario_xvel = 0;
        } else if (mario_xvel < 0) {
            mario_xvel += MARIO_ACCEL;
            if (mario_xvel > 0) mario_xvel = 0;
        }
    }

    if (padPressed(0) & KEY_A) {
        if (mario_action == ACT_STAND || mario_action == ACT_WALK) {
            mario_action = ACT_JUMP;
            if (pad & KEY_UP)
                mario_yvel = -MARIO_HIJUMPING;
            else
                mario_yvel = -MARIO_JUMPING;
        }
    }
}

/*
 * Arithmetic right shift by 8 — workaround for compiler using LSR (logical)
 * instead of ASR (arithmetic) for signed shifts.
 * For positive: normal unsigned shift.
 * For negative: invert, shift, invert back (preserves sign correctly).
 */
static s16 asr8(s16 val) {
    if (val >= 0)
        return (u16)val >> 8;
    return ~(((u16)~val) >> 8);
}

static void mario_apply_physics(void) {
    s16 new_frac;

    if (mario_action == ACT_JUMP || mario_action == ACT_FALL) {
        mario_yvel += GRAVITY;
        if (mario_yvel > 0x0400)
            mario_yvel = 0x0400;
    }

    new_frac = (s16)mario_xfrac + mario_xvel;
    mario_x += asr8(new_frac);
    mario_xfrac = new_frac & 0xFF;

    new_frac = (s16)mario_yfrac + mario_yvel;
    mario_y += asr8(new_frac);
    mario_yfrac = new_frac & 0xFF;
}

static void mario_collide_vertical(void) {
    u16 prop;

    /* Ground check */
    if (mario_yvel >= 0) {
        prop = map_get_tile_prop(mario_x + 2, mario_y + 16);
        if (prop == T_EMPTY)
            prop = map_get_tile_prop(mario_x + 13, mario_y + 16);

        if (prop != T_EMPTY) {
            mario_y = ((mario_y + 16) & 0xFFF8) - 16;
            mario_yfrac = 0;
            mario_yvel = 0;
            if (mario_action == ACT_FALL || mario_action == ACT_JUMP)
                mario_action = ACT_STAND;
        } else {
            if (mario_action == ACT_STAND || mario_action == ACT_WALK)
                mario_action = ACT_FALL;
        }
    }

    /* Ceiling check */
    if (mario_yvel < 0) {
        prop = map_get_tile_prop(mario_x + 8, mario_y - 1);
        if (prop != T_EMPTY) {
            mario_y = (mario_y & 0xFFF8) + 8;
            mario_yfrac = 0;
            mario_yvel = 0;
            mario_action = ACT_FALL;
        }
    }
}

static void mario_collide_horizontal(void) {
    u16 prop;
    s16 mid_y;

    mid_y = mario_y + 8;

    if (mario_xvel > 0) {
        prop = map_get_tile_prop(mario_x + 15, mid_y);
        if (prop != T_EMPTY) {
            mario_x = ((mario_x + 15) & 0xFFF8) - 16;
            mario_xfrac = 0;
            mario_xvel = 0;
        }
    }

    if (mario_xvel < 0) {
        prop = map_get_tile_prop(mario_x, mid_y);
        if (prop != T_EMPTY) {
            mario_x = (mario_x & 0xFFF8) + 8;
            mario_xfrac = 0;
            mario_xvel = 0;
        }
    }
}

static void mario_clamp_and_transition(void) {
    if (mario_x < 0) {
        mario_x = 0;
        mario_xvel = 0;
    }
    if (mario_x > map_max_x) {
        mario_x = map_max_x;
        mario_xvel = 0;
    }
    if (mario_y < 0) {
        mario_y = 0;
        mario_yvel = 0;
    }
    if (mario_y > 240) {
        mario_x = 48;
        mario_y = 32;
        mario_xvel = 0;
        mario_yvel = 0;
        mario_action = ACT_FALL;
    }

    if (mario_action == ACT_WALK && mario_xvel == 0)
        mario_action = ACT_STAND;
    if (mario_action == ACT_JUMP && mario_yvel >= 0)
        mario_action = ACT_FALL;
}

static void mario_animate(void) {
    anim_tick++;

    if (mario_action == ACT_WALK) {
        if ((anim_tick & 3) == 3) {
            mario_anim_idx ^= 1;
            oambuffer[0].oamframeid = FRAME_WALK0 + mario_anim_idx;
            oambuffer[0].oamrefresh = 1;
        }
    } else if (mario_action == ACT_JUMP || mario_action == ACT_FALL) {
        if (oambuffer[0].oamframeid != FRAME_JUMP) {
            oambuffer[0].oamframeid = FRAME_JUMP;
            oambuffer[0].oamrefresh = 1;
        }
    } else {
        if (oambuffer[0].oamframeid != FRAME_STAND) {
            oambuffer[0].oamframeid = FRAME_STAND;
            oambuffer[0].oamrefresh = 1;
        }
    }
}

static void mario_update_camera(void) {
    camera_x = mario_x - 128;
    if (camera_x < 0) camera_x = 0;
    if (camera_x > cam_max_x)
        camera_x = cam_max_x;

    oambuffer[0].oamx = mario_x - camera_x;
    oambuffer[0].oamy = mario_y;
    oamDynamic16Draw(0);
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    consoleInit();      /* Sets force blank ON — stays until setScreenOn */

    setMode(BG_MODE1, 0);

    bgSetGfxPtr(0, VRAM_BG_TILES);
    bgSetMapPtr(0, VRAM_BG_MAP, SC_64x32);

    bgInitTileSet(0, tiles_til, tiles_pal, 0,
                  (u16)(tiles_tilend - tiles_til),
                  16 * 2, BG_16COLORS, VRAM_BG_TILES);

    oamInitDynamicSprite(VRAM_SPR_LARGE, VRAM_SPR_SMALL, 0, 0, OBJ_SIZE8_L16);
    dmaCopyCGram(mario_sprite_pal, 128, 16 * 2);

    map_load();     /* All VRAM writes safe — force blank from consoleInit */
    mario_init();

    /* Initialize SNESMOD audio */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);
    snesmodLoadModule(MOD_OVERWORLD);

    REG_TM = TM_BG1 | TM_OBJ;

    /* Initial sprite draw + upload (still in force blank from consoleInit) */
    oambuffer[0].oamx = mario_x;
    oambuffer[0].oamy = mario_y;
    oamDynamic16Draw(0);
    oamVramQueueUpdate();
    oamInitDynamicSpriteEndFrame();

    setScreenOn();      /* Release force blank — display begins */

    snesmodPlay(0);
    snesmodSetModuleVolume(100);

    WaitForVBlank();

    while (1) {
        mario_handle_input();
        mario_apply_physics();
        mario_collide_vertical();
        mario_collide_horizontal();
        mario_clamp_and_transition();
        mario_animate();
        mario_update_camera();
        map_update();

        oamInitDynamicSpriteEndFrame();

        WaitForVBlank();
        /* VRAM operations first — must complete within VBlank */
        map_flush_column();
        bgSetScroll(0, camera_x, 0);
        oamVramQueueUpdate();
        /* SPC700 communication last — no VBlank restriction */
        snesmodProcess();
    }

    return 0;
}
