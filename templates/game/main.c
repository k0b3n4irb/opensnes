/**
 * @file main.c
 * @brief OpenSNES Game Template
 *
 * A complete game template with proper structure:
 * - Game state management (title, playing, paused)
 * - Sprite-based player with movement
 * - Background with scrolling
 * - Input handling with edge detection
 *
 * This template is designed to be extended for your game.
 *
 * @author Your Name
 * @license MIT (or your preferred license)
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef signed char s8;
typedef signed short s16;

/*============================================================================
 * Hardware Registers
 *============================================================================*/

#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_OBJSEL   (*(volatile u8*)0x2101)
#define REG_OAMADDL  (*(volatile u8*)0x2102)
#define REG_OAMADDH  (*(volatile u8*)0x2103)
#define REG_OAMDATA  (*(volatile u8*)0x2104)
#define REG_BGMODE   (*(volatile u8*)0x2105)
#define REG_BG1SC    (*(volatile u8*)0x2107)
#define REG_BG12NBA  (*(volatile u8*)0x210B)
#define REG_BG1HOFS  (*(volatile u8*)0x210D)
#define REG_BG1VOFS  (*(volatile u8*)0x210E)
#define REG_VMAIN    (*(volatile u8*)0x2115)
#define REG_VMADDL   (*(volatile u8*)0x2116)
#define REG_VMADDH   (*(volatile u8*)0x2117)
#define REG_VMDATAL  (*(volatile u8*)0x2118)
#define REG_VMDATAH  (*(volatile u8*)0x2119)
#define REG_CGADD    (*(volatile u8*)0x2121)
#define REG_CGDATA   (*(volatile u8*)0x2122)
#define REG_TM       (*(volatile u8*)0x212C)
#define REG_NMITIMEN (*(volatile u8*)0x4200)
#define REG_RDNMI    (*(volatile u8*)0x4210)
#define REG_HVBJOY   (*(volatile u8*)0x4212)
#define REG_JOY1L    (*(volatile u8*)0x4218)
#define REG_JOY1H    (*(volatile u8*)0x4219)

/* Joypad masks */
#define JOY_A       0x0080
#define JOY_B       0x8000
#define JOY_X       0x0040
#define JOY_Y       0x4000
#define JOY_L       0x0020
#define JOY_R       0x0010
#define JOY_UP      0x0800
#define JOY_DOWN    0x0400
#define JOY_LEFT    0x0200
#define JOY_RIGHT   0x0100
#define JOY_START   0x1000
#define JOY_SELECT  0x2000

/*============================================================================
 * Game Constants
 *============================================================================*/

#define SCREEN_W    256
#define SCREEN_H    224

#define PLAYER_W    16
#define PLAYER_H    16
#define PLAYER_SPEED 2

/* Game states */
#define STATE_TITLE   0
#define STATE_PLAYING 1
#define STATE_PAUSED  2
#define STATE_GAMEOVER 3

/*============================================================================
 * Graphics Data
 *============================================================================*/

/* Player sprite: 16x16, 4bpp = 128 bytes (4 tiles) */
static const u8 player_sprite[] = {
    /* Tile 0 (top-left) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00,
    0x7E, 0x00, 0x7E, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Tile 1 (top-right) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00,
    0x7E, 0x00, 0x7E, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Tile 2 (bottom-left) */
    0xFF, 0x00, 0xFF, 0x00, 0x7E, 0x00, 0x7E, 0x00,
    0x3C, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Tile 3 (bottom-right) */
    0xFF, 0x00, 0xFF, 0x00, 0x7E, 0x00, 0x7E, 0x00,
    0x3C, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* Background tile (simple grid pattern) */
static const u8 bg_tiles[] = {
    /* Tile 0: Empty */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Tile 1: Grid */
    0xFF, 0x00, 0x81, 0x00, 0x81, 0x00, 0x81, 0x00,
    0x81, 0x00, 0x81, 0x00, 0x81, 0x00, 0xFF, 0x00,
};

/*============================================================================
 * Game State
 *============================================================================*/

static u8 game_state;
static u16 game_frame;

/* Player state */
static u16 player_x;
static u16 player_y;

/* Input state (for edge detection) */
static u16 joy_current;
static u16 joy_previous;

/* Scroll position */
static s16 scroll_x;
static s16 scroll_y;

/*============================================================================
 * OAM Management
 *============================================================================*/

static u8 oam_buffer[128 * 4];  /* Low table */
static u8 oam_hi[32];            /* High table */
static u8 oam_index;

static void oam_clear(void) {
    u8 i;
    oam_index = 0;
    /* Hide all sprites (Y = 240) */
    for (i = 0; i < 128; i++) {
        oam_buffer[i * 4 + 0] = 0;
        oam_buffer[i * 4 + 1] = 240;
        oam_buffer[i * 4 + 2] = 0;
        oam_buffer[i * 4 + 3] = 0;
    }
}

static void oam_add_sprite(u16 x, u8 y, u8 tile, u8 attr, u8 large) {
    u8 base;
    u8 hi_byte;
    u8 hi_shift;

    if (oam_index >= 128) return;

    base = oam_index * 4;
    oam_buffer[base + 0] = x & 0xFF;
    oam_buffer[base + 1] = y;
    oam_buffer[base + 2] = tile;
    oam_buffer[base + 3] = attr;

    /* High table: bit 0 = X bit 8, bit 1 = size */
    hi_byte = oam_index / 4;
    hi_shift = (oam_index % 4) * 2;
    oam_hi[hi_byte] &= ~(3 << hi_shift);
    if (x > 255) oam_hi[hi_byte] |= (1 << hi_shift);
    if (large) oam_hi[hi_byte] |= (2 << hi_shift);

    oam_index++;
}

static void oam_update(void) {
    u16 i;

    REG_OAMADDL = 0;
    REG_OAMADDH = 0;

    for (i = 0; i < 512; i++) {
        REG_OAMDATA = oam_buffer[i];
    }
    for (i = 0; i < 32; i++) {
        REG_OAMDATA = oam_hi[i];
    }
}

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void wait_vblank(void) {
    while (!(REG_RDNMI & 0x80)) {}
}

static void update_input(void) {
    while (REG_HVBJOY & 0x01) {}
    joy_previous = joy_current;
    joy_current = REG_JOY1L | (REG_JOY1H << 8);
}

/* Returns buttons just pressed this frame */
static u16 joy_pressed(void) {
    return joy_current & ~joy_previous;
}

/* Returns buttons currently held */
static u16 joy_held(void) {
    return joy_current;
}

static void set_scroll(s16 x, s16 y) {
    REG_BG1HOFS = x & 0xFF;
    REG_BG1HOFS = (x >> 8) & 0xFF;
    REG_BG1VOFS = y & 0xFF;
    REG_BG1VOFS = (y >> 8) & 0xFF;
}

/*============================================================================
 * Game Logic
 *============================================================================*/

static void init_player(void) {
    player_x = SCREEN_W / 2 - PLAYER_W / 2;
    player_y = SCREEN_H / 2 - PLAYER_H / 2;
}

static void update_player(void) {
    u16 held;
    held = joy_held();

    if (held & JOY_UP) {
        if (player_y > 0) {
            player_y = player_y - PLAYER_SPEED;
        }
    }
    if (held & JOY_DOWN) {
        if (player_y < SCREEN_H - PLAYER_H) {
            player_y = player_y + PLAYER_SPEED;
        }
    }
    if (held & JOY_LEFT) {
        if (player_x > 0) {
            player_x = player_x - PLAYER_SPEED;
        }
    }
    if (held & JOY_RIGHT) {
        if (player_x < SCREEN_W - PLAYER_W) {
            player_x = player_x + PLAYER_SPEED;
        }
    }
}

static void draw_player(void) {
    /* Using 8x8 sprites for 16x16 player (OBJSEL=0 means 8x8 small) */
    /* For a real game, configure 16x16 sprites with OBJSEL */
    oam_add_sprite(player_x, player_y, 0, 0x30, 0);
    oam_add_sprite(player_x + 8, player_y, 1, 0x30, 0);
    oam_add_sprite(player_x, player_y + 8, 2, 0x30, 0);
    oam_add_sprite(player_x + 8, player_y + 8, 3, 0x30, 0);
}

/*============================================================================
 * State Handlers
 *============================================================================*/

static void state_title(void) {
    /* Wait for Start to begin */
    if (joy_pressed() & JOY_START) {
        game_state = STATE_PLAYING;
        init_player();
    }
}

static void state_playing(void) {
    /* Handle pause */
    if (joy_pressed() & JOY_START) {
        game_state = STATE_PAUSED;
        return;
    }

    /* Update game */
    update_player();

    /* Draw game */
    oam_clear();
    draw_player();
}

static void state_paused(void) {
    /* Resume on Start */
    if (joy_pressed() & JOY_START) {
        game_state = STATE_PLAYING;
    }
}

/*============================================================================
 * Initialization
 *============================================================================*/

static void init_graphics(void) {
    u16 i;

    /* Mode 1 */
    REG_BGMODE = 0x01;
    REG_BG1SC = 0x04;
    REG_BG12NBA = 0x00;

    /* 8x8/16x16 sprites */
    REG_OBJSEL = 0x00;

    /* Load BG tiles */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    for (i = 0; i < sizeof(bg_tiles); i += 2) {
        REG_VMDATAL = bg_tiles[i];
        REG_VMDATAH = bg_tiles[i + 1];
    }

    /* Load sprite tiles at $4000 */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x40;

    for (i = 0; i < sizeof(player_sprite); i += 2) {
        REG_VMDATAL = player_sprite[i];
        REG_VMDATAH = player_sprite[i + 1];
    }

    /* BG palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x40;  /* Dark blue */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* White */

    /* Sprite palette */
    REG_CGADD = 128;
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Transparent */
    REG_CGDATA = 0x1F; REG_CGDATA = 0x00;  /* Red */
    REG_CGDATA = 0x00; REG_CGDATA = 0x7C;  /* Blue */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* White */

    /* Fill tilemap with grid pattern */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;

    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 1;  /* Grid tile */
        REG_VMDATAH = 0;
    }

    /* Initialize OAM */
    oam_clear();
    oam_update();
}

static void init_game(void) {
    game_state = STATE_TITLE;
    game_frame = 0;
    scroll_x = 0;
    scroll_y = 0;
    joy_current = 0;
    joy_previous = 0;

    init_player();
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    init_graphics();
    init_game();

    /* Enable NMI and auto-joypad */
    REG_NMITIMEN = 0x81;

    /* Enable BG1 and sprites */
    REG_TM = 0x11;

    /* Turn on screen */
    REG_INIDISP = 0x0F;

    /* Main loop */
    while (1) {
        wait_vblank();
        update_input();

        /* Run state handler */
        switch (game_state) {
            case STATE_TITLE:
                state_title();
                break;
            case STATE_PLAYING:
                state_playing();
                break;
            case STATE_PAUSED:
                state_paused();
                break;
        }

        /* Update hardware */
        oam_update();
        set_scroll(scroll_x, scroll_y);

        game_frame++;
    }

    return 0;
}
