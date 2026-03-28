/**
 * @file main.c
 * @brief Tetris with 3-layer BG architecture, DAS input, and SNESMOD audio
 * @ingroup examples
 *
 * A full Tetris implementation demonstrating a 3-layer Mode 1 architecture
 * where each BG layer serves a distinct purpose: BG1 (4bpp) renders the
 * playfield with border, locked blocks, and falling piece; BG2 (4bpp)
 * displays the HUD with score, level, lines, and next piece preview;
 * BG3 (2bpp) provides a message overlay (PRESS START, PAUSED, GAME OVER)
 * that floats above the playfield using MODE1_PRIORITY_HIGH.
 *
 * Input uses NES-authentic Delayed Auto Shift (DAS): an initial delay of
 * 16 frames before auto-repeat kicks in at 6-frame intervals. Gravity
 * follows the NES NTSC speed table (48 frames/drop at level 0 down to
 * 1 frame/drop at level 29+). Scoring uses NES multipliers.
 *
 * The renderer uses dirty flags to minimize VBlank DMA -- only changed
 * layers are flushed each frame. Heavy DMA frames (line clear completion,
 * game over) use force blank to guarantee all VRAM writes succeed.
 * An HDMA gradient provides a color wash effect on the background.
 *
 * @par SNES Concepts
 * - Mode 1 three-layer architecture (playfield / HUD / message overlay)
 * - BG3 priority bit for floating overlay above 4bpp layers
 * - Dirty-flag DMA: selective per-layer VRAM updates each VBlank
 * - Force blank for heavy multi-layer DMA (BG1+BG2+BG3 > 4KB)
 * - HDMA gradient effect on background palette
 * - Delayed Auto Shift (DAS) for responsive piece movement
 * - NES gravity speed table and scoring system
 * - SNESMOD: SPC700 music playback (Tetris theme)
 * - State machine: TITLE, PLAYING, LINE_CLEAR, GAME_OVER
 * - Screen shake effect during line clear animation
 *
 * @par What to Observe
 * - Rainbow-cycling "PRESS START" on the title screen
 * - Pieces fall with increasing speed as level rises
 * - D-pad moves pieces (DAS auto-repeat after holding); A/B rotates
 * - Completed lines flash and shake the screen before clearing
 * - Score, level, and line count update in the right-side HUD
 * - Next piece preview shows the upcoming tetromino
 * - START pauses with a "PAUSED" overlay; GAME OVER cycles rainbow text
 *
 * @par Modules Used
 * console, sprite, dma, background, input
 *
 * @see background.h, dma.h, input.h, snesmod.h
 */

#include <snes.h>
#include <snes/snesmod.h>
#include "soundbank.h"
#include "board.h"
#include "piece.h"  /* pieceGetCellRow/Col, pieceSpawnRow/Col, etc. */
#include "render.h"
#include "hud.h"

/**
 * @brief Raw joypad state buffer written by the NMI handler every frame.
 *
 * Used directly (instead of padHeld()) for precise edge detection:
 * pressed = pad_keys[0] & ~prev_pad gives newly-pressed buttons this frame.
 */
extern u16 pad_keys[];

/**
 * @brief Global frame counter incremented by the NMI handler.
 *
 * Used to seed srand() at game start -- the player's variable-length
 * wait on the title screen produces a different seed each game, ensuring
 * different piece sequences.
 */
/** @brief "PAUSED" overlay message string (in data.asm for bank $00 placement) */
extern const char str_paused[];
/** @brief "GAME OVER" overlay message string */
extern const char str_gameover[];
/** @brief "PRESS START" title screen message string */
extern const char str_start[];

/*============================================================================
 * Game States
 *============================================================================*/

/** @brief Title screen: displaying "PRESS START" with rainbow cycle */
#define STATE_TITLE      0
/** @brief Active gameplay: piece falling, player input, gravity */
#define STATE_PLAYING    1
/** @brief Line clear animation: flashing rows + screen shake */
#define STATE_LINE_CLEAR 2
/** @brief Game over: board frozen, "GAME OVER" rainbow cycle, await restart */
#define STATE_GAME_OVER  3

/*============================================================================
 * NES Gravity Speed Table (frames per drop)
 *============================================================================*/

/**
 * @brief NES NTSC gravity speed table (frames per automatic drop).
 *
 * Index = level number. Level 0 = 48 frames/drop (slowest), level 29+ = 1
 * frame/drop (fastest). Values match the original NES Tetris for authentic feel.
 *
 * NOT const -- const arrays go to SUPERFREE ROM which may land in bank $01+.
 * The compiler generates `lda.l $0000,x` (always bank $00) for array access,
 * so const data in bank $01 would read garbage. Mutable arrays go to WRAM
 * (bank $00) via CopyInitData at startup.
 */
static u8 speed_table[] = {
    48, 43, 38, 33, 28, 23, 18, 13, 8, 6,   /* L0-9: NES NTSC */
    5, 5, 5, 4, 4, 4, 3, 3, 3,              /* L10-18 */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1         /* L19-29+ */
};

/** @brief Number of entries in speed_table (levels 0-29) */
#define SPEED_TABLE_LEN 30

/*============================================================================
 * DAS (Delayed Auto Shift) Constants
 *============================================================================*/

/** @brief DAS initial delay before auto-repeat starts (~267ms at 60fps) */
#define DAS_INITIAL_DELAY  16
/** @brief DAS repeat interval once auto-repeat is active (~100ms at 60fps) */
#define DAS_REPEAT_DELAY    6

/*============================================================================
 * NES Scoring
 *============================================================================*/

/**
 * @brief NES scoring table: base points per number of simultaneous line clears.
 *
 * Index 0 = 0 lines (unused), 1 = single (40), 2 = double (100),
 * 3 = triple (300), 4 = Tetris (1200). Multiplied by (level + 1).
 * Not const -- see speed_table comment about bank $00 requirement.
 */
static u16 line_scores[] = { 0, 40, 100, 300, 1200 };

/*============================================================================
 * Game State Variables
 *============================================================================*/

static u8  game_state;           /**< Current game state (STATE_TITLE/PLAYING/etc.) */
static u8  cur_type;             /**< Current falling piece type (0-6, one per tetromino shape) */
static u8  cur_rot;              /**< Current piece rotation (0-3, quarter turns) */
static s8  cur_row;              /**< Current piece row on the board (signed: can be negative during spawn) */
static s8  cur_col;              /**< Current piece column on the board */
static u8  next_type;            /**< Next piece type (shown in the preview box) */
static u16 score;                /**< Player's current score (capped at 65000 to avoid u16 overflow) */
static u16 level;                /**< Current level (0-based, increases every 10 lines) */
static u16 total_lines;          /**< Total lines cleared this game */
static u8  lines_until_levelup;  /**< Countdown from 10; avoids expensive __div16 for modulo */
static u8  gravity_timer;        /**< Frames since last automatic drop */
static u8  gravity_speed;        /**< Frames per drop at current level (from speed_table) */
static u8  paused;               /**< Nonzero when game is paused */

static u8  das_left_timer;       /**< DAS timer for LEFT button (0=idle, >=DAS_INITIAL_DELAY=repeating) */
static u8  das_right_timer;      /**< DAS timer for RIGHT button */
static u8  das_down_timer;       /**< DAS timer for DOWN button (soft drop) */

/** @brief Result of the last boardFindFullLines() call (which rows are full) */
static LineClearResult clear_result;
static u8 flash_timer;          /**< Frame counter during line clear flash animation */
/** @brief Duration of the line clear flash animation in frames */
#define FLASH_FRAMES 10

/** @brief Previous frame's joypad state (for edge-triggered button detection) */
static u16 prev_pad;

/**
 * @brief Rainbow color cycle palette for animated message text (BGR555).
 *
 * Cycled through at MSG_COLOR_SPEED frames per color to create a
 * rainbow shimmer on "PRESS START" and "GAME OVER" overlay text.
 * Not const -- see speed_table comment about bank $00 requirement.
 */
static u16 msg_colors[] = {
    0x7FFF,  /* white */
    0x307C,  /* magenta (#e71861) */
    0x0B5E,  /* yellow (#f6d714) */
    0x2B61,  /* green (#0bdb52) */
    0x6EF0,  /* cyan (#80bedb) */
    0x6E2E,  /* purple (#7161de) */
    0x01BC,  /* orange (#dc6f0f) */
    0x3E22,  /* teal (#138e7d) */
};
/** @brief Number of colors in the rainbow cycle array */
#define MSG_COLOR_COUNT  8
/** @brief Frames to display each color before advancing to the next */
#define MSG_COLOR_SPEED  8

/*============================================================================
 * Utility
 *============================================================================*/

/**
 * @brief Look up the gravity speed (frames per drop) for the current level.
 *
 * Clamps the level index to SPEED_TABLE_LEN-1 so levels beyond 29 all
 * run at maximum speed (1 frame/drop).
 *
 * @return Frames per automatic piece drop
 */
static u8 getGravitySpeed(void) {
    u16 idx = level;
    if (idx >= SPEED_TABLE_LEN) idx = SPEED_TABLE_LEN - 1;
    return speed_table[idx];
}

/**
 * @brief Add points for cleared lines using NES scoring formula.
 *
 * Points = line_scores[n] * (level + 1). Uses repeated addition instead
 * of multiplication to avoid the expensive __mul16 runtime call on the
 * 65816. Caps score at 65000 to prevent u16 overflow.
 *
 * @param lines_cleared Number of lines cleared simultaneously (1-4)
 */
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

/**
 * @brief Spawn the next piece at the top of the board.
 *
 * Promotes next_type to the current piece, generates a new random
 * next piece, renders it in the preview box, and checks whether the
 * spawn position is blocked (which means game over).
 *
 * @return 1 if spawn position is blocked (game over), 0 if successful
 */
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

/**
 * @brief Process player input with DAS (Delayed Auto Shift) for piece movement.
 *
 * Implements NES-authentic input handling:
 * - A/B: edge-triggered rotation (clockwise / counterclockwise)
 * - LEFT/RIGHT: DAS with 16-frame initial delay, then 6-frame repeat
 * - DOWN: soft drop (3-frame initial delay, then every-frame repeat)
 * - START: toggle pause (blocks input until unpaused)
 *
 * DAS prevents accidental double-moves on button press while allowing
 * fast continuous movement when held. Each direction has its own timer.
 */
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

/**
 * @brief Initialize a new game: clear board, reset state, start music.
 *
 * Resets all game variables to initial state, seeds the RNG from
 * frame_count (which varies based on how long the player waited on
 * the title screen), spawns the first piece, and performs a force-blank
 * flush to upload all three BG layers to VRAM simultaneously.
 */
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

    srand(frame_count);  /* seed RNG from frame count — varies with player timing on title screen */
    next_type = pieceRandom();

    /* Start music */
    snesmodPlay(0);
    snesmodSetModuleVolume(60);

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

/**
 * @brief Main gameplay state: handle input, apply gravity, lock pieces.
 *
 * Uses delta rendering: erases the piece from the tilemap buffer, processes
 * input and gravity, then redraws the piece at its new position. This way
 * only the changed cells need updating rather than the entire board.
 *
 * When a piece locks, checks for completed lines. If found, transitions
 * to STATE_LINE_CLEAR. Otherwise, re-renders the board and spawns next piece.
 */
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

/**
 * @brief Screen shake X offsets during line clear animation.
 *
 * Indexed by (flash_timer & 7). Alternating positive/negative values
 * create a jarring shake effect. Not const -- bank $00 requirement.
 */
static s8 shake_dx[] = { 2, -2, 1, -1, 2, -1, 1, 0 };
/** @brief Screen shake Y offsets during line clear animation */
static s8 shake_dy[] = { -1, 1, -2, 1, 0, -1, 1, 0 };

/**
 * @brief Line clear animation state: flash rows, shake screen, then collapse.
 *
 * Runs for FLASH_FRAMES (10) frames. Each frame applies screen shake
 * via BG scroll offsets and toggles the clearing rows' visual appearance.
 * After the animation completes: removes lines from the board, updates
 * scoring/level, performs a force-blank flush (heavy DMA of BG1+BG2),
 * and spawns the next piece.
 */
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

/**
 * @brief Game over state: display frozen board with rainbow "GAME OVER" text.
 *
 * Renders the final board state with the piece that caused the block-out,
 * shows the GAME OVER message on the BG3 overlay, and cycles through
 * rainbow colors until the player presses START to restart.
 */
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

/**
 * @brief Title screen state: rainbow "PRESS START" text until player begins.
 *
 * Cycles through msg_colors[] at MSG_COLOR_SPEED frames per color,
 * creating a rainbow shimmer on the BG3 overlay text. The frame_count
 * accumulates during this wait, providing RNG seed entropy proportional
 * to how long the player watches the title screen.
 */
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

/**
 * @brief Main entry point -- Tetris initialization and state machine loop.
 *
 * Initialization sequence (all during force blank):
 * 1. Wait 5 frames for NMI joypad reads to stabilize
 * 2. renderInit() sets up Mode 1, loads tiles/palettes, configures all 3 BG layers
 * 3. hudInit() + hudShowMessage() prepare the title screen content
 * 4. renderFlush() DMAs everything to VRAM (guaranteed during force blank)
 * 5. SNESMOD audio initialization (SPC700 module + soundbank)
 * 6. setScreenOn() + renderEnableGradient() begin display with HDMA gradient
 *
 * The main loop dispatches to the current state handler (title, playing,
 * line_clear, game_over) each frame, then calls snesmodProcess() for
 * SPC700 communication and renderFlush() for selective VRAM DMA.
 *
 * @return 0 (never reached -- infinite game loop)
 */
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

    /* Initialize SNESMOD audio */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);
    snesmodLoadModule(MOD_TETRIS_THEME);

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

        snesmodProcess();
        WaitForVBlank();
        renderFlush();
    }

    return 0;
}
