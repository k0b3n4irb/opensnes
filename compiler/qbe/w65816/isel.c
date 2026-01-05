/*
 * QBE 65816 Backend - Instruction Selection
 *
 * Translates QBE IR operations to 65816 instruction patterns.
 * The 65816 has limited addressing modes:
 *   - Immediate: LDA #$1234
 *   - Direct page: LDA $12
 *   - Absolute: LDA $1234
 *   - Stack relative: LDA 5,S
 *   - Indirect: LDA ($12), LDA [$12]
 *
 * Most operations use the accumulator as scratch.
 */

#include "all.h"

/*
 * Check if a reference is a small immediate
 * (fits in 16 bits for word operations)
 */
static int
isimm(Ref r, Fn *fn, int64_t *val)
{
    Con *c;

    if (rtype(r) != RCon)
        return 0;
    c = &fn->con[r.val];
    if (c->type != CBits)
        return 0;
    if (val)
        *val = c->bits.i;
    return c->bits.i >= -32768 && c->bits.i <= 65535;
}

/*
 * Select instructions for a basic block
 */
static void
sel(Ins *i, Fn *fn)
{
    (void)fn;
    (void)isimm;

    /* For now, just pass through all instructions.
     * The emit pass will handle the actual code generation.
     * More sophisticated instruction selection can be added later
     * (e.g., combining operations, strength reduction).
     */
    switch (i->op) {
    case Oadd:
    case Osub:
    case Omul:
    case Odiv:
    case Oudiv:
    case Orem:
    case Ourem:
    case Oand:
    case Oor:
    case Oxor:
    case Osar:
    case Oshr:
    case Oshl:
    case Oceqw:
    case Ocnew:
    case Ocsgew:
    case Ocsgtw:
    case Ocslew:
    case Ocsltw:
    case Ocugew:
    case Ocugtw:
    case Oculew:
    case Ocultw:
    case Ostorew:
    case Ostoreh:
    case Ostoreb:
    case Ostorel:
    case Ostores:
    case Ostored:
    case Oloadsw:
    case Oloaduw:
    case Oload:
    case Oloadsb:
    case Oloadub:
    case Oloadsh:
    case Oloaduh:
    case Ocopy:
    case Ocall:
    case Oalloc4:
    case Oalloc8:
    case Oalloc16:
    default:
        emiti(*i);
        break;
    }
}

/*
 * Instruction selection for a function
 */
void
w65816_isel(Fn *fn)
{
    Blk *b;
    Ins *i;

    for (b = fn->start; b; b = b->link) {
        curi = &insb[NIns];

        for (i = &b->ins[b->nins]; i > b->ins;) {
            i--;
            sel(i, fn);
        }

        b->nins = &insb[NIns] - curi;
        idup(b, curi, b->nins);
    }

    if (debug['I']) {
        fprintf(stderr, "\n> After instruction selection:\n");
        printfn(fn, stderr);
    }
}
