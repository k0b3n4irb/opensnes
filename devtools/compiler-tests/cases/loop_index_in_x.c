/* The counter of a rotated loop lives in X (stage 2, T3, 2026-10-12): a text
 * rule of the emitter's peephole stage. The entry test loads X, the body
 * indexes with it, the step is inx / inx, the bottom test is cpx. */
typedef unsigned short u16;
typedef unsigned char u8;
typedef struct { u16 x, y, vx, vy; } ent_t;
extern u16 words[256];
extern u8 bytes[64];
extern ent_t ents[16];
extern void tick(void);
extern u16 total;

/* the plain fill: ldx / cpx on the way in, inx inx / cpx at the bottom */
void fill(u16 v) {
    u16 k;
    for (k = 0; k < 256; k++)
        words[k] = v;
}

/* a byte array stepping by one: a single inx */
void fill_bytes(u8 v) {
    u16 k;
    for (k = 0; k < 64; k++)
        bytes[k] = v;
}

/* a stride of 8 (a struct): txa / clc / adc / tax */
void move(void) {
    u16 k;
    for (k = 0; k < 16; k++) {
        ents[k].x += ents[k].vx;
        ents[k].y += ents[k].vy;
    }
}

/* a call in the body clobbers X: the counter stays in its slot */
void fill_calls(u16 v) {
    u16 k;
    for (k = 0; k < 256; k++) {
        words[k] = v;
        tick();
    }
}

/* the counter read after the loop: left alone */
u16 count_up(void) {
    u16 k;
    for (k = 0; k < 256; k++)
        words[k] = 0;
    return k;
}
