/* A counted loop tests its condition at the bottom of each turn (no jmp back
 * to the condition), and its body does not reload the index the compare has
 * just left in A. */
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
