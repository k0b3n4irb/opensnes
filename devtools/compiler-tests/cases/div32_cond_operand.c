/* A condition used as the 32-bit dividend: the compiler stopped with an
 * internal error ("has no stack slot") until 2026-10-11. The 32-bit divide
 * reads its first operand from the stack after pushing the second. */
short g0;
long d;

long rem_of_cond(void)
{
    return (!g0 ? 1L : 0L) % d;
}

unsigned long div_of_cond(void)
{
    return (!g0 ? 1UL : 0UL) / (unsigned long)d;
}
