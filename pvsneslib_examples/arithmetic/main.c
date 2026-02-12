/*
 * arithmetic - Ported from PVSnesLib to OpenSNES
 *
 * Integer calculator with on-screen button grid.
 * Extends examples/basics/2_calculator with power (^) operation.
 * D-pad moves cursor, A presses the selected button.
 * Supports +, -, *, /, ^ (power), C (clear), = (equals).
 *
 * Uses direct VRAM writes for reliable tile rendering.
 *
 * -- alekmaul (original PVSnesLib example)
 */

#include <snes.h>

/*============================================================================
 * Embedded Font (2bpp, 16 bytes per tile)
 * Characters: space, 0-9, +, -, *, /, =, C, [, ], ^
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
    /* 19: ^ (caret/power) */
    0x18,0x00, 0x3C,0x00, 0x66,0x00, 0x42,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    /* 20: A */
    0x18,0x00, 0x3C,0x00, 0x66,0x00, 0x7E,0x00,
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* 21: L */
    0x60,0x00, 0x60,0x00, 0x60,0x00, 0x60,0x00,
    0x60,0x00, 0x60,0x00, 0x7E,0x00, 0x00,0x00,
    /* 22: U */
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 23: T */
    0x7E,0x00, 0x18,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x18,0x00, 0x00,0x00,
    /* 24: O */
    0x3C,0x00, 0x66,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 25: R */
    0x7C,0x00, 0x66,0x00, 0x66,0x00, 0x7C,0x00,
    0x6C,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* 26: E */
    0x7E,0x00, 0x60,0x00, 0x60,0x00, 0x7C,0x00,
    0x60,0x00, 0x60,0x00, 0x7E,0x00, 0x00,0x00,
};

#define FONT_TILE_COUNT 27
#define FONT_SIZE (FONT_TILE_COUNT * 16)

/* Tile indices */
#define TILE_SPACE  0
#define TILE_0      1
#define TILE_PLUS   11
#define TILE_MINUS  12
#define TILE_MUL    13
#define TILE_DIV    14
#define TILE_EQ     15
#define TILE_C      16
#define TILE_LBRACK 17
#define TILE_RBRACK 18
#define TILE_CARET  19
#define TILE_A      20
#define TILE_L      21
#define TILE_U      22
#define TILE_T      23
#define TILE_O      24
#define TILE_R      25
#define TILE_E      26

/*============================================================================
 * Calculator State
 *============================================================================*/

extern volatile u8 vblank_flag;  /* Defined in crt0.asm, set by NMI handler */

static u8 cur_x;          /* Cursor X (0-3) */
static u8 cur_y;          /* Cursor Y (0-4) */
static u16 display_val;   /* Current display value (unsigned, like 2_calculator) */
static u16 accum;         /* Stored value for operations */
static u8 pending_op;     /* 0=none, 1=+, 2=-, 3=*, 4=/, 5=^ */
static u8 new_number;     /* Next digit starts new number */
static u8 overflow;       /* Set when result exceeds u16 max */

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
    addr = TILEMAP_ADDR + (u16)y * 32 + x;
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
 * Button Layout (5 rows x 4 cols)
 *
 *   [7] [8] [9] [/]
 *   [4] [5] [6] [*]
 *   [1] [2] [3] [-]
 *   [0] [C] [=] [+]
 *   [^]
 *============================================================================*/

/* Tile for each button position (20 slots, row 4 only has 1) */
static const u8 button_tiles[20] = {
    TILE_0 + 7, TILE_0 + 8, TILE_0 + 9, TILE_DIV,
    TILE_0 + 4, TILE_0 + 5, TILE_0 + 6, TILE_MUL,
    TILE_0 + 1, TILE_0 + 2, TILE_0 + 3, TILE_MINUS,
    TILE_0,     TILE_C,     TILE_EQ,    TILE_PLUS,
    TILE_CARET, TILE_SPACE, TILE_SPACE, TILE_SPACE
};

/* Values: 0-9 = digits, 0xF0-F4 = ops, 0xFE = clear, 0xFF = equals */
static const u8 button_values[20] = {
    7, 8, 9, 0xF3,       /* /, op 4 */
    4, 5, 6, 0xF2,       /* *, op 3 */
    1, 2, 3, 0xF1,       /* -, op 2 */
    0, 0xFE, 0xFF, 0xF0, /* C, =, +, op 1 */
    0xF4, 0, 0, 0        /* ^, op 5 */
};

#define BTN_START_X  10
#define BTN_START_Y  10
#define BTN_SPACE    4
#define DISPLAY_X    12
#define DISPLAY_Y    6
#define DISPLAY_W    7

static void draw_buttons(void) {
    u8 i, bx, by;
    for (i = 0; i < 17; i++) {
        bx = BTN_START_X + (i & 3) * BTN_SPACE;
        by = BTN_START_Y + (i >> 2) * 2;
        write_tile(bx, by, button_tiles[i]);
    }
}

static void draw_cursor(u8 show) {
    u8 bx, by;
    bx = BTN_START_X + cur_x * BTN_SPACE;
    by = BTN_START_Y + cur_y * 2;
    if (show) {
        write_tile(bx - 1, by, TILE_LBRACK);
        write_tile(bx + 1, by, TILE_RBRACK);
    } else {
        write_tile(bx - 1, by, TILE_SPACE);
        write_tile(bx + 1, by, TILE_SPACE);
    }
}

static void draw_title(void) {
    /* "CALCULATOR" at row 3 */
    write_tile(11, 3, TILE_C);
    write_tile(12, 3, TILE_A);
    write_tile(13, 3, TILE_L);
    write_tile(14, 3, TILE_C);
    write_tile(15, 3, TILE_U);
    write_tile(16, 3, TILE_L);
    write_tile(17, 3, TILE_A);
    write_tile(18, 3, TILE_T);
    write_tile(19, 3, TILE_O);
    write_tile(20, 3, TILE_R);
}

/*============================================================================
 * Display (signed s16)
 *============================================================================*/

static void update_display(void) {
    u16 val;
    u8 x;
    u8 d0, d1, d2, d3, d4;

    /* Clear stale VBlank flag — if computation (multiply/power loops)
       took longer than one frame, NMI already set vblank_flag=1.
       Without clearing, WaitForVBlank returns immediately during
       active display, and VRAM writes silently fail. */
    vblank_flag = 0;
    WaitForVBlank();

    /* Clear display area (5 digits) */
    for (x = 0; x < 5; x++) {
        write_tile(DISPLAY_X + x, DISPLAY_Y, TILE_SPACE);
    }

    if (overflow) {
        write_tile(DISPLAY_X, DISPLAY_Y, TILE_E);
        write_tile(DISPLAY_X + 1, DISPLAY_Y, TILE_R);
        write_tile(DISPLAY_X + 2, DISPLAY_Y, TILE_R);
        write_tile(DISPLAY_X + 3, DISPLAY_Y, TILE_O);
        write_tile(DISPLAY_X + 4, DISPLAY_Y, TILE_R);
        return;
    }

    val = display_val;

    /* Extract digits using repeated subtraction */
    d4 = 0;
    while (val >= 10000) { val = val - 10000; d4++; }
    d3 = 0;
    while (val >= 1000) { val = val - 1000; d3++; }
    d2 = 0;
    while (val >= 100) { val = val - 100; d2++; }
    d1 = 0;
    while (val >= 10) { val = val - 10; d1++; }
    d0 = (u8)val;

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

    a = accum;
    b = display_val;
    result = 0;
    overflow = 0;

    if (pending_op == 1) {
        /* Addition — detect overflow: if result < either operand, it wrapped */
        result = a + b;
        if (result < a) {
            overflow = 1;
        } else {
            display_val = result;
        }
    } else if (pending_op == 2) {
        /* Subtraction */
        display_val = a - b;
    } else if (pending_op == 3) {
        /* Multiplication — repeated addition with overflow check */
        while (b > 0) {
            u16 prev = result;
            result = result + a;
            if (result < prev) {
                overflow = 1;
                break;
            }
            b = b - 1;
        }
        if (!overflow) display_val = result;
    } else if (pending_op == 4) {
        /* Division */
        if (b != 0) {
            while (a >= b) {
                a = a - b;
                result = result + 1;
            }
            display_val = result;
        }
    } else if (pending_op == 5) {
        /* Power — repeated multiplication with overflow check */
        result = 1;
        while (b > 0) {
            u16 tmp = 0;
            u16 i = result;
            while (i > 0) {
                u16 prev = tmp;
                tmp = tmp + a;
                if (tmp < prev) {
                    overflow = 1;
                    break;
                }
                i = i - 1;
            }
            if (overflow) break;
            result = tmp;
            b = b - 1;
        }
        if (!overflow) display_val = result;
    }
    update_display();
}

static void handle_digit(u8 digit) {
    u16 val;
    overflow = 0;
    if (new_number) {
        display_val = 0;
        new_number = 0;
    }
    if (display_val <= 6553) {
        /* Multiply by 10 using shifts: x*10 = x*8 + x*2 = (x<<3) + (x<<1) */
        val = display_val;
        display_val = (val << 3) + (val << 1) + digit;
    }
    update_display();
}

static void handle_operator(u8 op) {
    if (pending_op != 0) {
        do_operation();
    }
    accum = display_val;
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
    display_val = 0;
    accum = 0;
    pending_op = 0;
    new_number = 1;
    overflow = 0;
    update_display();
}

static void press_button(void) {
    u8 pos, value;
    pos = cur_y * 4 + cur_x;
    if (pos >= 20) return;
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
    REG_TM = TM_BG1;

    /* Set palette: bg dark blue, text white */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x28;  /* Dark blue */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* White */

    /* Draw interface */
    draw_title();
    draw_buttons();

    /* Initialize state */
    cur_x = 0;
    cur_y = 0;
    display_val = 0;
    accum = 0;
    pending_op = 0;
    new_number = 1;
    overflow = 0;

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
        old_x = cur_x;
        old_y = cur_y;

        /* Process D-pad */
        if (pad_pressed & KEY_LEFT) {
            if (cur_x > 0) cur_x = cur_x - 1;
        }
        if (pad_pressed & KEY_RIGHT) {
            if (cur_y == 4) {
                /* Row 4 only has ^ at col 0 */
            } else {
                if (cur_x < 3) cur_x = cur_x + 1;
            }
        }
        if (pad_pressed & KEY_UP) {
            if (cur_y > 0) cur_y = cur_y - 1;
        }
        if (pad_pressed & KEY_DOWN) {
            if (cur_y < 4) cur_y = cur_y + 1;
            /* Clamp col on row 4 */
            if (cur_y == 4 && cur_x > 0) cur_x = 0;
        }

        /* Process A button */
        if (pad_pressed & KEY_A) {
            press_button();
        }

        /* Update cursor if moved */
        if (cur_x != old_x || cur_y != old_y) {
            /* Clear old cursor */
            cur_x = old_x;
            cur_y = old_y;
            draw_cursor(0);
            /* Restore position from pad input */
            if (pad_pressed & KEY_LEFT) {
                if (old_x > 0) cur_x = old_x - 1;
            }
            if (pad_pressed & KEY_RIGHT) {
                if (old_y == 4) {
                    cur_x = old_x;
                } else {
                    if (old_x < 3) cur_x = old_x + 1;
                    else cur_x = old_x;
                }
            }
            if (pad_pressed & KEY_UP) {
                if (old_y > 0) cur_y = old_y - 1;
                else cur_y = old_y;
            }
            if (pad_pressed & KEY_DOWN) {
                if (old_y < 4) cur_y = old_y + 1;
                else cur_y = old_y;
            }
            /* Clamp col on row 4 */
            if (cur_y == 4 && cur_x > 0) cur_x = 0;
            draw_cursor(1);
        }
    }

    return 0;
}
