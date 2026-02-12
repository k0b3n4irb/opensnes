/**
 * @file main.c
 * @brief Calculator Example using OpenSNES C Library
 *
 * A simple 4-function calculator demonstrating:
 * - Library initialization (consoleInit, setMode)
 * - Text rendering with embedded font
 * - Input handling (padUpdate, padPressed)
 * - 16-bit arithmetic in C
 *
 * Controls:
 * - D-pad: Move cursor between buttons
 * - A button: Press selected button
 * - Buttons: 0-9, +, -, *, /, C (clear), = (equals)
 */

#include <snes.h>

/*============================================================================
 * Embedded Font (2bpp, 16 bytes per tile)
 * Characters: space, 0-9, +, -, *, /, =, C, [, ]
 *============================================================================*/

static const u8 font_tiles[] = {
    /* 0: Space */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    /* 1: 0 */
    0x3C,0x00, 0x66,0x00, 0x6E,0x00, 0x76,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 2: 1 */
    0x18,0x00, 0x38,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x7E,0x00, 0x00,0x00,
    /* 3: 2 */
    0x3C,0x00, 0x66,0x00, 0x06,0x00, 0x1C,0x00,
    0x30,0x00, 0x60,0x00, 0x7E,0x00, 0x00,0x00,
    /* 4: 3 */
    0x3C,0x00, 0x66,0x00, 0x06,0x00, 0x1C,0x00,
    0x06,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 5: 4 */
    0x0C,0x00, 0x1C,0x00, 0x3C,0x00, 0x6C,0x00,
    0x7E,0x00, 0x0C,0x00, 0x0C,0x00, 0x00,0x00,
    /* 6: 5 */
    0x7E,0x00, 0x60,0x00, 0x7C,0x00, 0x06,0x00,
    0x06,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 7: 6 */
    0x1C,0x00, 0x30,0x00, 0x60,0x00, 0x7C,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 8: 7 */
    0x7E,0x00, 0x06,0x00, 0x0C,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x18,0x00, 0x00,0x00,
    /* 9: 8 */
    0x3C,0x00, 0x66,0x00, 0x66,0x00, 0x3C,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 10: 9 */
    0x3C,0x00, 0x66,0x00, 0x66,0x00, 0x3E,0x00,
    0x06,0x00, 0x0C,0x00, 0x38,0x00, 0x00,0x00,
    /* 11: + */
    0x00,0x00, 0x18,0x00, 0x18,0x00, 0x7E,0x00,
    0x18,0x00, 0x18,0x00, 0x00,0x00, 0x00,0x00,
    /* 12: - */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x7E,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    /* 13: * */
    0x00,0x00, 0x66,0x00, 0x3C,0x00, 0xFF,0x00,
    0x3C,0x00, 0x66,0x00, 0x00,0x00, 0x00,0x00,
    /* 14: / */
    0x06,0x00, 0x0C,0x00, 0x18,0x00, 0x30,0x00,
    0x60,0x00, 0xC0,0x00, 0x80,0x00, 0x00,0x00,
    /* 15: = */
    0x00,0x00, 0x00,0x00, 0x7E,0x00, 0x00,0x00,
    0x7E,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    /* 16: C */
    0x3C,0x00, 0x66,0x00, 0x60,0x00, 0x60,0x00,
    0x60,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 17: [ */
    0x1E,0x00, 0x18,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x1E,0x00, 0x00,0x00,
    /* 18: ] */
    0x78,0x00, 0x18,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x78,0x00, 0x00,0x00,
    /* 19: A (for CALCULATOR title) */
    0x18,0x00, 0x3C,0x00, 0x66,0x00, 0x7E,0x00,
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* 20: L */
    0x60,0x00, 0x60,0x00, 0x60,0x00, 0x60,0x00,
    0x60,0x00, 0x60,0x00, 0x7E,0x00, 0x00,0x00,
    /* 21: U */
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 22: T */
    0x7E,0x00, 0x18,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x18,0x00, 0x00,0x00,
    /* 23: O */
    0x3C,0x00, 0x66,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 24: R */
    0x7C,0x00, 0x66,0x00, 0x66,0x00, 0x7C,0x00,
    0x6C,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
};

#define FONT_SIZE (25 * 16)

/* Tile indices */
#define TILE_SPACE 0
#define TILE_0     1
#define TILE_1     2
#define TILE_2     3
#define TILE_3     4
#define TILE_4     5
#define TILE_5     6
#define TILE_6     7
#define TILE_7     8
#define TILE_8     9
#define TILE_9     10
#define TILE_PLUS  11
#define TILE_MINUS 12
#define TILE_MUL   13
#define TILE_DIV   14
#define TILE_EQ    15
#define TILE_C     16
#define TILE_LBRACK 17
#define TILE_RBRACK 18
#define TILE_A     19
#define TILE_L     20
#define TILE_U     21
#define TILE_T     22
#define TILE_O     23
#define TILE_R     24

/*============================================================================
 * Calculator State
 *============================================================================*/

extern volatile u8 vblank_flag;  /* Defined in crt0.asm, set by NMI handler */

static u8 cursor_x;        /* Cursor X (0-3) */
static u8 cursor_y;        /* Cursor Y (0-3) */
static u16 display_value;  /* Current display value */
static u16 accumulator;    /* Stored value for operations */
static u8 pending_op;      /* Pending operation: 0=none, 1=+, 2=-, 3=*, 4=/ */
static u8 new_number;      /* Flag: next digit starts new number */

/*============================================================================
 * VRAM Configuration
 *============================================================================*/

#define TILEMAP_ADDR  0x0400   /* Word address for tilemap */
#define TILES_ADDR    0x0000   /* Word address for tiles */

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void write_tile(u8 x, u8 y, u8 tile) {
    u16 addr;
    addr = TILEMAP_ADDR + y * 32 + x;
    REG_VMAIN = 0x80;
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;
    REG_VMDATAL = tile;
    REG_VMDATAH = 0;
}

static void clear_tilemap(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = TILEMAP_ADDR & 0xFF;
    REG_VMADDH = TILEMAP_ADDR >> 8;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = TILE_SPACE;
        REG_VMDATAH = 0;
    }
}

static void load_font(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = TILES_ADDR & 0xFF;
    REG_VMADDH = TILES_ADDR >> 8;
    for (i = 0; i < FONT_SIZE; i += 2) {
        REG_VMDATAL = font_tiles[i];
        REG_VMDATAH = font_tiles[i + 1];
    }
}

/*============================================================================
 * Display Functions
 *============================================================================*/

/* Button layout:
 *   [7] [8] [9] [/]
 *   [4] [5] [6] [*]
 *   [1] [2] [3] [-]
 *   [0] [C] [=] [+]
 */

/* Tile for each button position */
static const u8 button_tiles[16] = {
    TILE_7, TILE_8, TILE_9, TILE_DIV,
    TILE_4, TILE_5, TILE_6, TILE_MUL,
    TILE_1, TILE_2, TILE_3, TILE_MINUS,
    TILE_0, TILE_C, TILE_EQ, TILE_PLUS
};

/* Values: 0-9 for digits, 0xF0-F3 for ops, 0xFE=clear, 0xFF=equals */
static const u8 button_values[16] = {
    7, 8, 9, 0xF3,      /* 7, 8, 9, / */
    4, 5, 6, 0xF2,      /* 4, 5, 6, * */
    1, 2, 3, 0xF1,      /* 1, 2, 3, - */
    0, 0xFE, 0xFF, 0xF0 /* 0, C, =, + */
};

#define BTN_START_X 10
#define BTN_START_Y 10
#define BTN_SPACE   4
#define DISPLAY_X   14
#define DISPLAY_Y   6

static void draw_buttons(void) {
    u8 i, bx, by;
    for (i = 0; i < 16; i++) {
        bx = BTN_START_X + (i & 3) * BTN_SPACE;
        by = BTN_START_Y + (i >> 2) * 2;
        write_tile(bx, by, button_tiles[i]);
    }
}

static void draw_cursor(u8 show) {
    u8 bx, by;
    bx = BTN_START_X + cursor_x * BTN_SPACE;
    by = BTN_START_Y + cursor_y * 2;
    if (show) {
        write_tile(bx - 1, by, TILE_LBRACK);
        write_tile(bx + 1, by, TILE_RBRACK);
    } else {
        write_tile(bx - 1, by, TILE_SPACE);
        write_tile(bx + 1, by, TILE_SPACE);
    }
}

static void draw_title(void) {
    /* "CALCULATOR" at row 4 */
    write_tile(11, 4, TILE_C);
    write_tile(12, 4, TILE_A);
    write_tile(13, 4, TILE_L);
    write_tile(14, 4, TILE_C);
    write_tile(15, 4, TILE_U);
    write_tile(16, 4, TILE_L);
    write_tile(17, 4, TILE_A);
    write_tile(18, 4, TILE_T);
    write_tile(19, 4, TILE_O);
    write_tile(20, 4, TILE_R);
}

static void update_display(void) {
    u16 val;
    u8 x;
    u8 d0, d1, d2, d3, d4;

    /* Clear stale VBlank flag — if computation (multiply loops)
       took longer than one frame, NMI already set vblank_flag=1.
       Without clearing, WaitForVBlank returns immediately during
       active display, and VRAM writes silently fail. */
    vblank_flag = 0;
    WaitForVBlank();

    /* Clear display area (5 digits) */
    for (x = 0; x < 5; x++) {
        write_tile(DISPLAY_X + x, DISPLAY_Y, TILE_SPACE);
    }

    val = display_value;

    /* Extract digits using repeated subtraction */
    d4 = 0;
    while (val >= 10000) { val = val - 10000; d4++; }
    d3 = 0;
    while (val >= 1000) { val = val - 1000; d3++; }
    d2 = 0;
    while (val >= 100) { val = val - 100; d2++; }
    d1 = 0;
    while (val >= 10) { val = val - 10; d1++; }
    d0 = (u8)val;  /* Remainder is ones digit */

    /* Display right-aligned, skip leading zeros */
    x = DISPLAY_X;
    if (d4 > 0) { write_tile(x, DISPLAY_Y, TILE_0 + d4); }
    x++;
    if (d4 > 0 || d3 > 0) { write_tile(x, DISPLAY_Y, TILE_0 + d3); }
    x++;
    if (d4 > 0 || d3 > 0 || d2 > 0) { write_tile(x, DISPLAY_Y, TILE_0 + d2); }
    x++;
    if (d4 > 0 || d3 > 0 || d2 > 0 || d1 > 0) { write_tile(x, DISPLAY_Y, TILE_0 + d1); }
    x++;
    write_tile(x, DISPLAY_Y, TILE_0 + d0);  /* Always show ones digit */
}

/*============================================================================
 * Calculator Logic
 *============================================================================*/

static void do_operation(void) {
    u16 a, b, result;

    a = accumulator;
    b = display_value;
    result = 0;

    if (pending_op == 1) {
        /* Addition */
        display_value = a + b;
    } else if (pending_op == 2) {
        /* Subtraction */
        display_value = a - b;
    } else if (pending_op == 3) {
        /* Multiplication - inline repeated addition */
        while (b > 0) {
            result = result + a;
            b = b - 1;
        }
        display_value = result;
    } else if (pending_op == 4) {
        /* Division - inline repeated subtraction */
        if (b != 0) {
            while (a >= b) {
                a = a - b;
                result = result + 1;
            }
            display_value = result;
        }
    }
    update_display();
}

static void handle_digit(u8 digit) {
    u16 val;
    if (new_number) {
        display_value = 0;
        new_number = 0;
    }
    if (display_value <= 6553) {
        /* Multiply by 10 using shifts: x*10 = x*8 + x*2 = (x<<3) + (x<<1) */
        val = display_value;
        display_value = (val << 3) + (val << 1) + digit;
    }
    update_display();
}

static void handle_operator(u8 op) {
    if (pending_op != 0) {
        do_operation();
    }
    accumulator = display_value;
    pending_op = op;
    new_number = 1;
}

static void handle_equals(void) {
    if (pending_op != 0) {
        do_operation();
        pending_op = 0;
    }
    new_number = 1;
}

static void handle_clear(void) {
    display_value = 0;
    accumulator = 0;
    pending_op = 0;
    new_number = 1;
    update_display();
}

static void press_button(void) {
    u8 pos, value;
    pos = cursor_y * 4 + cursor_x;
    value = button_values[pos];

    if (value <= 9) {
        handle_digit(value);
    } else if (value == 0xFE) {
        handle_clear();
    } else if (value == 0xFF) {
        handle_equals();
    } else {
        handle_operator((value & 0x0F) + 1);
    }
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u16 pad;
    u16 pad_prev;
    u16 pad_pressed;
    u8 old_x, old_y;

    /* Initialize hardware */
    consoleInit();
    setMode(BG_MODE0, 0);

    /* Load font tiles */
    load_font();

    /* Clear tilemap */
    clear_tilemap();

    /* Configure BG1 */
    REG_BG1SC = 0x04;    /* Tilemap at $0800, 32x32 */
    REG_BG12NBA = 0x00;  /* BG1 tiles at $0000 */
    REG_TM = TM_BG1;     /* Enable BG1 */

    /* Set palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x28;  /* Dark blue */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* White */

    /* Draw interface */
    draw_title();
    draw_buttons();

    /* Initialize state */
    cursor_x = 0;
    cursor_y = 0;
    display_value = 0;
    accumulator = 0;
    pending_op = 0;
    new_number = 1;

    update_display();
    draw_cursor(1);

    /* Enable screen */
    setScreenOn();

    /* Initialize input */
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
        if (pad_pressed == 0) continue;

        /* Save old position */
        old_x = cursor_x;
        old_y = cursor_y;

        /* Process D-pad */
        if (pad_pressed & KEY_LEFT) {
            if (cursor_x > 0) cursor_x--;
        }
        if (pad_pressed & KEY_RIGHT) {
            if (cursor_x < 3) cursor_x++;
        }
        if (pad_pressed & KEY_UP) {
            if (cursor_y > 0) cursor_y--;
        }
        if (pad_pressed & KEY_DOWN) {
            if (cursor_y < 3) cursor_y++;
        }

        /* Process A button */
        if (pad_pressed & KEY_A) {
            press_button();
        }

        /* Update cursor if moved */
        if (cursor_x != old_x || cursor_y != old_y) {
            /* Clear old cursor */
            cursor_x = old_x;
            cursor_y = old_y;
            draw_cursor(0);
            /* Restore and draw new */
            if (pad_pressed & KEY_LEFT) { if (old_x > 0) cursor_x = old_x - 1; }
            if (pad_pressed & KEY_RIGHT) { if (old_x < 3) cursor_x = old_x + 1; }
            if (pad_pressed & KEY_UP) { if (old_y > 0) cursor_y = old_y - 1; }
            if (pad_pressed & KEY_DOWN) { if (old_y < 3) cursor_y = old_y + 1; }
            draw_cursor(1);
        }
    }

    return 0;
}
