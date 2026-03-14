#include "hud.h"
#include "render.h"

/* BG2 tilemap buffer (HUD labels + values) from data.asm */
extern u16 tilemap_bg2[];

/* BG3 message row buffer (32 entries) from data.asm */
extern u16 bg3_msgrow[];

/* String constants from data.asm */
extern const char str_score[];
extern const char str_level[];
extern const char str_lines[];
extern const char str_next[];

/* HUD column: after thick border (col 13) + 2-col gap */
#define HUD_COL  16

/* Tilemap positions for HUD elements (on BG2) */
/* Labels start at HUD_COL (col 16). Right edge = col 20 for 5-char labels. */
#define HUD_RIGHT  (HUD_COL + 4)  /* col 20: right edge of 5-char labels */

#define POS_SCORE_L  (3  * 32 + HUD_COL)              /* SCORE label */
#define POS_SCORE_V  (4  * 32 + HUD_RIGHT - 6 + 1)    /* 6 digits right-aligned */
#define POS_LEVEL_L  (7  * 32 + HUD_COL)              /* LEVEL label */
#define POS_LEVEL_V  (8  * 32 + HUD_RIGHT - 3 + 1)    /* 3 digits right-aligned */
#define POS_LINES_L  (11 * 32 + HUD_COL)              /* LINES label */
#define POS_LINES_V  (12 * 32 + HUD_RIGHT - 4 + 1)    /* 4 digits right-aligned */
#define POS_NEXT_L   (15 * 32 + HUD_COL)              /* NEXT label */

/* Message centering within bordered playfield area (cols 0-13) */
#define FIELD_LEFT    0
#define FIELD_WIDTH  14

/*
 * BG3 tilemap entry format for 2bpp:
 *   bits 0-9:   tile number (= ASCII code)
 *   bits 10-12: palette 4 (CGRAM colors 16-19)
 *   bit 13:     priority 1 (high — appears above BG1/BG2)
 *
 * palette 4 << 10 = 0x1000, priority 1 << 13 = 0x2000 → 0x3000
 */
#define BG3_ATTR  0x3800  /* palette 6 + priority high (isolated from border) */

/* Palette attributes for BG2 HUD text (breakout font uses colors 11-12) */
#define HUD_ATTR          0x0400  /* palette 1: white text */
#define HUD_ATTR_RED      0x0800  /* palette 2: magenta text */
#define HUD_ATTR_GREEN    0x0C00  /* palette 3: green text */
#define HUD_ATTR_PURPLE   0x1000  /* palette 4: purple text */
#define HUD_ATTR_ORANGE   0x1400  /* palette 5: orange text */

static void writestring_bg2_attr(const char *st, u16 pos, u16 attr) {
    u8 ch;
    while ((ch = *st)) {
        tilemap_bg2[pos] = (u16)ch | attr;
        pos++;
        st++;
    }
    bg2_dirty = 1;
}

static void writestring_bg2(const char *st, u16 pos) {
    writestring_bg2_attr(st, pos, HUD_ATTR);
}

static void writenum_bg2(u16 num, u8 len, u16 pos) {
    u8 figure;
    pos += len - 1;

    if (!num) {
        tilemap_bg2[pos] = 48 | HUD_ATTR;  /* '0' */
    } else {
        while (len && num) {
            figure = num % 10;
            tilemap_bg2[pos] = (figure + 48) | HUD_ATTR;
            num /= 10;
            pos--;
            len--;
        }
        while (len) {
            tilemap_bg2[pos] = 0;  /* empty tile = transparent */
            pos--;
            len--;
        }
    }
    bg2_dirty = 1;
}

void hudInit(void) {
    writestring_bg2_attr(str_score, POS_SCORE_L, HUD_ATTR_RED);
    writestring_bg2_attr(str_level, POS_LEVEL_L, HUD_ATTR_GREEN);
    writestring_bg2_attr(str_lines, POS_LINES_L, HUD_ATTR_PURPLE);
    writestring_bg2_attr(str_next, POS_NEXT_L, HUD_ATTR_ORANGE);

    hudUpdateScore(0);
    hudUpdateLevel(1);
    hudUpdateLines(0);
}

void hudUpdateScore(u16 score) {
    writenum_bg2(score, 6, POS_SCORE_V);
}

void hudUpdateLevel(u16 level) {
    writenum_bg2(level, 3, POS_LEVEL_V);
}

void hudUpdateLines(u16 lines) {
    writenum_bg2(lines, 4, POS_LINES_V);
}

void hudShowMessage(const char *str) {
    u8 len, col, i;
    const char *s;
    u8 ch;

    /* Clear row first */
    for (i = 0; i < 32; i++) {
        bg3_msgrow[i] = 0;
    }

    /* Measure string length */
    len = 0;
    s = str;
    while (*s) { len++; s++; }

    /* Center horizontally within wide playfield (cols 2-21) */
    col = FIELD_LEFT;
    if (len < FIELD_WIDTH) {
        col += (u8)((FIELD_WIDTH - len) >> 1);
    }

    /* Write to message row buffer with palette 4 + priority high */
    s = str;
    while ((ch = *s)) {
        bg3_msgrow[col] = (u16)ch | BG3_ATTR;
        col++;
        s++;
    }
    bg3_dirty = 1;
}

void hudClearMessage(void) {
    u8 i;
    for (i = 0; i < 32; i++) {
        bg3_msgrow[i] = 0;
    }
    bg3_dirty = 1;
}
