/**
 * @file main.c
 * @brief shmup_1942 — S4: enemy spawns with a sprite pool
 * @ingroup examples
 *
 * Adds a fixed-size enemy pool on top of S3's player + scrolling. Up to
 * eight red chasseurs spawn from above the screen at pseudo-random X
 * positions, drift downward, and despawn when they exit the bottom.
 * The player ship (blue, S3) is unchanged and still controllable.
 *
 * Architecture: the player and enemies share OAM but use distinct
 * VRAM tile regions ($6000 for player, $6800 for enemy) and distinct
 * sprite palettes (0 and 1). Each on-screen enemy is one OAM entry
 * referencing the SAME 16 enemy tiles — the pool is in CPU state, not
 * a per-enemy VRAM copy.
 *
 * @par SNES Concepts
 * - VRAM region planning for two sprite types in the same OAM tile
 *   grid: player at tile 0 (16 tiles), enemy at tile 64 (next
 *   OAM-grid row of 32×32 sprites, 4 rows × 16 tiles down)
 * - Sprite palette 0 = CGRAM 128-143 (player), palette 1 = 144-159
 *   (enemy). `dmaCopyCGram(enemy_pal, 144, ...)` loads it during
 *   force blank.
 * - Sprite pool: 8 OAM entries reserved for enemies, hidden via the
 *   off-screen Y trick (set Y to OBJ_HIDE_Y when inactive)
 * - LCG pseudo-random for spawn X position
 *
 * @par What to Observe
 * - Blue player ship responds to D-pad (same as S3)
 * - Red enemies spawn from above the screen every second and drift
 *   downward at 2 px/frame
 * - Up to 8 enemies on screen simultaneously
 * - Background continues scrolling at 1 px/frame
 *
 * @par Modules Used
 * console, dma, background, asset, sprite, input
 *
 * @see background.h, asset.h, sprite.h, input.h, dma.h
 */

#include <snes.h>
#include <snes/asset.h>
#include <snes/input.h>

/** Composed scene bundle (4bpp, 16 colours, 32×64 tilemap). */
DECLARE_BG_ASSET(scene, BG_16COLORS, SC_32x64);

extern u8 player_tiles[], player_tiles_end[];
extern u8 player_pal[], player_pal_end[];
extern u8 enemy_tiles[], enemy_tiles_end[];
extern u8 enemy_pal[], enemy_pal_end[];

#define MAP_HEIGHT_PX  512
#define PLAYER_SPEED   2

#define PLAYER_MIN_X   8
#define PLAYER_MAX_X   216
#define PLAYER_MIN_Y   16
#define PLAYER_MAX_Y   176

#define MAX_ENEMIES        8
#define ENEMY_OAM_BASE     1            /**< OAM slot 0 is player; 1..8 are enemies */
#define ENEMY_TILE         64           /**< OBJ tile index for enemy sprite (= VRAM word $6400) */
#define ENEMY_PALETTE      1            /**< Sprite palette 1 = CGRAM 144-159 */
#define ENEMY_SPEED        2            /**< px/frame downward (faster than BG scroll) */
#define ENEMY_SPAWN_PERIOD 30           /**< frames between spawns at full pool */
#define ENEMY_HIDE_Y       240          /**< sprite Y value that hides it (below visible area) */
#define ENEMY_SPRITE       32           /**< full sprite extent (32×32) */

typedef struct {
    s16 x;
    s16 y;
    u8  active;
} Enemy;

typedef struct {
    s16 player_x;
    s16 player_y;
    u16 scroll_y;
    u16 frame;
    u16 prng;
    Enemy enemies[MAX_ENEMIES];
} GameState;

static GameState game;

/** 16-bit LCG. Galois constants from Knuth — cheap and good enough for
 *  spawn jitter on the 65816. */
static u16 prng_next(void) {
    game.prng = (u16)(game.prng * 25173u + 13849u);
    return game.prng;
}

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
            /* Range: [8, 8 + 208) = [8, 216) so the 32-px sprite stays
             * inside the visible 256-px screen */
            game.enemies[i].x = (s16)((r % 208u) + 8u);
            game.enemies[i].y = -ENEMY_SPRITE;
            game.enemies[i].active = 1;
            return;
        }
    }
}

static void enemies_update(void) {
    for (u8 i = 0; i < MAX_ENEMIES; i++) {
        if (!game.enemies[i].active) continue;
        game.enemies[i].y += ENEMY_SPEED;
        if (game.enemies[i].y >= 224) {
            game.enemies[i].active = 0;
            game.enemies[i].y = ENEMY_HIDE_Y;
        }
    }
}

static void enemies_render(void) {
    /* oamSet() has framesize=158 per call; calling it nine times per frame
     * (1 player + 8 enemies) blows past the documented 2-3 jitter limit.
     * oamSetFast() is a macro with zero stack overhead and is the
     * recommended API for per-frame sprite updates. */
    for (u8 i = 0; i < MAX_ENEMIES; i++) {
        u16 ex, ey;
        if (game.enemies[i].active) {
            ex = (u16)game.enemies[i].x;
            ey = (u16)game.enemies[i].y;
        } else {
            ex = 0;
            ey = ENEMY_HIDE_Y;
        }
        oamSetFast(ENEMY_OAM_BASE + i, ex, ey, ENEMY_TILE, ENEMY_PALETTE, 2, 0);
    }
}

int main(void) {
    setScreenOff();

    /* BG1: tilemap at VRAM $0000, tiles at $4000, palette slot 0. */
    bgLoad(0, &scene, 0, 0x4000, 0x0000);

    /* Player tiles → VRAM $6000, palette 0 (CGRAM 128-143).
     * OBJ_SIZE32_L64 makes "small" = 32×32 = our sprite size. */
    oamInitGfxSet(player_tiles, (u16)(player_tiles_end - player_tiles),
                  player_pal,   (u16)(player_pal_end   - player_pal),
                  0, 0x6000, OBJ_SIZE32_L64);

    /* Enemy tiles → VRAM word $6400. dmaCopyVram takes a WORD address
     * for VMADDR; OBJ tile N has VRAM word offset N×16 from the OBJ
     * tile base ($6000 here). So tile 64 = word $6000 + 64×16 = $6400.
     * One 2-KB chunk holds the 16 actual sprite tiles (padded to 64
     * slots for the OAM 16-tile-row stride); every enemy OAM entry
     * references this same data, varying only x/y. */
    dmaCopyVram(enemy_tiles, 0x6400, (u16)(enemy_tiles_end - enemy_tiles));

    /* Enemy palette → CGRAM 144 (sprite palette 1, 16 colours). */
    dmaCopyCGram(enemy_pal, 144, (u16)(enemy_pal_end - enemy_pal));

    setMode(BG_MODE1, 0);
    setMainScreen(TM_BG1 | TM_OBJ);

    game.player_x = 112;
    game.player_y = 160;
    game.scroll_y = 0;
    game.frame    = 0;
    game.prng     = 0xACE1;  /* arbitrary non-zero seed */
    enemies_init();

    /* Hide every enemy OAM entry before display goes live */
    oamSet(0, game.player_x, game.player_y, 0, 0, 2, 0);
    enemies_render();
    bgSetScroll(0, 0, 0);

    setScreenOn();

    while (1) {
        WaitForVBlank();

        game.frame++;
        game.scroll_y = (game.scroll_y + 1) % MAP_HEIGHT_PX;

        u16 pad = padHeld(0);
        if (pad & KEY_LEFT  && game.player_x > PLAYER_MIN_X) game.player_x -= PLAYER_SPEED;
        if (pad & KEY_RIGHT && game.player_x < PLAYER_MAX_X) game.player_x += PLAYER_SPEED;
        if (pad & KEY_UP    && game.player_y > PLAYER_MIN_Y) game.player_y -= PLAYER_SPEED;
        if (pad & KEY_DOWN  && game.player_y < PLAYER_MAX_Y) game.player_y += PLAYER_SPEED;

        if (game.frame % ENEMY_SPAWN_PERIOD == 0) {
            enemy_spawn();
        }
        enemies_update();

        oamSet(0, game.player_x, game.player_y, 0, 0, 2, 0);
        enemies_render();
        bgSetScroll(0, 0, game.scroll_y);
    }

    return 0;
}
