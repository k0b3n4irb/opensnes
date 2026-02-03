/**
 * @file main.c
 * @brief SRAM Save Game Example
 *
 * Demonstrates battery-backed SRAM for save games:
 * - Saving game data to SRAM
 * - Loading from SRAM on startup
 * - Magic byte validation (detect new/corrupt saves)
 * - Checksum verification
 *
 * Controls:
 * - D-pad Up/Down: Increase/decrease counter
 * - A button: Save to SRAM
 * - B button: Load from SRAM
 * - Select: Clear save (reset to defaults)
 *
 * The counter value persists between power cycles when saved.
 */

#include <snes.h>
#include <snes/sram.h>

/*============================================================================
 * Save Data Structure
 *============================================================================*/

/* Magic bytes to identify valid save data */
#define SAVE_MAGIC_0 'O'
#define SAVE_MAGIC_1 'S'
#define SAVE_MAGIC_2 'N'
#define SAVE_MAGIC_3 'S'

typedef struct {
    u8 magic[4];        /* "OSNS" - to verify save is valid */
    u16 counter;        /* The counter value */
    u8 times_saved;     /* How many times we've saved */
    u8 checksum;        /* Simple XOR checksum */
} SaveData;

static SaveData save_data;

/* Default values for new game */
#define DEFAULT_COUNTER 0
#define DEFAULT_TIMES_SAVED 0

/*============================================================================
 * Display
 *============================================================================*/

#define TILEMAP_ADDR  0x0400
#define TILES_ADDR    0x0000

/* Embedded 2bpp font - digits 0-9, A-Z, some symbols */
static const u8 font[] = {
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
    0x0E,0x00, 0x1E,0x00, 0x36,0x00, 0x66,0x00,
    0x7F,0x00, 0x06,0x00, 0x06,0x00, 0x00,0x00,
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
    /* 11: A */
    0x18,0x00, 0x3C,0x00, 0x66,0x00, 0x7E,0x00,
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* 12: B */
    0x7C,0x00, 0x66,0x00, 0x66,0x00, 0x7C,0x00,
    0x66,0x00, 0x66,0x00, 0x7C,0x00, 0x00,0x00,
    /* 13: C */
    0x3C,0x00, 0x66,0x00, 0x60,0x00, 0x60,0x00,
    0x60,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 14: D */
    0x78,0x00, 0x6C,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x6C,0x00, 0x78,0x00, 0x00,0x00,
    /* 15: E */
    0x7E,0x00, 0x60,0x00, 0x60,0x00, 0x78,0x00,
    0x60,0x00, 0x60,0x00, 0x7E,0x00, 0x00,0x00,
    /* 16: F (unused) */
    0x7E,0x00, 0x60,0x00, 0x60,0x00, 0x78,0x00,
    0x60,0x00, 0x60,0x00, 0x60,0x00, 0x00,0x00,
    /* 17: G (unused) */
    0x3C,0x00, 0x66,0x00, 0x60,0x00, 0x6E,0x00,
    0x66,0x00, 0x66,0x00, 0x3E,0x00, 0x00,0x00,
    /* 18: I */
    0x3C,0x00, 0x18,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x3C,0x00, 0x00,0x00,
    /* 19: L */
    0x60,0x00, 0x60,0x00, 0x60,0x00, 0x60,0x00,
    0x60,0x00, 0x60,0x00, 0x7E,0x00, 0x00,0x00,
    /* 20: M */
    0x63,0x00, 0x77,0x00, 0x7F,0x00, 0x6B,0x00,
    0x63,0x00, 0x63,0x00, 0x63,0x00, 0x00,0x00,
    /* 21: N */
    0x66,0x00, 0x76,0x00, 0x7E,0x00, 0x7E,0x00,
    0x6E,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* 22: O */
    0x3C,0x00, 0x66,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 23: R */
    0x7C,0x00, 0x66,0x00, 0x66,0x00, 0x7C,0x00,
    0x6C,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* 24: S */
    0x3E,0x00, 0x60,0x00, 0x60,0x00, 0x3C,0x00,
    0x06,0x00, 0x06,0x00, 0x7C,0x00, 0x00,0x00,
    /* 25: T */
    0x7E,0x00, 0x18,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x18,0x00, 0x00,0x00,
    /* 26: U */
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 27: V */
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x66,0x00,
    0x3C,0x00, 0x3C,0x00, 0x18,0x00, 0x00,0x00,
    /* 28: W */
    0x63,0x00, 0x63,0x00, 0x63,0x00, 0x6B,0x00,
    0x7F,0x00, 0x77,0x00, 0x63,0x00, 0x00,0x00,
    /* 29: colon */
    0x00,0x00, 0x18,0x00, 0x18,0x00, 0x00,0x00,
    0x18,0x00, 0x18,0x00, 0x00,0x00, 0x00,0x00,
    /* 30: ! */
    0x18,0x00, 0x18,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x00,0x00, 0x18,0x00, 0x00,0x00,
    /* 31: P */
    0x7C,0x00, 0x66,0x00, 0x66,0x00, 0x7C,0x00,
    0x60,0x00, 0x60,0x00, 0x60,0x00, 0x00,0x00,
};

/* Tile indices for our character set */
#define T_SP 0
#define T_0  1
#define T_1  2
#define T_2  3
#define T_3  4
#define T_4  5
#define T_5  6
#define T_6  7
#define T_7  8
#define T_8  9
#define T_9  10
#define T_A  11
#define T_B  12
#define T_C  13
#define T_D  14
#define T_E  15
#define T_F  16
#define T_G  17
#define T_I  18
#define T_L  19
#define T_M  20
#define T_N  21
#define T_O  22
#define T_R  23
#define T_S  24
#define T_T  25
#define T_U  26
#define T_V  27
#define T_W  28
#define T_COL 29
#define T_EX 30
#define T_P  31

/* Display state */
static u8 status_row;

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void write_tile(u8 x, u8 y, u8 tile) {
    u16 addr = TILEMAP_ADDR + y * 32 + x;
    REG_VMAIN = 0x80;
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;
    REG_VMDATAL = tile;
    REG_VMDATAH = 0;
}

static void write_number(u8 x, u8 y, u16 num) {
    u8 digits[5];
    u8 i;
    u8 digit_count = 0;
    u16 temp = num;

    /* Extract digits (reverse order) */
    if (temp == 0) {
        digits[0] = 0;
        digit_count = 1;
    } else {
        while (temp > 0 && digit_count < 5) {
            digits[digit_count] = temp - ((temp / 10) * 10);
            temp = temp / 10;
            digit_count = digit_count + 1;
        }
    }

    /* Write digits (reversed to correct order) */
    for (i = 0; i < digit_count; i++) {
        write_tile(x + (digit_count - 1 - i), y, T_0 + digits[i]);
    }

    /* Pad with spaces */
    for (i = digit_count; i < 5; i++) {
        write_tile(x - (i - digit_count + 1), y, T_SP);
    }
}

static void clear_row(u8 y) {
    u8 i;
    for (i = 0; i < 32; i++) {
        write_tile(i, y, T_SP);
    }
}

static void draw_title(void) {
    /* "SRAM SAVE DEMO" */
    u8 title[] = { T_S, T_R, T_A, T_M, T_SP, T_S, T_A, T_V, T_E, T_SP, T_D, T_E, T_M, T_O };
    u8 i;
    for (i = 0; i < 14; i++) {
        write_tile(9 + i, 2, title[i]);
    }
}

static void draw_labels(void) {
    /* "COUNTER:" at row 5 */
    u8 lbl_counter[] = { T_C, T_O, T_U, T_N, T_T, T_E, T_R, T_COL };
    u8 i;
    for (i = 0; i < 8; i++) {
        write_tile(4 + i, 5, lbl_counter[i]);
    }

    /* "SAVES:" at row 7 */
    u8 lbl_saves[] = { T_S, T_A, T_V, T_E, T_S, T_COL };
    for (i = 0; i < 6; i++) {
        write_tile(6 + i, 7, lbl_saves[i]);
    }

    /* Controls instructions */
    /* "UP DOWN COUNTER" at row 12 */
    u8 lbl_updown[] = { T_U, T_P, T_SP, T_D, T_O, T_W, T_N, T_SP, T_C, T_O, T_U, T_N, T_T, T_E, T_R };
    for (i = 0; i < 15; i++) {
        write_tile(8 + i, 12, lbl_updown[i]);
    }

    /* "A SAVE  B LOAD" at row 14 */
    u8 lbl_ab[] = { T_A, T_SP, T_S, T_A, T_V, T_E, T_SP, T_SP, T_B, T_SP, T_L, T_O, T_A, T_D };
    for (i = 0; i < 14; i++) {
        write_tile(9 + i, 14, lbl_ab[i]);
    }

    /* "SELECT CLEAR" at row 16 */
    u8 lbl_sel[] = { T_S, T_E, T_L, T_E, T_C, T_T, T_SP, T_C, T_L, T_E, T_A, T_R };
    for (i = 0; i < 12; i++) {
        write_tile(10 + i, 16, lbl_sel[i]);
    }
}

static void draw_values(void) {
    write_number(18, 5, save_data.counter);
    write_number(18, 7, save_data.times_saved);
}

static void show_status(const u8 *text, u8 len) {
    u8 i;
    clear_row(status_row);
    for (i = 0; i < len; i++) {
        write_tile(11 + i, status_row, text[i]);
    }
}

/*============================================================================
 * Save/Load Functions
 *============================================================================*/

static u8 compute_checksum(void) {
    u8 *data = (u8*)&save_data;
    u8 sum = 0;
    u8 i;

    /* XOR all bytes except the checksum itself */
    for (i = 0; i < sizeof(SaveData) - 1; i++) {
        sum = sum ^ data[i];
    }
    return sum;
}

static u8 validate_save(void) {
    /* Check magic bytes */
    if (save_data.magic[0] != SAVE_MAGIC_0) return 0;
    if (save_data.magic[1] != SAVE_MAGIC_1) return 0;
    if (save_data.magic[2] != SAVE_MAGIC_2) return 0;
    if (save_data.magic[3] != SAVE_MAGIC_3) return 0;

    /* Verify checksum */
    if (compute_checksum() != save_data.checksum) return 0;

    return 1;
}

static void init_default_save(void) {
    save_data.magic[0] = SAVE_MAGIC_0;
    save_data.magic[1] = SAVE_MAGIC_1;
    save_data.magic[2] = SAVE_MAGIC_2;
    save_data.magic[3] = SAVE_MAGIC_3;
    save_data.counter = DEFAULT_COUNTER;
    save_data.times_saved = DEFAULT_TIMES_SAVED;
    save_data.checksum = compute_checksum();
}

static void do_save(void) {
    /* Update checksum and save count */
    save_data.times_saved = save_data.times_saved + 1;
    save_data.checksum = compute_checksum();

    /* Save to SRAM */
    sramSave((u8*)&save_data, sizeof(SaveData));

    /* Status message: "SAVED!" */
    u8 msg[] = { T_S, T_A, T_V, T_E, T_D, T_EX };
    show_status(msg, 6);

    draw_values();
}

static void do_load(void) {
    /* Load from SRAM */
    sramLoad((u8*)&save_data, sizeof(SaveData));

    if (validate_save()) {
        /* "LOADED!" */
        u8 msg[] = { T_L, T_O, T_A, T_D, T_E, T_D, T_EX };
        show_status(msg, 7);
    } else {
        /* Invalid save - reset to defaults */
        init_default_save();
        /* "NO SAVE" */
        u8 msg[] = { T_N, T_O, T_SP, T_S, T_A, T_V, T_E };
        show_status(msg, 7);
    }

    draw_values();
}

static void do_clear(void) {
    /* Reset to default values */
    init_default_save();

    /* Clear SRAM */
    sramClear(sizeof(SaveData));

    /* "CLEARED!" */
    u8 msg[] = { T_C, T_L, T_E, T_A, T_R, T_E, T_D, T_EX };
    show_status(msg, 8);

    draw_values();
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u16 pad, pad_prev, pad_pressed;
    u16 i;

    /* Initialize console */
    consoleInit();
    setMode(BG_MODE0, 0);

    /* Load font */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0;
    REG_VMADDH = 0;
    for (i = 0; i < sizeof(font); i += 2) {
        REG_VMDATAL = font[i];
        REG_VMDATAH = font[i + 1];
    }

    /* Clear tilemap */
    REG_VMADDL = TILEMAP_ADDR & 0xFF;
    REG_VMADDH = TILEMAP_ADDR >> 8;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0;
    }

    /* Configure BG1 */
    REG_BG1SC = 0x04;
    REG_BG12NBA = 0x00;
    REG_TM = TM_BG1;

    /* Set palette - dark blue background, white text */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x28;  /* Dark blue */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* White */

    /* Set status row */
    status_row = 10;

    /* Draw static UI */
    draw_title();
    draw_labels();

    /* Try to load existing save */
    do_load();

    /* Enable screen */
    setScreenOn();

    /* Initialize input state */
    WaitForVBlank();
    while (REG_HVBJOY & 0x01) {}
    pad_prev = REG_JOY1L | (REG_JOY1H << 8);

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Read joypad */
        while (REG_HVBJOY & 0x01) {}
        pad = REG_JOY1L | (REG_JOY1H << 8);
        pad_pressed = pad & ~pad_prev;
        pad_prev = pad;

        /* Skip invalid reads */
        if (pad == 0xFFFF) continue;

        /* Handle input */
        if (pad_pressed & KEY_UP) {
            if (save_data.counter < 9999) {
                save_data.counter = save_data.counter + 1;
                draw_values();
            }
        }

        if (pad_pressed & KEY_DOWN) {
            if (save_data.counter > 0) {
                save_data.counter = save_data.counter - 1;
                draw_values();
            }
        }

        if (pad_pressed & KEY_A) {
            do_save();
        }

        if (pad_pressed & KEY_B) {
            do_load();
        }

        if (pad_pressed & KEY_SELECT) {
            do_clear();
        }
    }

    return 0;
}
