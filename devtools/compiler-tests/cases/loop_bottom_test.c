/* A counted loop tests its condition at the bottom of each turn (no jmp back
 * to the condition). Since ivar (2026-10-12) its counter is the byte offset
 * of the array element, so no shift scales it. */
unsigned short nw[240];

void clear(void)
{
    unsigned short k;

    for (k = 0; k < 240; k++)
        nw[k] = 0;
}

unsigned short sum(void)
{
    unsigned short k, s = 0;

    for (k = 0; k < 240; k++)
        s += nw[k];
    return s;
}
