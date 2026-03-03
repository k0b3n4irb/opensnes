/**
 * @file main.c
 * @brief Breakout Game - A Complete SNES Game Example
 *
 * This game demonstrates core SNES development concepts:
 *
 * VIDEO SYSTEM:
 * - Mode 1 (most common SNES mode): BG1 4bpp, BG3 2bpp, sprites
 * - VRAM tilemap overlap to save memory
 * - Palette cycling for level progression
 *
 * SPRITES:
 * - Multi-sprite paddle (4 tiles with shadows)
 * - Ball with shadow for depth effect
 * - Secondary name table access (tile | 256)
 *
 * TILEMAPS:
 * - RAM buffer pattern for runtime modification
 * - Bricks as background tiles (not sprites)
 * - Text rendering via tilemap writes
 *
 * DMA:
 * - Atomic VRAM updates for overlapping tilemaps
 * - Palette DMA for color cycling
 *
 * Based on SNES SDK game by Ulrich Hecht
 * PVSnesLib port by alekmaul
 * OpenSNES port
 */

#include <snes.h>

/*============================================================================
 * External Assets (defined in data.asm)
 *
 * Assets are compiled into the ROM and accessed via these extern declarations.
 * The _end labels allow calculating sizes: (tiles1_end - tiles1) = byte count
 *============================================================================*/

extern u8 tiles1[], tiles1_end[];   /* BG tiles: border, bricks, font (4bpp) */
extern u8 tiles2[], tiles2_end[];   /* Sprite tiles: ball, paddle (4bpp) */
extern u8 bg1map[], bg1map_end[];   /* BG1 tilemap: playfield border */
extern u8 bg2map[], bg2map_end[];   /* BG3 tilemap: all 4 background patterns */
extern u8 bg2map0[], bg2map1[], bg2map2[], bg2map3[];  /* Individual patterns */
extern u8 palette[], palette_end[]; /* Full 256-color palette */
extern u8 backpal[], backpal_end[]; /* 7 level color sets (7 x 16 bytes) */

/* Input buffer - NMI handler reads joypads and stores here every frame */
extern u16 pad_keys[];

/* oamMemory[] and oam_update_flag declared in <snes/system.h> (via <snes.h>) */

/*============================================================================
 * Constants
 *============================================================================*/

/* Default brick layout (10x10 grid, 100 bytes) - defined in data.asm
 * Values 0-7 = brick color/type, 8 = no brick */
extern const u8 brick_map[];

/* Status message strings - defined in data.asm to ensure bank 0 placement */
extern const char str_ready[];
extern const char str_gameover[];
extern const char str_paused[];
extern const char str_blank[];

#define ST_READY    str_ready
#define ST_GAMEOVER str_gameover
#define ST_PAUSED   str_paused
#define ST_BLANK    str_blank

/*============================================================================
 * RAM Buffers (defined in data.asm)
 *
 * These buffers are placed at specific addresses to avoid WRAM mirroring
 * issues. Bank 0 addresses $0000-$1FFF mirror Bank $7E:$0000-$1FFF, so
 * we place these at $0800+ to avoid overlap with OAM buffer at $0300.
 *
 * Why RAM buffers?
 * - Tilemaps need runtime modification (brick destruction, score updates)
 * - Palette needs modification (level color cycling)
 * - ROM is read-only, so we copy to RAM and modify there
 *============================================================================*/

extern u16 blockmap[];  /* BG1 tilemap copy - 0x400 entries (2KB) at $0800 */
extern u16 backmap[];   /* BG3 tilemap copy - 0x400 entries (2KB) at $1000 */
extern u16 pal[];       /* Palette copy - 0x100 entries (512 bytes) at $1800 */
extern u8  blocks[];    /* Brick state array - 100 bytes */

/*============================================================================
 * Game State Variables
 *
 * Static variables without initializers are zero-initialized per C standard.
 * Initialized statics (e.g., "static u8 x = 5;") also work correctly -
 * their values are copied from ROM to RAM at startup.
 *============================================================================*/

static u8  i, j, k;          /* Loop variables (global to reduce stack usage) */
static u16 a, c, b;          /* Brick init temporaries */
static u16 blockcount;       /* Bricks remaining (0 = level complete) */
static u16 bx, by;           /* Ball position in brick grid coords */
static u16 obx, oby;         /* Previous grid position (for bounce direction) */
static u16 score, hiscore;   /* Current and high score */
static u16 level2;           /* Display level number (starts at 1) */
static u16 color;            /* Background color index (0-6, cycles each level) */
static u16 level;            /* Internal level counter (for scoring multiplier) */
static u16 lives;            /* Lives remaining */
static u16 px;               /* Paddle X position (16-144 range) */
static u16 pad0;             /* Current frame's joypad state */

/* Ball velocity and position as separate vars (not struct) to avoid
 * compiler issues with struct member operations */
static s16 vel_x, vel_y;     /* Ball velocity (-2 to +2 each axis) */
static s16 pos_x, pos_y;     /* Ball pixel position */

/*============================================================================
 * Utility Functions
 *============================================================================*/

/**
 * @brief Write a null-terminated string to a tilemap buffer
 *
 * Converts ASCII characters to tile indices by adding an offset.
 * Supports newline (\n) for multi-line text.
 *
 * @param st      Null-terminated string
 * @param tilemap Pointer to tilemap buffer (u16 per entry)
 * @param pos     Starting position in tilemap (0-0x3FF for 32x32)
 * @param offset  Value added to ASCII code to get tile index
 *
 * Tilemap entry format (16-bit):
 *   Bits 0-9:   Tile number (0-1023)
 *   Bits 10-12: Palette (0-7)
 *   Bit 13:     Priority
 *   Bit 14:     H-flip
 *   Bit 15:     V-flip
 */
static void writestring(const char *st, u16 *tilemap, u16 pos, u16 offset) {
    u16 sp = pos;  /* Start position for newline handling */
    u8 ch;

    while ((ch = *st)) {
        if (ch == '\n') {
            sp += 0x20;   /* Next row = +32 entries */
            pos = sp;
        } else {
            tilemap[pos] = ch + offset;
            pos++;
        }
        st++;
    }
}

/**
 * @brief Write a right-aligned number to a tilemap buffer
 *
 * @param num     Number to display
 * @param len     Field width (pads with spaces on left)
 * @param tilemap Pointer to tilemap buffer
 * @param pos     Starting position (leftmost digit position)
 * @param offset  Value added to digit (0-9) to get tile index
 */
static void writenum(u16 num, u8 len, u16 *tilemap, u16 pos, u16 offset) {
    u8 figure;
    pos += len - 1;  /* Start at rightmost position */

    if (!num) {
        tilemap[pos] = offset;  /* Just show "0" */
    } else {
        while (len && num) {
            figure = num % 10;
            if (num || figure)
                tilemap[pos] = figure + offset;
            num /= 10;
            pos--;
            len--;
        }
    }
}

/**
 * @brief Simple byte-by-byte memory copy
 *
 * Used instead of library memcpy to avoid bank addressing complexity.
 * Works for Bank 0 addresses only (ROM $8000+ and RAM $0000-$1FFF).
 */
static void mycopy(u8 *dest, const u8 *src, u16 len) {
    while (len--) {
        *dest++ = *src++;
    }
}

/*============================================================================
 * Sprite Rendering
 *============================================================================*/

/**
 * @brief Update all sprite positions
 *
 * SPRITE ORGANIZATION:
 * This game uses 10 hardware sprites for the ball and paddle.
 * Sprite 0 is skipped due to a corruption issue (possibly WRAM mirroring).
 *
 * Sprite Assignment:
 *   Sprite 1:     Ball
 *   Sprites 2-5:  Paddle (left cap, 2x middle mirrored, right cap)
 *   Sprite 6:     Ball shadow (offset +3,+3 pixels, lower priority)
 *   Sprites 7-10: Paddle shadow (offset +4,+4 pixels, lower priority)
 *
 * TILE NUMBERING:
 * Sprite tiles are at VRAM $2000 (secondary name table).
 * To access secondary table, set bit 8 of tile number: tile | 256
 *
 * Tile Layout in tiles2:
 *   Tile 15: Paddle left cap
 *   Tile 16: Paddle middle (used twice, second time H-flipped)
 *   Tile 17: Paddle right cap
 *   Tile 18: Paddle shadow left
 *   Tile 19: Paddle shadow middle
 *   Tile 20: Ball
 *   Tile 21: Ball shadow
 *
 * SHADOW TECHNIQUE:
 * Shadows are separate sprites with:
 * - Same shape but darker color (shadow palette)
 * - Offset position (+3 or +4 pixels down-right)
 * - Lower priority (1 vs 3) so they appear behind main sprites
 */
static void draw_screen(void) {
    /*
     * Direct OAM buffer writes — replaces 10x oamSet() calls.
     * oamSet() has framesize=158 per call; 10 calls = 1580 bytes stack overhead.
     *
     * OAM low table: 4 bytes per sprite at offset id*4
     *   [0] X low, [1] Y, [2] tile low, [3] attributes (vhoopppc)
     *
     * Attribute byte precomputed constants:
     *   0x31 = priority 3, palette 0, tile high bit 1 (secondary name table)
     *   0x71 = H-flip + priority 3, palette 0, tile high bit 1
     *   0x11 = priority 1, palette 0, tile high bit 1 (shadows)
     *   0x51 = H-flip + priority 1, palette 0, tile high bit 1
     */

    /* Sprite 1: Ball */
    oamMemory[4]  = (u8)pos_x;
    oamMemory[5]  = (u8)pos_y;
    oamMemory[6]  = 20;    /* tile 20 */
    oamMemory[7]  = 0x31;

    /* Sprite 2: Paddle left cap */
    oamMemory[8]  = (u8)px;
    oamMemory[9]  = 200;
    oamMemory[10] = 15;    /* tile 15 */
    oamMemory[11] = 0x31;

    /* Sprite 3: Paddle middle left */
    oamMemory[12] = (u8)(px + 8);
    oamMemory[13] = 200;
    oamMemory[14] = 16;    /* tile 16 */
    oamMemory[15] = 0x31;

    /* Sprite 4: Paddle middle right (H-flip) */
    oamMemory[16] = (u8)(px + 16);
    oamMemory[17] = 200;
    oamMemory[18] = 16;    /* tile 16 */
    oamMemory[19] = 0x71;

    /* Sprite 5: Paddle right cap */
    oamMemory[20] = (u8)(px + 24);
    oamMemory[21] = 200;
    oamMemory[22] = 17;    /* tile 17 */
    oamMemory[23] = 0x31;

    /* Sprite 6: Ball shadow */
    oamMemory[24] = (u8)(pos_x + 3);
    oamMemory[25] = (u8)(pos_y + 3);
    oamMemory[26] = 21;    /* tile 21 */
    oamMemory[27] = 0x11;

    /* Sprite 7: Paddle shadow left */
    oamMemory[28] = (u8)(px + 4);
    oamMemory[29] = 204;
    oamMemory[30] = 18;    /* tile 18 */
    oamMemory[31] = 0x11;

    /* Sprite 8: Paddle shadow middle left */
    oamMemory[32] = (u8)(px + 12);
    oamMemory[33] = 204;
    oamMemory[34] = 19;    /* tile 19 */
    oamMemory[35] = 0x11;

    /* Sprite 9: Paddle shadow middle right (H-flip) */
    oamMemory[36] = (u8)(px + 20);
    oamMemory[37] = 204;
    oamMemory[38] = 19;    /* tile 19 */
    oamMemory[39] = 0x51;

    /* Sprite 10: Paddle shadow right */
    oamMemory[40] = (u8)(px + 28);
    oamMemory[41] = 204;
    oamMemory[42] = 18;    /* tile 18 */
    oamMemory[43] = 0x11;

    oam_update_flag = 1;
}

/*============================================================================
 * Level Management
 *============================================================================*/

/**
 * @brief Initialize a new level
 *
 * This function handles level transitions:
 * 1. Reset ball/paddle positions
 * 2. Reset background tilemap and brick array from ROM
 * 3. Cycle background color
 * 4. Rebuild brick wall in existing BG1 tilemap (preserves HUD)
 * 5. Upload everything to VRAM
 * 6. Wait for player to press START
 *
 * CRITICAL DMA LESSON:
 * BG1 tilemap (0x0000-0x07FF) and BG3 tilemap (0x0400-0x0BFF) OVERLAP
 * in VRAM at addresses 0x0400-0x07FF. Both tilemaps must be uploaded
 * in the SAME VBlank, or the intermediate frame shows corruption.
 *
 * This was a hard-learned lesson: splitting "to be safe" actually
 * caused a visible pink flash. PVSnesLib does 4.5KB+ in one VBlank.
 */
static void new_level(void) {
    level++;
    level2++;
    pos_x = 94;
    pos_y = 109;
    px = 80;

    /* Select background pattern for this level (cycles through 4) */
    switch (level & 3) {
        case 0: mycopy((u8 *)backmap, bg2map0, 0x800); break;
        case 1: mycopy((u8 *)backmap, bg2map1, 0x800); break;
        case 2: mycopy((u8 *)backmap, bg2map2, 0x800); break;
        case 3: mycopy((u8 *)backmap, bg2map3, 0x800); break;
    }
    mycopy((u8 *)blocks, brick_map, 100);

    /* Update level display in tilemap */
    writenum(level2, 8, blockmap, 0x2D6, 0x426);
    writestring(ST_READY, blockmap, 0x248, 0x3F6);

    /* Cycle background color (0-6, wraps to 0) */
    if (color < 6) color++;
    else color = 0;

    /* PALETTE CYCLING:
     * backpal.dat contains 7 sets of 8 colors (7 x 16 bytes).
     * Each set replaces CGRAM colors 8-15 (byte offset 16).
     * This changes the background pattern color each level. */
    mycopy((u8 *)pal + 16, backpal + color * 16, 0x10);

    /* BRICK INITIALIZATION:
     * Each brick is 2 tiles wide. The brick array contains color values 0-7.
     * Value 8 means "no brick". Tilemap entries include palette bits.
     *
     * Tilemap entry format: tile_number | (palette << 10)
     * Tiles 13-14 are the left/right halves of a brick.
     */
    b = 0;
    for (j = 0; j < 10; j++) {          /* 10 rows */
        for (i = 0; i < 20; i += 2) {   /* 10 bricks per row (2 tiles each) */
            a = blocks[b];
            if (a < 8) {                /* Brick exists */
                c = (j << 5) + i;       /* Tilemap offset: row*32 + column */
                blockcount++;
                /* Brick tiles with palette (a << 10 sets palette bits) */
                blockmap[0x62 + c] = 13 + (a << 10);
                blockmap[0x63 + c] = 14 + (a << 10);
                /* Shadow effect: increase palette index of BG3 tiles behind brick */
                backmap[0x83 + c] += 0x400;
                backmap[0x84 + c] += 0x400;
            }
            b++;
        }
    }

    /* FORCE BLANK for bulk VRAM update:
     * Total: 512 + 2048 + 2048 = 4608 bytes — exceeds VBlank budget after
     * NMI handler overhead (OAM DMA + joypad read ~13K cycles). Force blank
     * disables rendering so VRAM writes are accepted at any time. */
    WaitForVBlank();
    setScreenOff();
    dmaCopyCGram((u8 *)pal, 0, 256 * 2);
    dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);
    dmaCopyVram((u8 *)backmap, 0x0400, 0x800);
    setScreenOn();

    draw_screen();

    /* Wait for START to begin */
    do {
        WaitForVBlank();
    } while ((pad_keys[0] & KEY_START) == 0);

    /* Wait for START release to prevent immediate pause */
    do {
        WaitForVBlank();
    } while (pad_keys[0] & KEY_START);

    /* Clear READY message */
    writestring(ST_BLANK, blockmap, 0x248, 0x3F6);
    writestring(ST_BLANK, blockmap, 0x289, 0x3F6);
    WaitForVBlank();
    dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);
}

/**
 * @brief Handle player losing a life
 */
static void die(void) {
    if (lives == 0) {
        /* Game over - display message and halt */
        writestring(ST_GAMEOVER, blockmap, 0x267, 0x3F6);
        WaitForVBlank();
        dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);
        while (1) { WaitForVBlank(); }
    }

    /* Reset ball and paddle, decrement lives */
    lives--;
    pos_x = 94;
    pos_y = 109;
    px = 80;

    /* Clear message area (preserve border columns) */
    for (i = 2; i < 22; i++) {
        blockmap[0x240 + i] = 0;
        blockmap[0x260 + i] = 0;
        blockmap[0x280 + i] = 0;
    }

    /* Update display */
    writenum(lives, 8, blockmap, 0x136, 0x426);
    writestring(ST_READY, blockmap, 0x248, 0x3F6);
    WaitForVBlank();
    dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);

    draw_screen();

    /* Wait for any button press */
    while (pad_keys[0] == 0) {
        WaitForVBlank();
    }

    /* Clear message and continue */
    writestring(ST_BLANK, blockmap, 0x248, 0x3F6);
    writestring(ST_BLANK, blockmap, 0x289, 0x3F6);
    WaitForVBlank();
    dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);
}

/*============================================================================
 * Input and Game Logic
 *============================================================================*/

/**
 * @brief Handle pause functionality
 */
static void handle_pause(void) {
    if ((pad0 & KEY_START) != 0) {
        writestring(ST_PAUSED, blockmap, 0x269, 0x3F6);
        WaitForVBlank();
        dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);

        /* Wait for START release, press, release sequence */
        do { WaitForVBlank(); } while (pad_keys[0] & KEY_START);
        do { WaitForVBlank(); } while ((pad_keys[0] & KEY_START) == 0);
        do { WaitForVBlank(); } while (pad_keys[0] & KEY_START);

        writestring(ST_BLANK, blockmap, 0x269, 0x3F6);
        WaitForVBlank();
        dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);
    }
}

/**
 * @brief Handle paddle movement from joypad input
 *
 * Paddle moves at 2 pixels/frame normally, 4 with A held.
 * Clamped to playfield boundaries (16-144).
 */
static void move_paddle(void) {
    u16 input = pad0 & (KEY_LEFT | KEY_RIGHT | KEY_A);
    u16 speed = (input & KEY_A) ? 4 : 2;

    if (input & KEY_RIGHT) {
        if (px < 144) px += speed;
        if (px > 144) px = 144;
    }
    if (input & KEY_LEFT) {
        if (px > 16 + speed) px -= speed;
        else px = 16;
    }
}

/**
 * @brief Update ball position and handle wall collision
 *
 * NOTE: Uses explicit temp variables instead of direct assignment
 * to work around QBE compiler issues with compound operations.
 */
static void move_ball(void) {
    /* Explicit temp vars avoid compiler issues with += */
    s16 new_x = pos_x + vel_x;
    s16 new_y = pos_y + vel_y;
    pos_x = new_x;
    pos_y = new_y;

    /* Right/left wall collision */
    if (pos_x > 171) {
        s16 neg_vx = -vel_x;  /* Temp var for negation */
        vel_x = neg_vx;
        pos_x = 171;
    } else if (pos_x < 16) {
        s16 neg_vx = -vel_x;
        vel_x = neg_vx;
        pos_x = 16;
    }

    /* Top wall collision */
    if (pos_y < 15) {
        s16 neg_vy = -vel_y;
        vel_y = neg_vy;
    }
}

/**
 * @brief Check collision with paddle
 *
 * BOUNCE PHYSICS:
 * Ball bounce angle depends on where it hits the paddle:
 *   Left edge:  Sharp left angle  (vel_x = -2, vel_y = -1)
 *   Left-mid:   Slight left       (vel_x = -1, vel_y = -2)
 *   Right-mid:  Slight right      (vel_x = +1, vel_y = -2)
 *   Right edge: Sharp right angle (vel_x = +2, vel_y = -1)
 *
 * This creates the classic breakout gameplay where paddle position
 * matters for aiming the ball.
 */
static void check_paddle(void) {
    if (pos_y > 195 && pos_y < 203) {
        if ((pos_x >= px) && (pos_x <= px + 27)) {
            /* Calculate hit zone (0-3) */
            k = (pos_x - px) / 7;

            /* Set bounce velocity based on hit zone */
            switch (k) {
                case 0: vel_x = -2; vel_y = -1; break;
                case 1: vel_x = -1; vel_y = -2; break;
                case 2: vel_x =  1; vel_y = -2; break;
                default: vel_x = 2; vel_y = -1; break;
            }
        }
    } else if (pos_y > 224) {
        /* Ball fell below paddle */
        die();
    }
}

/**
 * @brief Remove a brick and update score
 *
 * When a brick is destroyed:
 * 1. Update score (brick value + 1, multiplied by level)
 * 2. Reverse ball velocity based on collision axis
 * 3. Clear brick from tilemap
 * 4. Remove shadow effect from background
 * 5. DMA updated tilemaps to VRAM
 * 6. Check for level completion
 */
static void remove_brick(void) {
    u16 idx;

    blockcount--;

    /* Score: (brick_value + 1) * level_multiplier */
    for (i = 0; i <= level; i++)
        score += blocks[b] + 1;

    /* Bounce direction based on which axis changed */
    if (oby != by) { s16 neg_vy = -vel_y; vel_y = neg_vy; }
    if (obx != bx) { s16 neg_vx = -vel_x; vel_x = neg_vx; }

    /* Mark brick as destroyed */
    blocks[b] = 8;

    /* Calculate tilemap offset */
    idx = (by << 5) + (bx << 1);

    /* Clear brick tiles from BG1
     * Base 0x42 (not 0x62) because idx uses by which starts at 1,
     * while placement uses j which starts at 0: 0x42 + by*32 = 0x62 + j*32 */
    blockmap[0x42 + idx] = 0;
    blockmap[0x43 + idx] = 0;

    /* Remove shadow effect from BG3
     * Same row compensation: 0x63 + by*32 = 0x83 + j*32 */
    backmap[0x63 + idx] -= 0x400;
    backmap[0x64 + idx] -= 0x400;

    /* Update score display */
    writenum(score, 8, blockmap, 0xF5, 0x426);

    if (score > hiscore) {
        hiscore = score;
        writenum(score, 8, blockmap, 0x95, 0x426);
    }

    /* ATOMIC DMA: Both tilemaps in same VBlank (overlap at 0x0400-0x07FF) */
    WaitForVBlank();
    dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);
    dmaCopyVram((u8 *)backmap, 0x0400, 0x800);

    /* Level complete? */
    if (blockcount == 0) {
        new_level();
    }
}

/**
 * @brief Check collision with bricks
 *
 * GRID-BASED COLLISION:
 * Convert ball pixel position to brick grid coordinates, then check
 * if that grid cell contains a brick.
 *
 * Grid layout: 10 columns x 10 rows
 * Each brick is 16x8 pixels
 * Grid offset from screen edge: 14 pixels
 */
static void check_bricks(void) {
    /* Only check in brick zone (rows 1-10, y pixels 22-112) */
    if (pos_y >= 22 && pos_y < 112) {
        /* Save previous grid position for bounce direction */
        obx = bx;
        oby = by;

        /* Convert pixel position to grid coordinates */
        bx = (pos_x - 14) >> 4;  /* Divide by 16 (brick width) */
        by = (pos_y - 14) >> 3;  /* Divide by 8 (brick height) */

        /* Bounds check */
        if (bx < 10 && by >= 1 && by <= 10) {
            /* Calculate brick array index: bx + by*10 - 10
             * Using shifts: by*10 = (by<<3) + (by<<1) */
            b = bx + (by << 3) + (by << 1) - 10;

            if (b < 100 && blocks[b] != 8) {
                remove_brick();
            }
        }
    }
}

/**
 * @brief Run one frame of gameplay
 */
static void run_frame(void) {
    /* Read joypad (updated by NMI handler) */
    pad0 = pad_keys[0];

    handle_pause();
    move_paddle();
    move_ball();

    /* Check appropriate collision based on ball Y position */
    if (pos_y > 195) {
        check_paddle();
    } else {
        check_bricks();
    }

    draw_screen();
    WaitForVBlank();
    oamUpdate();
}

/*============================================================================
 * Main Entry Point
 *============================================================================*/

/**
 * @brief Game initialization and main loop
 *
 * INITIALIZATION SEQUENCE:
 * 1. Force blank (screen off) during setup
 * 2. Load tiles to VRAM
 * 3. Copy ROM data to RAM buffers
 * 4. Initialize game state
 * 5. Configure video mode and backgrounds
 * 6. Enable display
 * 7. Wait for START, then run game loop
 */
int main(void) {
    /* Force blank during setup - screen is black, VRAM accessible */
    setScreenOff();

    /* Initialize joypad buffer */
    pad_keys[0] = 0;
    pad_keys[1] = 0;

    /* Wait for NMI handler to read joypads */
    for (i = 0; i < 5; i++) {
        WaitForVBlank();
    }

    /* VRAM LAYOUT:
     * 0x0000-0x07FF: BG1 tilemap (32x32)
     * 0x0400-0x0BFF: BG3 tilemap (32x32) - overlaps BG1!
     * 0x1000-0x1EFF: BG tiles (tiles1)
     * 0x2000-0x224F: Sprite tiles (tiles2)
     *
     * Note: BG1 and BG3 tilemaps overlap at 0x0400-0x07FF.
     * This saves 1KB VRAM because BG1 only uses rows 0-15.
     */
    dmaCopyVram(tiles1, 0x1000, 0x0F00);  /* 3840 bytes of BG tiles */
    dmaCopyVram(tiles2, 0x2000, 0x0250);  /* 592 bytes of sprite tiles */

    /* Copy ROM data to RAM for runtime modification */
    mycopy((u8 *)blockmap, bg1map, 0x800);
    mycopy((u8 *)backmap, bg2map, 0x800);
    mycopy((u8 *)blocks, brick_map, 100);
    mycopy((u8 *)pal, palette, 0x200);

    /* Initialize game state */
    blockcount = 0;
    bx = 5;
    by = 11;
    score = 0;
    hiscore = 50000;
    level2 = 1;
    color = 0;
    level = 0;
    lives = 4;
    px = 80;
    vel_x = 2;
    vel_y = 1;
    pos_x = 94;
    pos_y = 109;

    /* Build initial brick wall in tilemap buffer */
    b = 0;
    for (j = 0; j < 10; j++) {
        for (i = 0; i < 20; i += 2) {
            a = blocks[b];
            b++;  /* Separate increment avoids compiler issues */
            if (a < 8) {
                c = (j << 5) + i;
                blockcount++;
                blockmap[0x62 + c] = 13 + (a << 10);
                blockmap[0x63 + c] = 14 + (a << 10);
                backmap[0x83 + c] += 0x400;
                backmap[0x84 + c] += 0x400;
            }
        }
    }

    /* Update HUD in tilemap */
    writenum(lives, 8, blockmap, 0x136, 0x426);
    writestring(ST_READY, blockmap, 0x248, 0x3F6);

    /* BACKGROUND CONFIGURATION:
     * BG1: Tilemap at 0x0000, tiles at 0x1000
     * BG3: Tilemap at 0x0400, tiles at 0x2000 (shared with sprites) */
    bgSetMapPtr(0, 0x0000, SC_32x32);
    bgSetMapPtr(2, 0x0400, SC_32x32);
    bgSetGfxPtr(0, 0x1000);
    bgSetGfxPtr(2, 0x2000);

    /* Upload tilemaps and palette to VRAM/CGRAM
     * CRITICAL: Both tilemaps must be uploaded in the SAME VBlank
     * because they overlap at 0x0400-0x07FF! */
    WaitForVBlank();
    dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);
    dmaCopyVram((u8 *)backmap, 0x0400, 0x800);
    dmaCopyCGram((u8 *)pal, 0, 256 * 2);

    /* SPRITE CONFIGURATION:
     * 8x8/16x16 sprites, name base=0, name select=0
     * Secondary name table at (0+0+1)*8KB = 0x2000
     * Access secondary table with: tile_number | 256 */
    REG_OBJSEL = OBJSEL(OBJ_SIZE8_L16, 0x0000);
    oamClear();

    /* MODE 1: BG1 4bpp, BG2 4bpp (unused), BG3 2bpp */
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1 | TM_BG3 | TM_OBJ;

    /* Initialize scroll positions */
    bgSetScroll(0, 0, 0);
    bgSetScroll(2, 0, 0);

    /* Setup sprites */
    draw_screen();
    /* High table: sprite 0 hidden (X high=1), sprites 1-10 visible (X high=0, size=small) */
    oamMemory[512] = 0x01;  /* Sprite 0: X high=1 (hidden), sprites 1-3: X high=0 */
    oamMemory[513] = 0x00;  /* Sprites 4-7: visible */
    oamMemory[514] = 0x40;  /* Sprites 8-10: visible, sprite 11: hidden (X high=1) */

    /* Transfer OAM to hardware */
    WaitForVBlank();
    oamUpdate();

    /* Enable display at full brightness */
    setScreenOn();

    /* Wait for START to begin game */
    do { WaitForVBlank(); } while ((pad_keys[0] & KEY_START) == 0);
    do { WaitForVBlank(); } while (pad_keys[0] & KEY_START);

    /* Clear ready message */
    writestring(ST_BLANK, blockmap, 0x248, 0x3F6);
    writestring(ST_BLANK, blockmap, 0x289, 0x3F6);
    WaitForVBlank();
    dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);

    /* Main game loop */
    while (1) {
        run_frame();
    }

    return 0;
}
