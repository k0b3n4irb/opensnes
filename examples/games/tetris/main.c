/**
 * @file main.c
 * @brief Tetris — Game loop, state machine, input (DAS), gravity
 *
 * Mode 1, 3-layer architecture:
 *   BG1 (4bpp): Playfield — border + locked blocks + falling piece
 *   BG2 (4bpp): HUD — labels, score/level/lines, next piece preview
 *   BG3 (2bpp): Message overlay — PRESS START, PAUSED, GAME OVER
 *
 * BG3 with MODE1_PRIORITY_HIGH makes messages float above the playfield.
 * Dirty flags minimize DMA: only changed layers are flushed per VBlank.
 */

#include <snes.h>
#include "board.h"
#include "piece.h"  /* pieceGetCellRow/Col, pieceSpawnRow/Col, etc. */
#include "render.h"
#include "hud.h"

/* Input buffer from NMI handler */
extern u16 pad_keys[];

/* String constants from data.asm */
extern const char str_paused[];
extern const char str_gameover[];
extern const char str_start[];

/*============================================================================
 * Game States
 *============================================================================*/

#define STATE_TITLE      0
#define STATE_PLAYING    1
#define STATE_LINE_CLEAR 2
#define STATE_GAME_OVER  3

/*============================================================================
 * NES Gravity Speed Table (frames per drop)
 *============================================================================*/

static const u8 speed_table[] = {
    48, 43, 38, 33, 28, 23, 18, 13, 8, 6,   /* L0-9: NES NTSC */
    5, 5, 5, 4, 4, 4, 3, 3, 3,              /* L10-18 */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1         /* L19-29+ */
};

#define SPEED_TABLE_LEN 30

/*============================================================================
 * DAS (Delayed Auto Shift) Constants
 *============================================================================*/

#define DAS_INITIAL_DELAY  16  /* ~267ms at 60fps */
#define DAS_REPEAT_DELAY    6  /* ~100ms at 60fps */

/*============================================================================
 * NES Scoring
 *============================================================================*/

static const u16 line_scores[] = { 0, 40, 100, 300, 1200 };

/*============================================================================
 * Game State Variables
 *============================================================================*/

static u8  game_state;
static u8  cur_type, cur_rot;
static s8  cur_row, cur_col;
static u8  next_type;
static u16 score;
static u16 level;
static u16 total_lines;
static u8  lines_until_levelup;  /* counts down from 10; avoids __div16 */
static u8  gravity_timer;
static u8  gravity_speed;
static u8  paused;

/* DAS state */
static u8  das_left_timer;
static u8  das_right_timer;
static u8  das_down_timer;

/* Line clear animation */
static LineClearResult clear_result;
static u8 flash_timer;
#define FLASH_FRAMES 10

/* Previous frame's pad state for edge detection */
static u16 prev_pad;

/* Rainbow color cycle for message text (BGR555, SNESS palette) */
static const u16 msg_colors[] = {
    0x7FFF,  /* white */
    0x307C,  /* magenta (#e71861) */
    0x0B5E,  /* yellow (#f6d714) */
    0x2B61,  /* green (#0bdb52) */
    0x6EF0,  /* cyan (#80bedb) */
    0x6E2E,  /* purple (#7161de) */
    0x01BC,  /* orange (#dc6f0f) */
    0x3E22,  /* teal (#138e7d) */
};
#define MSG_COLOR_COUNT  8
#define MSG_COLOR_SPEED  8  /* frames per color */

/*============================================================================
 * Utility
 *============================================================================*/

static u8 getGravitySpeed(void) {
    u16 idx = level;
    if (idx >= SPEED_TABLE_LEN) idx = SPEED_TABLE_LEN - 1;
    return speed_table[idx];
}

static void addScore(u8 lines_cleared) {
    u16 pts;
    u16 lv;
    u8 i;

    if (lines_cleared == 0 || lines_cleared > 4) return;

    /* score += line_scores[n] * (level + 1), using repeated addition */
    pts = line_scores[lines_cleared];
    lv = level + 1;
    for (i = 0; i < (u8)lv; i++) {
        score += pts;
        if (score > 65000) {
            score = 65000;  /* cap to avoid u16 overflow */
            break;
        }
    }
}

/*============================================================================
 * Piece Spawning
 *============================================================================*/

static u8 spawnPiece(void) {
    cur_type = next_type;
    cur_rot  = 0;
    cur_row  = pieceSpawnRow(cur_type);
    cur_col  = pieceSpawnCol(cur_type);

    next_type = pieceRandom();
    renderNextPiece(next_type);

    /* Check if spawn position is blocked (game over) */
    if (boardCheckCollision(cur_type, cur_rot, cur_row, cur_col)) {
        return 1;  /* blocked = game over */
    }

    return 0;
}

/*============================================================================
 * Input Handling
 *============================================================================*/

static void handleInput(void) {
    u16 pad;
    u16 pressed;
    u8 new_rot;
    s8 new_col;

    pad = pad_keys[0];
    pressed = pad & ~prev_pad;  /* newly pressed this frame */

    /* --- Pause toggle --- */
    if (pressed & KEY_START) {
        if (paused) {
            paused = 0;
            hudClearMessage();
        } else {
            paused = 1;
            hudShowMessage(str_paused);
            /* Redraw piece so it's visible during pause */
            renderPiece(cur_type, cur_rot, cur_row, cur_col);
            WaitForVBlank();
            renderFlush();
            /* Wait until START released, then pressed again, then released */
            do { WaitForVBlank(); } while (pad_keys[0] & KEY_START);
            do { WaitForVBlank(); } while ((pad_keys[0] & KEY_START) == 0);
            do { WaitForVBlank(); } while (pad_keys[0] & KEY_START);
            hudClearMessage();
            /* Erase piece to restore clean delta state */
            renderErasePiece(cur_type, cur_rot, cur_row, cur_col);
            paused = 0;
            prev_pad = pad_keys[0];
            return;
        }
    }

    if (paused) return;

    /* --- Rotation (edge-triggered) --- */
    if (pressed & KEY_A) {
        new_rot = pieceRotateCW(cur_rot);
        if (!boardCheckCollision(cur_type, new_rot, cur_row, cur_col)) {
            cur_rot = new_rot;
        }
    }
    if (pressed & KEY_B) {
        new_rot = pieceRotateCCW(cur_rot);
        if (!boardCheckCollision(cur_type, new_rot, cur_row, cur_col)) {
            cur_rot = new_rot;
        }
    }

    /* --- Horizontal movement (DAS) --- */
    if (pad & KEY_LEFT) {
        if (das_left_timer == 0 || das_left_timer >= DAS_INITIAL_DELAY) {
            new_col = cur_col - 1;
            if (!boardCheckCollision(cur_type, cur_rot, cur_row, new_col)) {
                cur_col = new_col;
            }
            if (das_left_timer >= DAS_INITIAL_DELAY) {
                das_left_timer = DAS_INITIAL_DELAY - DAS_REPEAT_DELAY;
            }
        }
        das_left_timer++;
    } else {
        das_left_timer = 0;
    }

    if (pad & KEY_RIGHT) {
        if (das_right_timer == 0 || das_right_timer >= DAS_INITIAL_DELAY) {
            new_col = cur_col + 1;
            if (!boardCheckCollision(cur_type, cur_rot, cur_row, new_col)) {
                cur_col = new_col;
            }
            if (das_right_timer >= DAS_INITIAL_DELAY) {
                das_right_timer = DAS_INITIAL_DELAY - DAS_REPEAT_DELAY;
            }
        }
        das_right_timer++;
    } else {
        das_right_timer = 0;
    }

    /* --- Soft drop (holding DOWN speeds up gravity) --- */
    if (pad & KEY_DOWN) {
        if (das_down_timer == 0 || das_down_timer >= 3) {
            gravity_timer = gravity_speed;  /* force drop next frame */
            if (das_down_timer >= 3) {
                das_down_timer = 1;
            }
        }
        das_down_timer++;
    } else {
        das_down_timer = 0;
    }

    prev_pad = pad;
}

/*============================================================================
 * Game Logic
 *============================================================================*/

static void startGame(void) {
    boardClear();
    score = 0;
    level = 0;
    total_lines = 0;
    lines_until_levelup = 10;
    gravity_timer = 0;
    gravity_speed = getGravitySpeed();
    paused = 0;
    das_left_timer = 0;
    das_right_timer = 0;
    das_down_timer = 0;
    prev_pad = 0;
    flash_timer = 0;

    srand(pad_keys[0]);  /* seed RNG from joypad timing */
    next_type = pieceRandom();

    renderBoard();
    hudInit();

    /* Force blank for guaranteed DMA of all layers (BG1+BG2+BG3 = ~4KB) */
    WaitForVBlank();
    setScreenOff();
    renderFlush();
    setScreenOn();
    renderEnableGradient();

    if (spawnPiece()) {
        game_state = STATE_GAME_OVER;
        return;
    }

    game_state = STATE_PLAYING;
}

static void statePlaying(void) {
    s8 new_row;

    /* Ensure scroll is reset (safety after shake) */
    renderShake(0, 0);

    /* Delta rendering: erase piece from tilemap (restores board cells) */
    renderErasePiece(cur_type, cur_rot, cur_row, cur_col);

    handleInput();

    if (paused) {
        /* Re-draw piece before returning (tilemap must show piece) */
        renderPiece(cur_type, cur_rot, cur_row, cur_col);
        return;
    }

    /* Gravity */
    gravity_timer++;
    if (gravity_timer >= gravity_speed) {
        gravity_timer = 0;

        new_row = cur_row + 1;
        if (!boardCheckCollision(cur_type, cur_rot, new_row, cur_col)) {
            cur_row = new_row;
        } else {
            /* Lock piece */
            boardLockPiece(cur_type, cur_rot, cur_row, cur_col);

            /* Check for line clears */
            if (boardFindFullLines(&clear_result) > 0) {
                renderBoard();
                flash_timer = 0;
                game_state = STATE_LINE_CLEAR;
                return;
            }

            /* Sync board and spawn next */
            renderBoard();
            if (spawnPiece()) {
                game_state = STATE_GAME_OVER;
                return;
            }
            renderPiece(cur_type, cur_rot, cur_row, cur_col);
            return;
        }
    }

    /* Draw piece at current position */
    renderPiece(cur_type, cur_rot, cur_row, cur_col);
}

/* Screen shake offsets (alternating pattern) */
static const s8 shake_dx[] = { 2, -2, 1, -1, 2, -1, 1, 0 };
static const s8 shake_dy[] = { -1, 1, -2, 1, 0, -1, 1, 0 };

static void stateLineClear(void) {
    flash_timer++;

    /* Screen shake during flash */
    renderShake(shake_dx[flash_timer & 7], shake_dy[flash_timer & 7]);

    /* Flash only the clearing rows (no full board sync needed per frame) */
    renderLineClearFlash(&clear_result, flash_timer);

    if (flash_timer >= FLASH_FRAMES) {
        /* Reset shake */
        renderShake(0, 0);

        /* Remove cleared lines from board FIRST */
        boardRemoveLines(&clear_result);

        /* Update scoring */
        addScore(clear_result.count);
        total_lines += clear_result.count;

        /* Level up every 10 lines (counter-based, no __div16) */
        {
            u8 i;
            for (i = 0; i < clear_result.count; i++) {
                if (lines_until_levelup == 0) {
                    lines_until_levelup = 10;
                    level++;
                    gravity_speed = getGravitySpeed();
                }
                lines_until_levelup--;
            }
        }

        hudUpdateScore(score);
        hudUpdateLevel(level + 1);
        hudUpdateLines(total_lines);

        /* Full sync after rows shifted down */
        renderBoard();

        /* Spawn next piece */
        if (spawnPiece()) {
            game_state = STATE_GAME_OVER;
            return;
        }

        renderPiece(cur_type, cur_rot, cur_row, cur_col);

        /* Force blank flush: completion frame DMAs BG1+BG2 (~3.3KB).
         * NMI handler consumes ~15K mc of VBlank; not enough time remains
         * for both layers in main-thread renderFlush. Force blank guarantees
         * all VRAM writes succeed. Same pattern as startGame/stateGameOver. */
        WaitForVBlank();
        setScreenOff();
        renderFlush();
        setScreenOn();
        renderEnableGradient();

        game_state = STATE_PLAYING;
    }
}

static void stateGameOver(void) {
    u8 timer = 0;
    u8 cidx = 0;

    renderBoard();
    renderPiece(cur_type, cur_rot, cur_row, cur_col);
    hudShowMessage(str_gameover);

    /* Force blank guarantees DMA of all layers (BG1+BG3) */
    WaitForVBlank();
    setScreenOff();
    renderFlush();
    setScreenOn();
    renderEnableGradient();

    /* Rainbow cycle on GAME OVER until player presses START */
    while ((pad_keys[0] & KEY_START) == 0) {
        timer++;
        if (timer >= MSG_COLOR_SPEED) {
            timer = 0;
            cidx++;
            if (cidx >= MSG_COLOR_COUNT) cidx = 0;
            renderSetMsgColor(msg_colors[cidx]);
        }
        WaitForVBlank();
        renderFlush();
    }
    do { WaitForVBlank(); } while (pad_keys[0] & KEY_START);

    renderSetMsgColor(0x7FFF);
    hudClearMessage();
    startGame();
}

static void stateTitle(void) {
    u8 timer = 0;
    u8 cidx = 0;

    /* Rainbow cycle on PRESS START until player presses START */
    while ((pad_keys[0] & KEY_START) == 0) {
        timer++;
        if (timer >= MSG_COLOR_SPEED) {
            timer = 0;
            cidx++;
            if (cidx >= MSG_COLOR_COUNT) cidx = 0;
            renderSetMsgColor(msg_colors[cidx]);
        }
        WaitForVBlank();
        renderFlush();
    }
    /* Wait for START release */
    do { WaitForVBlank(); } while (pad_keys[0] & KEY_START);

    renderSetMsgColor(0x7FFF);  /* restore white */
    hudClearMessage();
    startGame();
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u8 i;

    /* Initialize joypads */
    pad_keys[0] = 0;
    pad_keys[1] = 0;

    for (i = 0; i < 5; i++) {
        WaitForVBlank();
    }

    /* Initialize renderer (PPU, tiles, palette, borders — screen stays OFF) */
    renderInit();

    /* Set up title screen content while still in force blank */
    hudInit();
    hudShowMessage(str_start);

    /* Flush HUD + message to VRAM (force blank = guaranteed write) */
    WaitForVBlank();
    renderFlush();

    /* Display comes on with everything visible at once */
    setScreenOn();

    /* Enable HDMA gradient (must be after setScreenOn) */
    renderEnableGradient();

    game_state = STATE_TITLE;

    while (1) {
        switch (game_state) {
            case STATE_TITLE:
                stateTitle();
                break;
            case STATE_PLAYING:
                statePlaying();
                break;
            case STATE_LINE_CLEAR:
                stateLineClear();
                break;
            case STATE_GAME_OVER:
                stateGameOver();
                break;
        }

        WaitForVBlank();
        renderFlush();
    }

    return 0;
}
