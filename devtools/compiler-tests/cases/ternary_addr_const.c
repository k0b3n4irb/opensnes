/*
 * Regression: a phi / conditional expression that yields an ADDRESS CONSTANT
 * (a string literal, or any symbol address) must materialise the far pointer's
 * BANK byte in each predecessor — not only the low 16 bits.
 *
 * qbe's emitphimoves used to store only the offset word (`lda.w #sym ; sta
 * N,s`) for a Kl phi arg, leaving the bank half as stack garbage; the consumer
 * (call arg / return / store) then read a corrupt far pointer. Fixed in
 * w65816/emit.c. See .claude/notes/tech/ternary_addr_const_bank_drop.md and the
 * ph2 runtime cell in runtime/a6_farptr.
 */
extern void sink(const char *s);

void pick(int x) {
    sink(x ? "AAAA" : "BBBB");
}
