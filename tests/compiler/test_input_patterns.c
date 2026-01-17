/**
 * Test: Input button patterns match SNES hardware layout
 *
 * SNES Joypad Register Layout (16-bit read from $4218):
 *   Bits 15-12: B, Y, Select, Start (from $4219 bits 7-4)
 *   Bits 11-8:  Up, Down, Left, Right (from $4219 bits 3-0)
 *   Bits 7-4:   A, X, L, R (from $4218 bits 7-4)
 *   Bits 3-0:   Controller ID (from $4218 bits 3-0, should be 0)
 *
 * Button constants must match pvsneslib for compatibility.
 */

/* Button constants - must match pvsneslib values */
#define KEY_B       (1 << 15)  /* 0x8000 */
#define KEY_Y       (1 << 14)  /* 0x4000 */
#define KEY_SELECT  (1 << 13)  /* 0x2000 */
#define KEY_START   (1 << 12)  /* 0x1000 */
#define KEY_UP      (1 << 11)  /* 0x0800 */
#define KEY_DOWN    (1 << 10)  /* 0x0400 */
#define KEY_LEFT    (1 << 9)   /* 0x0200 */
#define KEY_RIGHT   (1 << 8)   /* 0x0100 */
#define KEY_A       (1 << 7)   /* 0x0080 */
#define KEY_X       (1 << 6)   /* 0x0040 */
#define KEY_L       (1 << 5)   /* 0x0020 */
#define KEY_R       (1 << 4)   /* 0x0010 */

/* Simulated pad value */
extern unsigned short pad_value;

/* Movement variables */
unsigned short player_x;
unsigned short player_y;
unsigned char player_state;
unsigned char player_flipx;

void handle_input(void) {
    /* D-pad handling - should use correct masks */
    if (pad_value & KEY_UP) {
        player_state = 1;
        player_flipx = 0;
        if (player_y > 0) {
            player_y = player_y - 1;
        }
    }

    if (pad_value & KEY_LEFT) {
        player_state = 2;
        player_flipx = 1;
        if (player_x > 0) {
            player_x = player_x - 1;
        }
    }

    if (pad_value & KEY_RIGHT) {
        player_state = 2;
        player_flipx = 0;
        if (player_x < 255) {
            player_x = player_x + 1;
        }
    }

    if (pad_value & KEY_DOWN) {
        player_state = 0;
        player_flipx = 0;
        if (player_y < 223) {
            player_y = player_y + 1;
        }
    }
}

unsigned char check_button_a(void) {
    return (pad_value & KEY_A) ? 1 : 0;
}

unsigned char check_button_b(void) {
    return (pad_value & KEY_B) ? 1 : 0;
}
