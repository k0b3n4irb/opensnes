/* Test patterns used in animation example:
 * - Loops with array access
 * - Bit operations (&, |, >>)
 * - Pointer dereferencing (register access)
 * - Mixed u8/u16 operations
 * - Conditional updates
 */

/* Simulate hardware registers */
static volatile unsigned char *REG_VMDATAL = (unsigned char*)0x2118;
static volatile unsigned char *REG_VMDATAH = (unsigned char*)0x2119;
static volatile unsigned char *REG_CGADD = (unsigned char*)0x2121;
static volatile unsigned char *REG_CGDATA = (unsigned char*)0x2122;

/* Test data */
static const unsigned char tile_data[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37
};

static const unsigned short palette[] = {
    0x0000, 0x7FFF, 0x001F, 0x03E0, 0x7C00, 0x1234, 0x5678, 0x9ABC
};

/* Game state variables */
static unsigned short player_x;
static unsigned short player_y;
static unsigned char player_state;
static unsigned char player_frame;
static unsigned short pad_held;

/* Test: Loop with byte array access and bit mask */
void load_tiles(void) {
    unsigned short i;
    for (i = 0; i < 32; i++) {
        unsigned char byte = tile_data[i];
        if ((i & 1) == 0) {
            *REG_VMDATAL = byte;
        } else {
            *REG_VMDATAH = byte;
        }
    }
}

/* Test: Loop with word array and byte extraction via shift */
void load_palette(void) {
    unsigned char i;
    for (i = 0; i < 8; i++) {
        unsigned short color = palette[i];
        *REG_CGDATA = color & 0xFF;        /* Low byte */
        *REG_CGDATA = (color >> 8) & 0xFF; /* High byte */
    }
}

/* Test: Conditional updates based on bit flags */
void update_player(void) {
    /* Simulate d-pad input */
    if (pad_held & 0x0100) {  /* Right */
        player_x++;
        player_state = 2;
    }
    if (pad_held & 0x0200) {  /* Left */
        player_x--;
        player_state = 2;
    }
    if (pad_held & 0x0400) {  /* Down */
        player_y++;
        player_state = 0;
    }
    if (pad_held & 0x0800) {  /* Up */
        player_y--;
        player_state = 1;
    }

    /* Animation frame cycling */
    player_frame++;
    if (player_frame >= 24) {
        player_frame = 0;
    }
}

/* Test: Array indexing with computed offset */
static const unsigned char frame_tiles[] = {0, 2, 4, 6, 8, 10};

unsigned char get_tile_for_state(unsigned char state, unsigned char frame) {
    unsigned char base = state * 3;
    unsigned char anim = frame / 8;  /* 0, 1, or 2 */
    return frame_tiles[base + anim];
}

int main(void) {
    /* Initialize */
    player_x = 120;
    player_y = 104;
    player_state = 0;
    player_frame = 0;
    pad_held = 0;

    /* Load graphics */
    load_tiles();
    *REG_CGADD = 128;
    load_palette();

    /* Simulate game loop iterations */
    pad_held = 0x0100;  /* Right */
    update_player();

    pad_held = 0x0400;  /* Down */
    update_player();

    /* Get tile */
    unsigned char tile = get_tile_for_state(player_state, player_frame);

    return tile + player_x + player_y;
}
