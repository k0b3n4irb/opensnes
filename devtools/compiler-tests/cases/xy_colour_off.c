/* QBE_NO_XY_COLOUR=1 turns the X/Y colour off: the counter of glide is the
 * loop rule's, in Y, copied into X before each access (xy_colour.c). */
typedef unsigned short u16;
typedef unsigned char u8;

u16 px[48], ppx[48], out[48];
u8 tab[256];
u16 row;

void glide(void)
{
    u16 i, d;
    for (i = 0; i < 48; i++) {
        d = px[i] - ppx[i] + 48;
        out[i] = d <= 96 ? ppx[i] + tab[row + d] : px[i];
    }
}
