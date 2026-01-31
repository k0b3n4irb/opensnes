/**
 * @file main.c
 * @brief Smooth Movement Example
 *
 * Demonstrates fixed-point math for smooth sprite movement:
 * - Sub-pixel positioning (8.8 fixed-point)
 * - Sine/cosine for circular motion
 * - Smooth acceleration/deceleration
 * - Multiple movement patterns
 *
 * Controls:
 * - D-pad: Move player with smooth acceleration
 * - A button: Cycle through orbit speeds
 */

#include <snes.h>
#include <snes/math.h>

/*============================================================================
 * Game State (using fixed-point positions)
 *============================================================================*/

/* Player state */
static fixed player_x;      /* Fixed-point position */
static fixed player_y;
static fixed player_vx;     /* Velocity */
static fixed player_vy;

/* Orbiting sprite */
static u8 orbit_angle;
static fixed orbit_radius;
static u8 orbit_speed;

/* Constants */
#define ACCEL FIX_MAKE(0, 32)    /* 0.125 acceleration */
#define MAX_SPEED FIX(3)         /* 3 pixels/frame max */
#define FRICTION FIX_MAKE(0, 8)  /* 0.03 friction */
#define ORBIT_RADIUS FIX(40)     /* 40 pixel orbit */

/*============================================================================
 * Sprite Graphics (8x8, 4bpp)
 *============================================================================*/

/* Player sprite - filled square */
static const u8 player_tile[] = {
    0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF,
    0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

/* Orbit sprite - circle */
static const u8 orbit_tile[] = {
    0x3C,0x3C, 0x7E,0x42, 0xFF,0x81, 0xFF,0x81,
    0xFF,0x81, 0xFF,0x81, 0x7E,0x42, 0x3C,0x3C,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

/* Trail sprite - small dot */
static const u8 trail_tile[] = {
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x18,0x18,
    0x18,0x18, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

/* Sprite palette */
static const u8 sprite_palette[] = {
    0x00, 0x00,  /* Transparent */
    0xFF, 0x03,  /* Cyan (player) */
    0x1F, 0x7C,  /* Magenta (orbit) */
    0x00, 0x7C,  /* Red (trail) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/*============================================================================
 * Trail for orbit (shows path)
 *============================================================================*/

#define TRAIL_LENGTH 8
static s16 trail_x[TRAIL_LENGTH];
static s16 trail_y[TRAIL_LENGTH];
static u8 trail_index;

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void load_graphics(void) {
    u16 i;

    REG_INIDISP = 0x80;

    /* Load sprite tiles at $4000 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x40;

    /* Tile 0: player */
    for (i = 0; i < 32; i += 2) {
        REG_VMDATAL = player_tile[i];
        REG_VMDATAH = player_tile[i + 1];
    }
    /* Tile 1: orbit */
    for (i = 0; i < 32; i += 2) {
        REG_VMDATAL = orbit_tile[i];
        REG_VMDATAH = orbit_tile[i + 1];
    }
    /* Tile 2: trail */
    for (i = 0; i < 32; i += 2) {
        REG_VMDATAL = trail_tile[i];
        REG_VMDATAH = trail_tile[i + 1];
    }

    /* Load sprite palette */
    REG_CGADD = 128;
    for (i = 0; i < 32; i++) {
        REG_CGDATA = sprite_palette[i];
    }

    /* Set BG color */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x10;  /* Dark blue */
}

static void update_player(u16 pad) {
    /* Apply acceleration based on input */
    if (pad & KEY_LEFT) {
        player_vx = player_vx - ACCEL;
    }
    if (pad & KEY_RIGHT) {
        player_vx = player_vx + ACCEL;
    }
    if (pad & KEY_UP) {
        player_vy = player_vy - ACCEL;
    }
    if (pad & KEY_DOWN) {
        player_vy = player_vy + ACCEL;
    }

    /* Apply friction when not pressing direction */
    if (!(pad & (KEY_LEFT | KEY_RIGHT))) {
        if (player_vx > 0) {
            player_vx = player_vx - FRICTION;
            if (player_vx < 0) player_vx = 0;
        } else if (player_vx < 0) {
            player_vx = player_vx + FRICTION;
            if (player_vx > 0) player_vx = 0;
        }
    }
    if (!(pad & (KEY_UP | KEY_DOWN))) {
        if (player_vy > 0) {
            player_vy = player_vy - FRICTION;
            if (player_vy < 0) player_vy = 0;
        } else if (player_vy < 0) {
            player_vy = player_vy + FRICTION;
            if (player_vy > 0) player_vy = 0;
        }
    }

    /* Clamp velocity */
    player_vx = fixClamp(player_vx, 0 - MAX_SPEED, MAX_SPEED);
    player_vy = fixClamp(player_vy, 0 - MAX_SPEED, MAX_SPEED);

    /* Update position */
    player_x = player_x + player_vx;
    player_y = player_y + player_vy;

    /* Screen bounds (with fixed-point) */
    if (player_x < FIX(8)) player_x = FIX(8);
    if (player_x > FIX(240)) player_x = FIX(240);
    if (player_y < FIX(8)) player_y = FIX(8);
    if (player_y > FIX(208)) player_y = FIX(208);
}

static void update_orbit(void) {
    fixed sin_val, cos_val;
    fixed orbit_x, orbit_y;
    s16 screen_x, screen_y;

    /* Update angle */
    orbit_angle = orbit_angle + orbit_speed;

    /* Calculate orbit position using sine/cosine */
    sin_val = fixSin(orbit_angle);
    cos_val = fixCos(orbit_angle);

    /* Orbit around player position */
    orbit_x = player_x + fixMul(orbit_radius, cos_val);
    orbit_y = player_y + fixMul(orbit_radius, sin_val);

    /* Convert to screen coordinates */
    screen_x = UNFIX(orbit_x);
    screen_y = UNFIX(orbit_y);

    /* Store in trail */
    trail_x[trail_index] = screen_x;
    trail_y[trail_index] = screen_y;
    trail_index = (trail_index + 1) & (TRAIL_LENGTH - 1);
}

static void update_sprites(void) {
    s16 px, py;
    u8 i, idx;

    /* Player sprite */
    px = UNFIX(player_x);
    py = UNFIX(player_y);
    oamSet(0, px, py, 0, 0, 3, 0);

    /* Orbit sprite (at most recent trail position) */
    idx = (trail_index + TRAIL_LENGTH - 1) & (TRAIL_LENGTH - 1);
    oamSet(1, trail_x[idx], trail_y[idx], 1, 0, 2, 0);

    /* Trail sprites */
    for (i = 0; i < TRAIL_LENGTH - 1; i++) {
        idx = (trail_index + i) & (TRAIL_LENGTH - 1);
        oamSet(2 + i, trail_x[idx], trail_y[idx], 2, 0, 1, 0);
    }

    /* Hide remaining sprites */
    oamHide(2 + TRAIL_LENGTH - 1);
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u16 pad, pad_prev, pad_pressed;
    u8 i;

    /* Initialize */
    consoleInit();
    setMode(BG_MODE0, 0);
    oamInit();

    /* Load graphics */
    load_graphics();

    /* Configure display */
    REG_TM = TM_OBJ;  /* Sprites only */
    REG_OBJSEL = 0x02;  /* 8x8/16x16, tiles at $4000 */

    /* Initialize player */
    player_x = FIX(128);  /* Center of screen */
    player_y = FIX(112);
    player_vx = 0;
    player_vy = 0;

    /* Initialize orbit */
    orbit_angle = 0;
    orbit_radius = ORBIT_RADIUS;
    orbit_speed = 4;  /* Medium speed */

    /* Initialize trail */
    trail_index = 0;
    for (i = 0; i < TRAIL_LENGTH; i++) {
        trail_x[i] = UNFIX(player_x);
        trail_y[i] = UNFIX(player_y);
    }

    /* Initial update */
    update_orbit();
    update_sprites();
    oamUpdate();

    /* Enable screen */
    setScreenOn();

    /* Input state */
    WaitForVBlank();
    while (REG_HVBJOY & 0x01) {}
    pad_prev = REG_JOY1L | (REG_JOY1H << 8);

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Read input */
        while (REG_HVBJOY & 0x01) {}
        pad = REG_JOY1L | (REG_JOY1H << 8);
        pad_pressed = pad & ~pad_prev;
        pad_prev = pad;

        if (pad == 0xFFFF) continue;

        /* A button: cycle orbit speed */
        if (pad_pressed & KEY_A) {
            orbit_speed = orbit_speed + 2;
            if (orbit_speed > 12) orbit_speed = 2;
        }

        /* Update game */
        update_player(pad);
        update_orbit();

        /* Update display */
        update_sprites();
        oamUpdate();
    }

    return 0;
}
