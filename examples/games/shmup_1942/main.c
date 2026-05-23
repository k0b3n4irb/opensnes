/**
 * @file main.c
 * @brief shmup_1942 — S5: bullets, collision and enemy destruction
 * @ingroup examples
 *
 * Adds player bullets and bullet↔enemy AABB collision on top of S4.
 * Pressing A fires a bullet upward (with a small cooldown). Bullets
 * travel at 4 px/frame; when a bullet's bounding box overlaps an
 * enemy, both deactivate. The scene composition, scroll direction
 * and enemy spawn logic are unchanged from S4.
 *
 * @par SNES Concepts
 * - Sprite pools as fixed-size arrays of `{x, y, active}` slots
 *   (player gets OAM slot 0, enemies 1-8, bullets 9-16)
 * - Multiple sprite types in the same OAM grid: distinct VRAM regions
 *   (player @ $6000, enemy @ $6400, bullet @ $6800 — all word
 *   addresses) plus distinct sprite palettes (0/1/2)
 * - Axis-aligned bounding-box (AABB) overlap test for 32×32 sprites
 * - Input edge detection via `padPressed()` for one-shot fire
 *
 * @par What to Observe
 * - Press A to fire a bullet upward from the player's nose
 * - Bullets travel at 4 px/frame; they despawn at the top of the
 *   screen or on hitting an enemy
 * - Enemies hit by a bullet vanish immediately (S5.1 will add
 *   explosion animations)
 * - The terrain keeps scrolling and the enemy pool keeps spawning
 *
 * @par Modules Used
 * console, dma, background, asset, sprite, input
 *
 * @see background.h, asset.h, sprite.h, input.h, dma.h
 */

#include <snes.h>
#include <snes/asset.h>
#include <snes/input.h>

DECLARE_BG_ASSET(scene, BG_16COLORS, SC_32x64);

extern u8 player_tiles[], player_tiles_end[];
extern u8 player_pal[], player_pal_end[];
extern u8 enemy_tiles[], enemy_tiles_end[];
extern u8 enemy_pal[], enemy_pal_end[];
extern u8 bullet_tiles[], bullet_tiles_end[];
extern u8 bullet_pal[], bullet_pal_end[];

#define MAP_HEIGHT_PX  512
#define PLAYER_SPEED   2

#define PLAYER_MIN_X   8
#define PLAYER_MAX_X   216
#define PLAYER_MIN_Y   16
#define PLAYER_MAX_Y   176

#define MAX_ENEMIES        8
#define ENEMY_OAM_BASE     1
#define ENEMY_TILE         64           /* OBJ tile 64 = VRAM word $6400 */
#define ENEMY_PALETTE      1
#define ENEMY_SPEED        2
#define ENEMY_SPAWN_PERIOD 32      /* power of 2 → mask, not modulo (cc65816 has no fast %) */
#define ENEMY_SPAWN_MASK   (ENEMY_SPAWN_PERIOD - 1)
#define ENEMY_HIDE_Y       240
#define ENEMY_SPRITE       32

#define MAX_BULLETS        8
#define BULLET_OAM_BASE    (ENEMY_OAM_BASE + MAX_ENEMIES)  /* = 9 */
#define BULLET_TILE        128          /* OBJ tile 128 = VRAM word $6800 */
#define BULLET_PALETTE     2
#define BULLET_SPEED       4
#define BULLET_HIDE_Y      240
#define BULLET_SPRITE      32           /* full 32×32 OAM slot — visible bullet is centred top */
#define FIRE_COOLDOWN      8            /* frames between consecutive shots */

/* Pre-computed OAM attribute bytes for direct-write rendering. Matches
 * breakout's pattern: write OAM bytes directly into oamMemory[] each
 * frame, avoiding even the oamSetFast macro's per-call work. Constants
 * computed from OAM_ATTR(tile, pal, prio, flags). */
#define PLAYER_ATTR        0x20         /* tile 0,   pal 0, prio 2, no flip */
#define ENEMY_ATTR         0xA2         /* tile 64,  pal 1, prio 2, V-flip  */
#define BULLET_ATTR        0x24         /* tile 128, pal 2, prio 2, no flip */

/* Direct oamMemory[] write: 4 bytes per sprite, no X-high bit (all our
 * sprites stay at X < 256), no oam_update_flag (set once at end of
 * frame). This is the fastest possible sprite update on this SDK —
 * same pattern breakout uses for its 10-sprite playfield. */
#define OAM_WRITE(_id, _x, _y, _tile, _attr) do { \
    u16 _o = (u16)(_id) << 2; \
    oamMemory[_o + 0] = (u8)(_x); \
    oamMemory[_o + 1] = (u8)((_y) - 1); \
    oamMemory[_o + 2] = (u8)(_tile); \
    oamMemory[_o + 3] = (_attr); \
} while(0)

typedef struct {
    s16 x;
    s16 y;
    u8  active;
} Entity;

typedef struct {
    s16 player_x;
    s16 player_y;
    u16 scroll_y;
    u16 frame;
    u16 prng;
    u8  fire_cd;
    Entity enemies[MAX_ENEMIES];
    Entity bullets[MAX_BULLETS];
} GameState;

static GameState game;

static u16 prng_next(void) {
    game.prng = (u16)(game.prng * 25173u + 13849u);
    return game.prng;
}

/* ---- Enemy pool ----------------------------------------------------------*/

static void enemies_init(void) {
    for (u8 i = 0; i < MAX_ENEMIES; i++) {
        game.enemies[i].active = 0;
        game.enemies[i].x = 0;
        game.enemies[i].y = ENEMY_HIDE_Y;
    }
}

static void enemy_spawn(void) {
    for (u8 i = 0; i < MAX_ENEMIES; i++) {
        if (!game.enemies[i].active) {
            u16 r = prng_next();
            game.enemies[i].x = (s16)((r % 208u) + 8u);
            game.enemies[i].y = -ENEMY_SPRITE;
            game.enemies[i].active = 1;
            return;
        }
    }
}

/* On transition active → inactive, write Y=HIDE_Y once via direct
 * oamMemory[] write. Subsequent frames don't touch this slot — the
 * OAM byte sticks until the slot becomes active again. Same trick
 * for bullets. */
static void enemy_hide(u8 i) {
    OAM_WRITE(ENEMY_OAM_BASE + i, 0, ENEMY_HIDE_Y, ENEMY_TILE, ENEMY_ATTR);
}

static void enemies_update(void) {
    for (u8 i = 0; i < MAX_ENEMIES; i++) {
        if (!game.enemies[i].active) continue;
        game.enemies[i].y += ENEMY_SPEED;
        if (game.enemies[i].y >= 224) {
            game.enemies[i].active = 0;
            game.enemies[i].y = ENEMY_HIDE_Y;
            enemy_hide(i);
        }
    }
}

static void enemies_render(void) {
    /* Direct oamMemory[] writes — breakout's pattern. The base offset
     * for OAM entry N is N*4. Slot 1 → bytes 4..7, slot 2 → 8..11, etc.
     * We use a running pointer to dodge cc65816's per-iteration index
     * recomputation (each `i*4` inside a loop expands to a multiply). */
    u16 off = (u16)ENEMY_OAM_BASE << 2;   /* = 4 (slot 1) */
    for (u8 i = 0; i < MAX_ENEMIES; i++, off += 4) {
        if (!game.enemies[i].active) continue;
        oamMemory[off + 0] = (u8)game.enemies[i].x;
        oamMemory[off + 1] = (u8)(game.enemies[i].y - 1);
        oamMemory[off + 2] = ENEMY_TILE;
        oamMemory[off + 3] = ENEMY_ATTR;
    }
}

/* ---- Bullet pool ---------------------------------------------------------*/

static void bullets_init(void) {
    for (u8 i = 0; i < MAX_BULLETS; i++) {
        game.bullets[i].active = 0;
        game.bullets[i].x = 0;
        game.bullets[i].y = BULLET_HIDE_Y;
    }
    game.fire_cd = 0;
}

static void bullet_fire(void) {
    if (game.fire_cd != 0) return;
    for (u8 i = 0; i < MAX_BULLETS; i++) {
        if (!game.bullets[i].active) {
            /* Spawn just above the player ship's nose. The visible
             * bullet pixel column lives at canvas x=14..17 of its 32×32
             * sprite, so subtracting 16 from the player centre lands
             * the muzzle on the ship's nose. */
            /* Spawn at the player's top edge, not 16 px above. The visible
             * ball lives at canvas y 4-11, so the muzzle appears just
             * above the player's nose. The previous -16 offset meant
             * bullets fired while the player was near the top of the
             * screen spawned at y≈0 and were gone in two frames — felt
             * like the bullet's lifetime was "computed at fire time" but
             * was really just a spawn-position artefact. */
            game.bullets[i].x = game.player_x;
            game.bullets[i].y = game.player_y;
            game.bullets[i].active = 1;
            game.fire_cd = FIRE_COOLDOWN;
            return;
        }
    }
}

static void bullet_hide(u8 i) {
    OAM_WRITE(BULLET_OAM_BASE + i, 0, BULLET_HIDE_Y, BULLET_TILE, BULLET_ATTR);
}

static void bullets_update(void) {
    if (game.fire_cd > 0) game.fire_cd--;
    for (u8 i = 0; i < MAX_BULLETS; i++) {
        if (!game.bullets[i].active) continue;
        game.bullets[i].y -= BULLET_SPEED;
        /* Despawn one frame after the visible ball clears the top.
         * Natural SNES sprite wrap keeps the ball visible at the top
         * of the screen for sprite_y down to -10 (the ball's bottom
         * pixel at world scanline 0). Below that the ball is in the
         * 224-255 hidden Y zone — no render gymnastics needed. */
        if (game.bullets[i].y < -10) {
            game.bullets[i].active = 0;
            game.bullets[i].y = BULLET_HIDE_Y;
            bullet_hide(i);
        }
    }
}

static void bullets_render(void) {
    u16 off = (u16)BULLET_OAM_BASE << 2;
    for (u8 i = 0; i < MAX_BULLETS; i++, off += 4) {
        if (!game.bullets[i].active) continue;
        oamMemory[off + 0] = (u8)game.bullets[i].x;
        oamMemory[off + 1] = (u8)(game.bullets[i].y - 1);
        oamMemory[off + 2] = BULLET_TILE;
        oamMemory[off + 3] = BULLET_ATTR;
    }
}

/* ---- Collision -----------------------------------------------------------*/

/* Point-in-rect collision. The bullet's VISIBLE pixels are an 8×8 ball
 * at canvas (12,4)..(19,11) — centre at (16, 8). The enemy ship's
 * visible silhouette doesn't quite fill its 32×32 canvas, so we use an
 * inset 20×20 hit-box anchored at (ex+6, ey+6). A hit is registered
 * when the bullet's centre point falls inside the enemy hit-box —
 * tight enough that bullets must visibly touch the ship, cheap enough
 * to run 64× per frame on a 65816. */
#define BULLET_CX_OFFSET 16
#define BULLET_CY_OFFSET 8
#define ENEMY_HITBOX_INSET 6
#define ENEMY_HITBOX_END   (32 - ENEMY_HITBOX_INSET)

static void collisions_resolve(void) {
    for (u8 b = 0; b < MAX_BULLETS; b++) {
        if (!game.bullets[b].active) continue;
        s16 bcx = game.bullets[b].x + BULLET_CX_OFFSET;
        s16 bcy = game.bullets[b].y + BULLET_CY_OFFSET;
        for (u8 e = 0; e < MAX_ENEMIES; e++) {
            if (!game.enemies[e].active) continue;
            s16 ex = game.enemies[e].x;
            s16 ey = game.enemies[e].y;
            if (bcx <= ex + ENEMY_HITBOX_INSET) continue;
            if (bcx >= ex + ENEMY_HITBOX_END)   continue;
            if (bcy <= ey + ENEMY_HITBOX_INSET) continue;
            if (bcy >= ey + ENEMY_HITBOX_END)   continue;
            /* Hit! */
            game.bullets[b].active = 0;
            game.bullets[b].y = BULLET_HIDE_Y;
            bullet_hide(b);
            game.enemies[e].active = 0;
            game.enemies[e].y = ENEMY_HIDE_Y;
            enemy_hide(e);
            break;
        }
    }
}

/* ---- Main ----------------------------------------------------------------*/

int main(void) {
    setScreenOff();

    bgLoad(0, &scene, 0, 0x4000, 0x0000);

    oamInitGfxSet(player_tiles, (u16)(player_tiles_end - player_tiles),
                  player_pal,   (u16)(player_pal_end   - player_pal),
                  0, 0x6000, OBJ_SIZE32_L64);

    /* Enemy tiles at VRAM word $6400 (= OBJ tile 64), palette 1. */
    dmaCopyVram(enemy_tiles, 0x6400, (u16)(enemy_tiles_end - enemy_tiles));
    dmaCopyCGram(enemy_pal, 144, (u16)(enemy_pal_end - enemy_pal));

    /* Bullet tiles at VRAM word $6800 (= OBJ tile 128), palette 2. */
    dmaCopyVram(bullet_tiles, 0x6800, (u16)(bullet_tiles_end - bullet_tiles));
    dmaCopyCGram(bullet_pal, 160, (u16)(bullet_pal_end - bullet_pal));

    setMode(BG_MODE1, 0);
    setMainScreen(TM_BG1 | TM_OBJ);

    game.player_x = 112;
    game.player_y = 160;
    game.scroll_y = 0;
    game.frame    = 0;
    game.prng     = 0xACE1;
    enemies_init();
    bullets_init();

    /* Clear the OAM extension table bits for the slots we render.
     *
     * Background: oamClear (called by oamInit) leaves the ext table at
     * 0x55 = X-high bit set on every sprite, which combined with Y=240
     * pushes them fully off-screen (32×32 sprites at Y=240 otherwise
     * wrap their bottom half back to the top of the screen — see the
     * crt0.asm note above the ext-table init).
     *
     * Our direct oamMemory[] writes for enemies/bullets don't touch
     * the ext table, so they'd inherit X-high=1 and render at X+256.
     * We clear the bits for slots 0..16 only (player + enemies +
     * bullets). Slots 17..127 stay at 0x55 → invisible.
     *
     * Ext table layout: 1 byte per 4 sprites, 2 bits per sprite
     * (bit 0 = X-high, bit 1 = size). Slots 0-15 = bytes 512-515
     * (clear fully). Slot 16 = bits 0-1 of byte 516. */
    oamMemory[512] = 0;     /* slots 0-3 */
    oamMemory[513] = 0;     /* slots 4-7 */
    oamMemory[514] = 0;     /* slots 8-11 */
    oamMemory[515] = 0;     /* slots 12-15 */
    oamMemory[516] &= 0xFC; /* clear slot 16's two bits, preserve 17-19 */

    /* Hide every enemy and bullet OAM slot once at boot. */
    for (u8 i = 0; i < MAX_ENEMIES; i++) enemy_hide(i);
    for (u8 i = 0; i < MAX_BULLETS; i++) bullet_hide(i);

    oamSet(0, game.player_x, game.player_y, 0, 0, 2, 0);
    bgSetScroll(0, 0, 0);

    setScreenOn();

    while (1) {
        WaitForVBlank();

        game.frame++;
        if (game.scroll_y == 0) game.scroll_y = MAP_HEIGHT_PX;
        game.scroll_y--;

        u16 pad = padHeld(0);
        if (pad & KEY_LEFT  && game.player_x > PLAYER_MIN_X) game.player_x -= PLAYER_SPEED;
        if (pad & KEY_RIGHT && game.player_x < PLAYER_MAX_X) game.player_x += PLAYER_SPEED;
        if (pad & KEY_UP    && game.player_y > PLAYER_MIN_Y) game.player_y -= PLAYER_SPEED;
        if (pad & KEY_DOWN  && game.player_y < PLAYER_MAX_Y) game.player_y += PLAYER_SPEED;
        if (pad & KEY_A) bullet_fire();

        if ((game.frame & ENEMY_SPAWN_MASK) == 0) enemy_spawn();
        enemies_update();
        bullets_update();
        collisions_resolve();

        oamSet(0, (u16)game.player_x, (u16)game.player_y, 0, 0, 2, 0);
        enemies_render();
        bullets_render();
        oam_update_flag = 1;  /* re-arm after our direct writes */
        bgSetScroll(0, 0, game.scroll_y);
    }

    return 0;
}
