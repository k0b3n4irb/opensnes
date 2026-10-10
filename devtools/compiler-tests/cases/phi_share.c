/* A loop variable and its next value share a slot: `i++` is done in
 * place and the back edge copies nothing. Same for a value that one arm
 * of an `if` changes: the merge is the variable's own slot (2026-10-10,
 * issue #166; emit.c, color_slots). */
typedef unsigned short u16;
typedef short s16;
extern u16 tab[32];

u16 count_up(u16 n) {
    u16 i, acc = 0;
    for (i = 0; i < n; i++)
        acc += tab[i];
    return acc;
}

/* one arm changes the accumulator, the other leaves it: the merge is the
 * accumulator's own slot, and the arm that changes nothing copies nothing */
u16 cond_add(u16 n) {
    u16 i, acc = 0;
    for (i = 0; i < n; i++)
        if (tab[i] & 1) acc += 3;
    return acc;
}

/* the old value is still read after the new one exists: no sharing */
u16 keeps_old(u16 n) {
    u16 j = n, sum = 0;
    while (j--) sum += tab[j & 31] ^ j;
    return sum;
}
