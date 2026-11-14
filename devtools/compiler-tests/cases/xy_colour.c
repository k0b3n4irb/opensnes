/* The X/Y colour (lot XY, 2026-11-14, color_slots / xy_emit in emit.c): a
 * counter or an index kept in X or Y for its whole life, proved on the
 * emitted text. Checks: xy_colour.checks; the twin xy_colour_off pins the
 * text with QBE_NO_XY_COLOUR=1. */
typedef unsigned short u16;
typedef unsigned char u8;

u16 px[48], ppx[48], out[48];
u8 tab[256];
u16 row, hits;

/* a counter in Y indexes the word arrays as `sym,y`: no `tyx` before each
 * access; the table read through `tax` keeps X for itself */
void glide(void)
{
    u16 i, d;
    for (i = 0; i < 48; i++) {
        d = px[i] - ppx[i] + 48;
        out[i] = d <= 96 ? ppx[i] + tab[row + d] : px[i];
    }
}

/* a counter in X: `inx`, `cpx`, the index read in place */
void fill(u16 v)
{
    u16 i;
    for (i = 0; i < 48; i++)
        out[i] = v;
}

/* the inner counter is the loop rule's (an operand of `ora`): the colour
 * leaves it its register, the rare `hits++` stays in memory */
void nest(void)
{
    u16 y, x;
    for (y = 0; y < 16; y++)
        for (x = 0; x < 32; x++)
            if (((y << 5) | x) == 100)
                hits++;
}
