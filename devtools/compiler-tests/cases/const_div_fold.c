/* A divide or remainder of two constants is folded by gvn although QBE pins
 * it (2026-11-07): `19391 / 3584` called __sdiv16 while `7 * 32` folded.
 * The numerator of the first division is constant only once the multiply,
 * add and subtract before it are folded. INT_MIN / -1 and a zero divisor are
 * refused by the folder and stay calls. Values: testing/fixtures/compiler/
 * const_div. */
unsigned int u16out[3];
long s32out;
int s16out;

void folded(void)
{
    u16out[0] = (7 * 32 + 3584 - 1) / 3584;
    u16out[1] = 19391 / 3584;
    u16out[2] = 19391u % 3584u;
    s32out = -100000L % 7L;
}

void kept(void)
{
    s16out = (int)((-32767 - 1) / -1);
    u16out[0] = 1u / 0u;
}
