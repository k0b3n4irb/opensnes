/*
 * QBE 65816 Backend - ABI / Calling Convention
 *
 * Stack-based calling convention:
 *   - All arguments passed on stack, left-to-right
 *   - Return value in accumulator (A)
 *   - Caller cleans up stack after call
 *
 * This is simpler for the 65816 which has limited registers.
 */

#include "all.h"

/*
 * Alloc tracking - shared between abi0 and emit
 *
 * We scan for alloc instructions in abi0 (before promote() removes them)
 * and record which temps are alloc results. This info is used during emit
 * for proper stack-relative addressing of local variables.
 *
 * w65816_alloc_size[idx] = size in words (2 bytes) for temp (Tmp0 + idx)
 * w65816_alloc_slots = total slots reserved for allocs
 */
int w65816_alloc_size[MAX_ALLOC_TEMPS];
int w65816_alloc_slots;

/*
 * Scan for alloc instructions and record them
 * Called from abi0, BEFORE promote() runs
 */
static void
scanallocations(Fn *fn)
{
    Blk *b;
    Ins *i;
    int totalslots = 0;

    /* Reset alloc tracking */
    for (int j = 0; j < MAX_ALLOC_TEMPS; j++)
        w65816_alloc_size[j] = 0;

    for (b = fn->start; b; b = b->link) {
        for (i = b->ins; i < &b->ins[b->nins]; i++) {
            if (i->op == Oalloc4 || i->op == Oalloc8 || i->op == Oalloc16) {
                /* Get size in bytes from the argument */
                int bytes = 2;  /* default minimum */
                if (rtype(i->arg[0]) == RCon) {
                    Con *c = &fn->con[i->arg[0].val];
                    if (c->type == CBits)
                        bytes = (int)c->bits.i;
                }

                /* Round up to words (2 bytes per word) */
                int words = (bytes + 1) / 2;
                if (words < 1) words = 1;

                /* Record this temp as an alloc result */
                if (rtype(i->to) == RTmp && i->to.val >= Tmp0) {
                    int idx = i->to.val - Tmp0;
                    if (idx >= 0 && idx < MAX_ALLOC_TEMPS) {
                        w65816_alloc_size[idx] = words;
                        totalslots += words;
                    }
                }
            }
        }
    }

    w65816_alloc_slots = totalslots;

    if (debug['A']) {
        fprintf(stderr, "> Alloc scan for %s: %d total slots\n",
                fn->name, totalslots);
        for (int j = 0; j < MAX_ALLOC_TEMPS; j++) {
            if (w65816_alloc_size[j] > 0) {
                fprintf(stderr, "  temp %d: %d words\n",
                        j + Tmp0, w65816_alloc_size[j]);
            }
        }
    }
}

/*
 * ABI phase 0 - runs BEFORE promote()
 *
 * 1. Scan for alloc instructions (before they get optimized away)
 * 2. Call elimsb to handle sign-extension elimination
 */
void
w65816_abi0(Fn *fn)
{
    /* Scan allocs first - this MUST happen before promote() */
    scanallocations(fn);

    /* Then do standard sign-byte elimination */
    elimsb(fn);
}

/*
 * Return value registers - just accumulator (encoded as 1 GPR)
 */
bits
w65816_retregs(Ref r, int p[2])
{
    (void)r;
    if (p) {
        p[0] = 1;  /* 1 GPR (conceptually) for return */
        p[1] = 0;  /* No FP regs */
    }
    return 0;  /* No actual register bits - A is implicit */
}

/*
 * Argument registers - none, all on stack
 */
bits
w65816_argregs(Ref r, int p[2])
{
    (void)r;
    if (p) {
        p[0] = 0;
        p[1] = 0;
    }
    return 0;
}

/*
 * Count parameters in a block
 */
static int
countpars(Blk *b)
{
    Ins *i;
    int n = 0;
    for (i = b->ins; i < &b->ins[b->nins]; i++)
        if (ispar(i->op))
            n++;
    return n;
}

/*
 * ABI lowering pass
 *
 * For w65816 with skiprega, we do minimal transformation:
 * - Parameters (Opar) become stack loads
 * - Arguments (Oarg) become stack stores
 * - Calls remain as-is, emit will handle the JSL
 * - Return values are in A
 *
 * Stack layout (after callee setup):
 *   [local frame]
 *   [PHP - 1 byte]
 *   [return addr - 3 bytes]
 *   [last arg pushed]      <- lower offset from SP
 *   ...
 *   [first arg pushed]     <- higher offset from SP
 */
void
w65816_abi(Fn *fn)
{
    Blk *b;
    Ins *i;
    int paroff;  /* parameter offset from return address */
    int npars;   /* number of parameters */
    int parn;    /* current parameter number (0 = first) */

    for (b = fn->start; b; b = b->link) {
        curi = &insb[NIns];
        npars = countpars(b);
        parn = 0;

        for (i = &b->ins[b->nins]; i > b->ins;) {
            i--;

            switch (i->op) {
            case Opar:
            case Oparsb:
            case Oparub:
            case Oparsh:
            case Oparuh:
                /* Parameter: load from stack */
                /* Encode byte offset above return address as negative slot */
                /* Iterating backwards: first encountered = last declared param */
                /* Last param is at lowest offset (closest to ret addr) */
                /* slot = -(parn + 1) * 2 encodes byte offset from ret addr */
                paroff = (parn + 1) * 2;  /* 2, 4, 6, ... */
                emit(Oloadsw, Kw, i->to, SLOT(-paroff), R);
                parn++;
                break;

            case Oarg:
            case Oargsb:
            case Oargub:
            case Oargsh:
            case Oarguh:
                /* Argument: push to stack (handled in call sequence) */
                /* emit will push with pha */
                emiti(*i);
                break;

            case Ocall:
                /* Call: pass through, emit handles JSL */
                emiti(*i);
                break;

            default:
                emiti(*i);
                break;
            }
        }

        b->nins = &insb[NIns] - curi;
        idup(b, curi, b->nins);
    }

    if (debug['A']) {
        fprintf(stderr, "\n> After ABI lowering:\n");
        printfn(fn, stderr);
    }
}
