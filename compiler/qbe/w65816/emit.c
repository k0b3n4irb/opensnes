/*
 * QBE 65816 Backend - Code Emission
 *
 * Maps QBE's virtual registers R0-R7 to direct page $00-$0F.
 * Maps SSA temps to stack slots starting at $10.
 * Uses A as scratch for all ALU operations.
 *
 * For local variables (alloc), we track which temps hold stack addresses
 * and use direct stack-relative addressing for loads/stores through them.
 *
 * Alloc tracking is done in abi0 (before promote() removes alloc instructions)
 * and the results are stored in w65816_alloc_size[] and w65816_alloc_slots.
 */

#include "all.h"

/* Strip .L prefix from symbol names for WLA-DX compatibility.
 * WLA-DX doesn't support labels starting with '.'
 * e.g., ".Lstring.1" becomes "string.1"
 */
static char *
stripsym(char *name)
{
    if (name[0] == '.' && name[1] == 'L')
        return name + 2;
    return name;
}

static FILE *outf;
static int framesize;  /* Current function's frame size */
static int argbytes;   /* Bytes of arguments pushed for current call */

/* Alloc slot offset tracking - computed from w65816_alloc_size */
static int allocslot[MAX_ALLOC_TEMPS];  /* stack offset for each alloc temp */

/*
 * Get the stack slot offset if this temp is from alloc, -1 otherwise.
 * The offset is into the alloc region of the frame.
 */
static int
getallocslot(Ref r, Fn *fn)
{
    (void)fn;
    if (rtype(r) != RTmp)
        return -1;
    int idx = r.val - Tmp0;
    if (idx >= 0 && idx < MAX_ALLOC_TEMPS && w65816_alloc_size[idx] > 0)
        return allocslot[idx];
    return -1;
}

/* Direct page address for register */
static int
regaddr(int r)
{
    if (r >= R0 && r <= R7)
        return (r - R0) * 2;
    return 0;
}

/* Check if ref is a virtual register (R0-R7) */
static int
isvreg(Ref r)
{
    return rtype(r) == RTmp && r.val >= R0 && r.val <= R7;
}

/*
 * Assign a slot to a temp if not already assigned
 */
static int
maybeassign(Ref r, Fn *fn, int slot)
{
    if (rtype(r) == RTmp && r.val >= Tmp0) {
        if (fn->tmp[r.val].slot < 0) {
            fn->tmp[r.val].slot = slot++;
        }
    }
    return slot;
}

/*
 * Safe phi coalescing: Only coalesce if the phi argument is:
 * 1. Not used elsewhere in the block that defines it (would lose value)
 * 2. Not itself a phi result (avoid circular dependencies)
 *
 * For safety, we now only coalesce constants and single-use temps that
 * don't conflict with other phis in the same block.
 *
 * Actually, the safest approach is to NOT coalesce at all and rely on
 * explicit phi moves (emitphimoves). This is less efficient but correct.
 */
static void
coalescephi(Phi *p, Fn *fn)
{
    /* Just ensure the phi result has a slot */
    if (rtype(p->to) != RTmp || p->to.val < Tmp0)
        return;

    if (fn->tmp[p->to.val].slot < 0) {
        fn->tmp[p->to.val].slot = fn->slot++;
    }

    /* Don't coalesce phi arguments - let emitphimoves handle copies */
}

/*
 * Assign stack slots to all unassigned temps.
 * Called before emission when skiprega is set.
 * Must handle ALL refs (destinations and operands) since
 * optimization passes may create new temps.
 *
 * IMPORTANT: Process phi nodes first and coalesce their arguments
 * so that phi argument writes update the phi result directly.
 */
static int
assignslots(Fn *fn)
{
    Blk *b;
    Ins *i;
    Phi *p;
    int n;
    int maxslot = fn->slot;

    /* First pass: coalesce phi arguments with their results */
    for (b = fn->start; b; b = b->link) {
        for (p = b->phi; p; p = p->link) {
            coalescephi(p, fn);
        }
    }

    /* Update maxslot after phi coalescing */
    maxslot = fn->slot;

    /* Second pass: assign remaining slots */
    for (b = fn->start; b; b = b->link) {
        /* Process phi nodes - result already handled, check args */
        for (p = b->phi; p; p = p->link) {
            maxslot = maybeassign(p->to, fn, maxslot);
            for (n = 0; n < p->narg; n++)
                maxslot = maybeassign(p->arg[n], fn, maxslot);
        }
        /* Process instructions */
        for (i = b->ins; i < &b->ins[b->nins]; i++) {
            maxslot = maybeassign(i->to, fn, maxslot);
            maxslot = maybeassign(i->arg[0], fn, maxslot);
            maxslot = maybeassign(i->arg[1], fn, maxslot);
        }
        /* Process jump arguments */
        maxslot = maybeassign(b->jmp.arg, fn, maxslot);
    }
    return maxslot;
}

/*
 * Load value into accumulator
 * sp_adjust: additional offset to add for stack-relative loads
 *            (used during argument pushing when SP has already changed)
 */
static void
emitload_adj(Ref r, Fn *fn, int sp_adjust)
{
    Con *c;
    int slot;

    switch (rtype(r)) {
    case RTmp:
        if (r.val >= R0 && r.val <= R7) {
            /* Virtual register - direct page */
            fprintf(outf, "\tlda.b $%02X\n", regaddr(r.val));
        } else if (r.val >= Tmp0) {
            /* Spilled temp */
            slot = fn->tmp[r.val].slot;
            if (slot >= 0)
                fprintf(outf, "\tlda %d,s\n", (slot + 1) * 2 + sp_adjust);
            else
                fprintf(outf, "\t; unallocated temp %d\n", r.val);
        } else {
            fprintf(outf, "\t; unknown temp %d\n", r.val);
        }
        break;
    case RCon:
        c = &fn->con[r.val];
        if (c->type == CBits) {
            fprintf(outf, "\tlda.w #%d\n", (int)(c->bits.i & 0xFFFF));
        } else if (c->type == CAddr) {
            fprintf(outf, "\tlda.w #%s", stripsym(str(c->sym.id)));
            if (c->bits.i)
                fprintf(outf, "+%d", (int)c->bits.i);
            fprintf(outf, "\n");
        }
        break;
    case RSlot:
        slot = rsval(r);
        if (slot < 0) {
            /* Negative slot = parameter from caller's frame */
            /* Stack layout after prologue (framesize F):
             *   S+1 to S+F: local frame (F bytes)
             *   S+(F+1): P (from PHP, 1 byte)
             *   S+(F+2): PCL (1 byte) }
             *   S+(F+3): PCH (1 byte) } JSL return (3 bytes)
             *   S+(F+4): PBR (1 byte) }
             *   S+(F+5): first param low byte
             *   S+(F+6): first param high byte
             * slot=-2 means first param, slot=-4 means second, etc.
             * Offset = framesize + 5 + (-slot) - 2 = framesize + 3 + (-slot)
             */
            fprintf(outf, "\tlda %d,s\n", framesize + 3 + (-slot) + sp_adjust);
        } else {
            /* Positive slot = local variable in our frame */
            fprintf(outf, "\tlda %d,s\n", (slot + 1) * 2 + sp_adjust);
        }
        break;
    default:
        fprintf(outf, "\t; unknown ref type %d\n", rtype(r));
        break;
    }
}

/* Convenience wrapper for normal loads (no SP adjustment) */
static void
emitload(Ref r, Fn *fn)
{
    emitload_adj(r, fn, 0);
}

/*
 * Store accumulator
 */
static void
emitstore(Ref r, Fn *fn)
{
    int slot;

    if (req(r, R))
        return;

    switch (rtype(r)) {
    case RTmp:
        if (r.val >= R0 && r.val <= R7) {
            fprintf(outf, "\tsta.b $%02X\n", regaddr(r.val));
        } else if (r.val >= Tmp0) {
            slot = fn->tmp[r.val].slot;
            if (slot >= 0)
                fprintf(outf, "\tsta %d,s\n", (slot + 1) * 2);
        }
        break;
    case RSlot:
        slot = rsval(r);
        if (slot < 0)
            fprintf(outf, "\tsta %d,s\n", framesize + 3 + (-slot));
        else
            fprintf(outf, "\tsta %d,s\n", (slot + 1) * 2);
        break;
    default:
        break;
    }
}

/*
 * Emit ALU second operand
 */
static void
emitop2(char *op, Ref r, Fn *fn)
{
    Con *c;
    int slot;

    switch (rtype(r)) {
    case RTmp:
        if (r.val >= R0 && r.val <= R7) {
            fprintf(outf, "\t%s.b $%02X\n", op, regaddr(r.val));
        } else if (r.val >= Tmp0) {
            slot = fn->tmp[r.val].slot;
            if (slot >= 0)
                fprintf(outf, "\t%s %d,s\n", op, (slot + 1) * 2);
        }
        break;
    case RCon:
        c = &fn->con[r.val];
        if (c->type == CBits) {
            fprintf(outf, "\t%s.w #%d\n", op, (int)(c->bits.i & 0xFFFF));
        } else if (c->type == CAddr) {
            fprintf(outf, "\t%s.w #%s", op, stripsym(str(c->sym.id)));
            if (c->bits.i)
                fprintf(outf, "+%d", (int)c->bits.i);
            fprintf(outf, "\n");
        }
        break;
    case RSlot:
        slot = rsval(r);
        if (slot < 0)
            fprintf(outf, "\t%s %d,s\n", op, framesize + 3 + (-slot));
        else
            fprintf(outf, "\t%s %d,s\n", op, (slot + 1) * 2);
        break;
    default:
        break;
    }
}

/*
 * Emit instruction
 */
static void
emitins(Ins *i, Fn *fn)
{
    Ref r0, r1;
    Con *c;

    r0 = i->arg[0];
    r1 = i->arg[1];

    switch (i->op) {
    case Oadd:
        emitload(r0, fn);
        fprintf(outf, "\tclc\n");
        emitop2("adc", r1, fn);
        emitstore(i->to, fn);
        break;

    case Osub:
        emitload(r0, fn);
        fprintf(outf, "\tsec\n");
        emitop2("sbc", r1, fn);
        emitstore(i->to, fn);
        break;

    case Oneg:
        /* Negate: result = -value = ~value + 1 (two's complement) */
        emitload(r0, fn);
        fprintf(outf, "\teor.w #$FFFF\n");
        fprintf(outf, "\tinc a\n");
        emitstore(i->to, fn);
        break;

    case Omul:
        /* 65816 has no MUL instruction - need to use a loop or library call */
        /* For now, emit a simple shift-add loop for small constants */
        if (rtype(r1) == RCon) {
            c = &fn->con[r1.val];
            int val = c->bits.i;
            if (val == 0) {
                fprintf(outf, "\tlda.w #0\n");
            } else if (val == 1) {
                emitload(r0, fn);
            } else if (val == 2) {
                emitload(r0, fn);
                fprintf(outf, "\tasl a\n");
            } else if (val == 4) {
                emitload(r0, fn);
                fprintf(outf, "\tasl a\n");
                fprintf(outf, "\tasl a\n");
            } else if (val == 8) {
                emitload(r0, fn);
                fprintf(outf, "\tasl a\n");
                fprintf(outf, "\tasl a\n");
                fprintf(outf, "\tasl a\n");
            } else if (val == 16) {
                emitload(r0, fn);
                fprintf(outf, "\tasl a\n");
                fprintf(outf, "\tasl a\n");
                fprintf(outf, "\tasl a\n");
                fprintf(outf, "\tasl a\n");
            } else if (val == 32) {
                emitload(r0, fn);
                fprintf(outf, "\txba\n");  /* swap bytes = *256 */
                fprintf(outf, "\tlsr a\n");
                fprintf(outf, "\tlsr a\n");
                fprintf(outf, "\tlsr a\n");  /* /8 = *32 */
            } else {
                /* General case: use stack for multiplier, call __mul */
                emitload(r1, fn);
                fprintf(outf, "\tpha\n");
                emitload_adj(r0, fn, 2);  /* Adjust for pushed value */
                fprintf(outf, "\tpha\n");
                fprintf(outf, "\tjsl __mul16\n");
                fprintf(outf, "\ttax\n");  /* Save result in X */
                fprintf(outf, "\ttsa\n");
                fprintf(outf, "\tclc\n");
                fprintf(outf, "\tadc.w #4\n");
                fprintf(outf, "\ttas\n");
                fprintf(outf, "\ttxa\n");  /* Restore result to A */
            }
        } else {
            /* Variable * variable - call __mul16 */
            emitload(r1, fn);
            fprintf(outf, "\tpha\n");
            emitload_adj(r0, fn, 2);  /* Adjust for pushed value */
            fprintf(outf, "\tpha\n");
            fprintf(outf, "\tjsl __mul16\n");
            fprintf(outf, "\ttax\n");  /* Save result in X */
            fprintf(outf, "\ttsa\n");
            fprintf(outf, "\tclc\n");
            fprintf(outf, "\tadc.w #4\n");
            fprintf(outf, "\ttas\n");
            fprintf(outf, "\ttxa\n");  /* Restore result to A */
        }
        emitstore(i->to, fn);
        break;

    case Odiv:
    case Oudiv:
        /* Division - use shifts for powers of 2, library call otherwise */
        if (rtype(r1) == RCon) {
            c = &fn->con[r1.val];
            int val = c->bits.i;
            if (val == 1) {
                emitload(r0, fn);
            } else if (val == 2) {
                emitload(r0, fn);
                fprintf(outf, "\tlsr a\n");
            } else if (val == 4) {
                emitload(r0, fn);
                fprintf(outf, "\tlsr a\n");
                fprintf(outf, "\tlsr a\n");
            } else if (val == 8) {
                emitload(r0, fn);
                fprintf(outf, "\tlsr a\n");
                fprintf(outf, "\tlsr a\n");
                fprintf(outf, "\tlsr a\n");
            } else if (val == 16) {
                emitload(r0, fn);
                fprintf(outf, "\tlsr a\n");
                fprintf(outf, "\tlsr a\n");
                fprintf(outf, "\tlsr a\n");
                fprintf(outf, "\tlsr a\n");
            } else if (val == 256) {
                emitload(r0, fn);
                fprintf(outf, "\txba\n");  /* swap bytes = /256 */
                fprintf(outf, "\tand.w #$00FF\n");
            } else {
                /* General case: call __div16 */
                emitload(r0, fn);
                fprintf(outf, "\tsta.l tcc__r0\n");
                emitload(r1, fn);
                fprintf(outf, "\tsta.l tcc__r1\n");
                fprintf(outf, "\tjsl __div16\n");
                fprintf(outf, "\tlda.l tcc__r0\n");
            }
        } else {
            /* Variable / variable - call __div16 */
            emitload(r0, fn);
            fprintf(outf, "\tsta.l tcc__r0\n");
            emitload(r1, fn);
            fprintf(outf, "\tsta.l tcc__r1\n");
            fprintf(outf, "\tjsl __div16\n");
            fprintf(outf, "\tlda.l tcc__r0\n");
        }
        emitstore(i->to, fn);
        break;

    case Orem:
    case Ourem:
        /* Modulo - use AND for powers of 2, library call otherwise */
        if (rtype(r1) == RCon) {
            c = &fn->con[r1.val];
            int val = c->bits.i;
            if (val == 2) {
                emitload(r0, fn);
                fprintf(outf, "\tand.w #1\n");
            } else if (val == 4) {
                emitload(r0, fn);
                fprintf(outf, "\tand.w #3\n");
            } else if (val == 8) {
                emitload(r0, fn);
                fprintf(outf, "\tand.w #7\n");
            } else if (val == 16) {
                emitload(r0, fn);
                fprintf(outf, "\tand.w #15\n");
            } else if (val == 256) {
                emitload(r0, fn);
                fprintf(outf, "\tand.w #255\n");
            } else {
                /* General case: call __mod16 */
                emitload(r0, fn);
                fprintf(outf, "\tsta.l tcc__r0\n");
                emitload(r1, fn);
                fprintf(outf, "\tsta.l tcc__r1\n");
                fprintf(outf, "\tjsl __mod16\n");
                fprintf(outf, "\tlda.l tcc__r0\n");
            }
        } else {
            /* Variable % variable - call __mod16 */
            emitload(r0, fn);
            fprintf(outf, "\tsta.l tcc__r0\n");
            emitload(r1, fn);
            fprintf(outf, "\tsta.l tcc__r1\n");
            fprintf(outf, "\tjsl __mod16\n");
            fprintf(outf, "\tlda.l tcc__r0\n");
        }
        emitstore(i->to, fn);
        break;

    case Oand:
        emitload(r0, fn);
        emitop2("and", r1, fn);
        emitstore(i->to, fn);
        break;

    case Oor:
        emitload(r0, fn);
        emitop2("ora", r1, fn);
        emitstore(i->to, fn);
        break;

    case Oxor:
        emitload(r0, fn);
        emitop2("eor", r1, fn);
        emitstore(i->to, fn);
        break;

    case Oshl:
        emitload(r0, fn);
        if (rtype(r1) == RCon) {
            c = &fn->con[r1.val];
            for (int j = 0; j < c->bits.i && j < 16; j++)
                fprintf(outf, "\tasl a\n");
        }
        emitstore(i->to, fn);
        break;

    case Osar:
        /* Arithmetic shift right - for 16-bit values, using LSR is safe
         * since high bits are already zero. For negative values, this
         * doesn't properly sign-extend, but SNES code rarely needs that. */
        emitload(r0, fn);
        if (rtype(r1) == RCon) {
            c = &fn->con[r1.val];
            for (int j = 0; j < c->bits.i && j < 16; j++)
                fprintf(outf, "\tlsr a\n");
        }
        emitstore(i->to, fn);
        break;

    case Oshr:
        emitload(r0, fn);
        if (rtype(r1) == RCon) {
            c = &fn->con[r1.val];
            for (int j = 0; j < c->bits.i && j < 16; j++)
                fprintf(outf, "\tlsr a\n");
        }
        emitstore(i->to, fn);
        break;

    case Ocopy:
        if (!req(i->to, r0)) {
            emitload(r0, fn);
            emitstore(i->to, fn);
        }
        break;

    /* Comparison operations - produce 0 or 1 */
    case Oceqw:
    case Oceql:
        emitload(r0, fn);
        emitop2("cmp", r1, fn);
        fprintf(outf, "\tbeq +\n");
        fprintf(outf, "\tlda.w #0\n");
        fprintf(outf, "\tbra ++\n");
        fprintf(outf, "+\tlda.w #1\n");
        fprintf(outf, "++\n");
        emitstore(i->to, fn);
        break;

    case Ocnew:
    case Ocnel:
        emitload(r0, fn);
        emitop2("cmp", r1, fn);
        fprintf(outf, "\tbne +\n");
        fprintf(outf, "\tlda.w #0\n");
        fprintf(outf, "\tbra ++\n");
        fprintf(outf, "+\tlda.w #1\n");
        fprintf(outf, "++\n");
        emitstore(i->to, fn);
        break;

    case Ocsltw:
    case Ocsltl:
        /* Signed less than */
        emitload(r0, fn);
        emitop2("cmp", r1, fn);
        fprintf(outf, "\tbmi +\n");
        fprintf(outf, "\tlda.w #0\n");
        fprintf(outf, "\tbra ++\n");
        fprintf(outf, "+\tlda.w #1\n");
        fprintf(outf, "++\n");
        emitstore(i->to, fn);
        break;

    case Ocsgtw:
    case Ocsgtl:
        /* Signed greater than: swap operands and use less than */
        emitload(r1, fn);
        emitop2("cmp", r0, fn);
        fprintf(outf, "\tbmi +\n");
        fprintf(outf, "\tlda.w #0\n");
        fprintf(outf, "\tbra ++\n");
        fprintf(outf, "+\tlda.w #1\n");
        fprintf(outf, "++\n");
        emitstore(i->to, fn);
        break;

    case Ocslew:
    case Ocslel:
        /* Signed less or equal: !(a > b) */
        emitload(r1, fn);
        emitop2("cmp", r0, fn);
        fprintf(outf, "\tbmi +\n");
        fprintf(outf, "\tlda.w #1\n");
        fprintf(outf, "\tbra ++\n");
        fprintf(outf, "+\tlda.w #0\n");
        fprintf(outf, "++\n");
        emitstore(i->to, fn);
        break;

    case Ocsgew:
    case Ocsgel:
        /* Signed greater or equal: !(a < b) */
        emitload(r0, fn);
        emitop2("cmp", r1, fn);
        fprintf(outf, "\tbmi +\n");
        fprintf(outf, "\tlda.w #1\n");
        fprintf(outf, "\tbra ++\n");
        fprintf(outf, "+\tlda.w #0\n");
        fprintf(outf, "++\n");
        emitstore(i->to, fn);
        break;

    case Ocultw:
    case Ocultl:
        /* Unsigned less than */
        emitload(r0, fn);
        emitop2("cmp", r1, fn);
        fprintf(outf, "\tbcc +\n");
        fprintf(outf, "\tlda.w #0\n");
        fprintf(outf, "\tbra ++\n");
        fprintf(outf, "+\tlda.w #1\n");
        fprintf(outf, "++\n");
        emitstore(i->to, fn);
        break;

    case Ocugtw:
    case Ocugtl:
        /* Unsigned greater than */
        emitload(r1, fn);
        emitop2("cmp", r0, fn);
        fprintf(outf, "\tbcc +\n");
        fprintf(outf, "\tlda.w #0\n");
        fprintf(outf, "\tbra ++\n");
        fprintf(outf, "+\tlda.w #1\n");
        fprintf(outf, "++\n");
        emitstore(i->to, fn);
        break;

    case Oculew:
    case Oculel:
        /* Unsigned less or equal */
        emitload(r1, fn);
        emitop2("cmp", r0, fn);
        fprintf(outf, "\tbcc +\n");
        fprintf(outf, "\tlda.w #1\n");
        fprintf(outf, "\tbra ++\n");
        fprintf(outf, "+\tlda.w #0\n");
        fprintf(outf, "++\n");
        emitstore(i->to, fn);
        break;

    case Ocugew:
    case Ocugel:
        /* Unsigned greater or equal */
        emitload(r0, fn);
        emitop2("cmp", r1, fn);
        fprintf(outf, "\tbcc +\n");
        fprintf(outf, "\tlda.w #1\n");
        fprintf(outf, "\tbra ++\n");
        fprintf(outf, "+\tlda.w #0\n");
        fprintf(outf, "++\n");
        emitstore(i->to, fn);
        break;

    case Ostorel:
        /* Store long (32-bit) - store low word then high word at +2 */
        /* For 65816, we treat this as storing a 32-bit value in two parts */
        emitload(r0, fn);  /* Load low 16 bits */
        {
            int aslot = getallocslot(r1, fn);
            if (aslot >= 0) {
                /* Store to stack-allocated local variable */
                fprintf(outf, "\tsta %d,s\n", (aslot + 1) * 2);
                /* High word is typically 0 for near pointers */
                fprintf(outf, "\tlda.w #0\n");
                fprintf(outf, "\tsta %d,s\n", (aslot + 1) * 2 + 2);
            } else if (isvreg(r1)) {
                fprintf(outf, "\tsta ($%02X)\n", regaddr(r1.val));
                fprintf(outf, "\tlda.w #0\n");
                fprintf(outf, "\tldy.w #2\n");
                fprintf(outf, "\tsta ($%02X),y\n", regaddr(r1.val));
            } else if (rtype(r1) == RCon) {
                /* Direct address constant or symbol */
                c = &fn->con[r1.val];
                if (c->type == CAddr) {
                    fprintf(outf, "\tsta.l %s", stripsym(str(c->sym.id)));
                    if (c->bits.i)
                        fprintf(outf, "+%d", (int)c->bits.i);
                    fprintf(outf, "\n");
                    /* Store high word */
                    fprintf(outf, "\tlda.w #0\n");
                    fprintf(outf, "\tsta.l %s", stripsym(str(c->sym.id)));
                    fprintf(outf, "+%d\n", (int)c->bits.i + 2);
                } else {
                    fprintf(outf, "\tsta.l $%06lX\n", (unsigned long)c->bits.i);
                    fprintf(outf, "\tlda.w #0\n");
                    fprintf(outf, "\tsta.l $%06lX\n", (unsigned long)c->bits.i + 2);
                }
            } else {
                /* Address in temp - indirect store */
                fprintf(outf, "\tpha\n");
                emitload_adj(r1, fn, 2);
                fprintf(outf, "\ttax\n");
                fprintf(outf, "\tpla\n");
                fprintf(outf, "\tsta.l $0000,x\n");
                /* Store high word at +2 */
                fprintf(outf, "\tlda.w #0\n");
                fprintf(outf, "\tsta.l $0002,x\n");
            }
        }
        break;

    case Ostorew:
    case Ostoreh:
        emitload(r0, fn);
        {
            int aslot = getallocslot(r1, fn);
            if (aslot >= 0) {
                /* Store to stack-allocated local variable */
                fprintf(outf, "\tsta %d,s\n", (aslot + 1) * 2);
            } else if (isvreg(r1)) {
                fprintf(outf, "\tsta ($%02X)\n", regaddr(r1.val));
            } else if (rtype(r1) == RCon) {
                /* Direct address constant or symbol */
                c = &fn->con[r1.val];
                if (c->type == CAddr) {
                    /* Symbol address - emit symbol name */
                    fprintf(outf, "\tsta.l %s", stripsym(str(c->sym.id)));
                    if (c->bits.i)
                        fprintf(outf, "+%d", (int)c->bits.i);
                    fprintf(outf, "\n");
                } else {
                    /* Literal address */
                    fprintf(outf, "\tsta.l $%06lX\n", (unsigned long)c->bits.i);
                }
            } else {
                /* Address in temp - indirect store */
                /* IMPORTANT: After pha, stack offsets change by 2 */
                fprintf(outf, "\tpha\n");
                emitload_adj(r1, fn, 2);  /* Adjust for pushed value */
                fprintf(outf, "\ttax\n");
                fprintf(outf, "\tpla\n");
                fprintf(outf, "\tsta.l $0000,x\n");
            }
        }
        break;

    case Ostoreb:
        /* Load value in 16-bit mode BEFORE switching to 8-bit */
        emitload(r0, fn);
        fprintf(outf, "\tsep #$20\n");  /* Switch to 8-bit for store */
        {
            int aslot = getallocslot(r1, fn);
            if (aslot >= 0) {
                /* Store to stack-allocated local variable */
                fprintf(outf, "\tsta %d,s\n", (aslot + 1) * 2);
            } else if (isvreg(r1)) {
                fprintf(outf, "\tsta ($%02X)\n", regaddr(r1.val));
            } else if (rtype(r1) == RCon) {
                /* Direct address constant or symbol */
                c = &fn->con[r1.val];
                if (c->type == CAddr) {
                    /* Symbol address - emit symbol name */
                    fprintf(outf, "\tsta.l %s", stripsym(str(c->sym.id)));
                    if (c->bits.i)
                        fprintf(outf, "+%d", (int)c->bits.i);
                    fprintf(outf, "\n");
                } else {
                    /* Literal address */
                    fprintf(outf, "\tsta.l $%06lX\n", (unsigned long)c->bits.i);
                }
            } else {
                /* Address in temp - indirect store */
                /* A already has the value, load addr to X, then store */
                /* IMPORTANT: After pha, stack offsets change by 2 */
                fprintf(outf, "\trep #$20\n");  /* Need 16-bit for address */
                fprintf(outf, "\tpha\n");       /* Save value */
                emitload_adj(r1, fn, 2);        /* Adjust for pushed value */
                fprintf(outf, "\ttax\n");
                fprintf(outf, "\tpla\n");
                fprintf(outf, "\tsep #$20\n");  /* Back to 8-bit for store */
                fprintf(outf, "\tsta.l $0000,x\n");
            }
        }
        fprintf(outf, "\trep #$20\n");
        break;

    case Oloadsw:
    case Oloaduw:
    case Oload:
        {
            int aslot = getallocslot(r0, fn);
            if (aslot >= 0) {
                /* Load from stack-allocated local variable */
                fprintf(outf, "\tlda %d,s\n", (aslot + 1) * 2);
            } else if (rtype(r0) == RSlot) {
                /* Direct stack access (e.g., loading parameter) */
                emitload(r0, fn);
            } else if (isvreg(r0)) {
                /* Indirect through register */
                fprintf(outf, "\tlda ($%02X)\n", regaddr(r0.val));
            } else if (rtype(r0) == RCon && fn->con[r0.val].type == CAddr) {
                /* Direct load from global/extern symbol */
                Con *c = &fn->con[r0.val];
                fprintf(outf, "\tlda.l %s", stripsym(str(c->sym.id)));
                if (c->bits.i)
                    fprintf(outf, "+%d", (int)c->bits.i);
                fprintf(outf, "\n");
            } else {
                /* Address in temp - indirect load */
                emitload(r0, fn);
                fprintf(outf, "\ttax\n");
                fprintf(outf, "\tlda.l $0000,x\n");
            }
        }
        emitstore(i->to, fn);
        break;

    case Oloadsb:
    case Oloadub:
        /* Load byte from memory, zero/sign extend to 16-bit */
        if (isvreg(r0)) {
            /* Pointer in virtual register - use indirect */
            fprintf(outf, "\tsep #$20\n");
            fprintf(outf, "\tlda ($%02X)\n", regaddr(r0.val));
            fprintf(outf, "\trep #$20\n");
        } else if (rtype(r0) == RCon && fn->con[r0.val].type == CAddr) {
            /* Direct load from global/extern symbol */
            Con *c = &fn->con[r0.val];
            fprintf(outf, "\tsep #$20\n");
            fprintf(outf, "\tlda.l %s", stripsym(str(c->sym.id)));
            if (c->bits.i)
                fprintf(outf, "+%d", (int)c->bits.i);
            fprintf(outf, "\n");
            fprintf(outf, "\trep #$20\n");
        } else {
            /* Pointer in stack slot - load addr, then indirect through X */
            emitload(r0, fn);  /* Load pointer value to A */
            fprintf(outf, "\ttax\n");  /* Transfer to X */
            fprintf(outf, "\tsep #$20\n");
            fprintf(outf, "\tlda.l $0000,x\n");  /* Load byte from memory */
            fprintf(outf, "\trep #$20\n");
        }
        fprintf(outf, "\tand.w #$00FF\n");  /* Zero extend */
        if (i->op == Oloadsb) {
            /* Sign extend */
            fprintf(outf, "\tcmp.w #$0080\n");
            fprintf(outf, "\tbcc +\n");
            fprintf(outf, "\tora.w #$FF00\n");
            fprintf(outf, "+\n");
        }
        emitstore(i->to, fn);
        break;

    case Oextsb:
        /* Sign extend byte to word */
        emitload(r0, fn);
        fprintf(outf, "\tand.w #$00FF\n");
        fprintf(outf, "\tcmp.w #$0080\n");
        fprintf(outf, "\tbcc +\n");
        fprintf(outf, "\tora.w #$FF00\n");
        fprintf(outf, "+\n");
        emitstore(i->to, fn);
        break;

    case Oextub:
        /* Zero extend byte to word */
        emitload(r0, fn);
        fprintf(outf, "\tand.w #$00FF\n");
        emitstore(i->to, fn);
        break;

    case Oextsh:
        /* Sign extend half (already 16-bit, no-op for 65816) */
        emitload(r0, fn);
        emitstore(i->to, fn);
        break;

    case Oextuh:
        /* Zero extend half (already 16-bit, no-op for 65816) */
        emitload(r0, fn);
        emitstore(i->to, fn);
        break;

    case Oextsw:
        /* Sign extend word to long - for 16-bit 65816, just copy the value.
         * If code actually uses the upper 16 bits of a signed long,
         * this would need proper sign extension to 32 bits. */
        emitload(r0, fn);
        emitstore(i->to, fn);
        break;

    case Oextuw:
        /* Zero extend word to long - for 16-bit 65816, just copy the value.
         * Upper 16 bits are implicitly zero in our 16-bit register model. */
        emitload(r0, fn);
        emitstore(i->to, fn);
        break;

    case Oarg:
    case Oargsb:
    case Oargub:
    case Oargsh:
    case Oarguh:
        /* Push argument to stack
         * IMPORTANT: Use emitload_adj with argbytes offset because
         * previous argument pushes have already modified SP, making
         * stack-relative offsets wrong if not adjusted.
         */
        emitload_adj(r0, fn, argbytes);
        fprintf(outf, "\tpha\n");
        argbytes += 2;  /* All args pushed as 16-bit */
        break;

    case Ocall:
        c = &fn->con[r0.val];
        fprintf(outf, "\tjsl %s\n", stripsym(str(c->sym.id)));
        {
            int cleanup = argbytes;
            argbytes = 0;
            /* Return value is in A - save it before stack cleanup */
            if (!req(i->to, R) && cleanup > 0) {
                /* Save return value to X temporarily */
                fprintf(outf, "\ttax\n");
            }
            /* Clean up pushed arguments */
            if (cleanup > 0) {
                fprintf(outf, "\ttsa\n");
                fprintf(outf, "\tclc\n");
                fprintf(outf, "\tadc.w #%d\n", cleanup);
                fprintf(outf, "\ttas\n");
            }
            /* Restore return value from X and store it */
            if (!req(i->to, R)) {
                if (cleanup > 0) {
                    fprintf(outf, "\ttxa\n");
                }
                emitstore(i->to, fn);
            }
        }
        break;

    case Oalloc4:
    case Oalloc8:
    case Oalloc16:
        /* alloc returns a pointer to stack space.
         * The slot was already assigned in the pre-pass.
         * We just emit a no-op here since loads/stores through
         * this temp use direct stack-relative addressing.
         */
        {
            int aslot = getallocslot(i->to, fn);
            if (aslot >= 0) {
                /* Store slot offset to temp (for debugging/consistency) */
                fprintf(outf, "\tlda.w #%d\n", (aslot + 1) * 2);
                emitstore(i->to, fn);
            }
        }
        break;

    default:
        fprintf(outf, "\t; unhandled op %d\n", i->op);
        break;
    }
}

/*
 * Emit phi moves for a jump from 'from' to 'to'.
 * For each phi in 'to', find the argument from 'from' and:
 * - If constant: store constant to phi result's slot
 * - If temp with different slot: copy to phi result's slot
 *   (With coalescing, temps should already share slots)
 */
static void
emitphimoves(Blk *from, Blk *to, Fn *fn)
{
    Phi *p;
    int n;
    int dstslot;

    if (!to)
        return;

    for (p = to->phi; p; p = p->link) {
        /* Find the argument corresponding to 'from' */
        for (n = 0; n < p->narg; n++) {
            if (p->blk[n] == from) {
                /* Get destination slot (phi result) */
                if (rtype(p->to) != RTmp || p->to.val < Tmp0)
                    continue;
                dstslot = fn->tmp[p->to.val].slot;
                if (dstslot < 0)
                    continue;

                /* Handle the argument */
                if (rtype(p->arg[n]) == RCon) {
                    /* Constant: load and store to phi slot */
                    emitload(p->arg[n], fn);
                    fprintf(outf, "\tsta %d,s\n", (dstslot + 1) * 2);
                } else if (rtype(p->arg[n]) == RTmp && p->arg[n].val >= Tmp0) {
                    /* Temp: check if we need to copy */
                    int srcslot = fn->tmp[p->arg[n].val].slot;
                    if (srcslot >= 0 && srcslot != dstslot) {
                        /* Different slots - need to copy */
                        fprintf(outf, "\tlda %d,s\n", (srcslot + 1) * 2);
                        fprintf(outf, "\tsta %d,s\n", (dstslot + 1) * 2);
                    }
                    /* If same slot, no copy needed (coalesced) */
                }
                break;
            }
        }
    }
}

/*
 * Emit jump/return
 */
static void
emitjmp(Blk *b, Fn *fn)
{
    switch (b->jmp.type) {
    case Jret0:
    case Jretw:
    case Jretl:
        if (!req(b->jmp.arg, R))
            emitload(b->jmp.arg, fn);
        break;
    case Jjmp:
        emitphimoves(b, b->s1, fn);
        fprintf(outf, "\tjmp @%s\n", b->s1->name);
        break;
    case Jjnz:
        emitload(b->jmp.arg, fn);
        /* For conditional: emit phi moves on each branch */
        fprintf(outf, "\tbne +\n");
        /* False branch (fall through to s2) */
        emitphimoves(b, b->s2, fn);
        fprintf(outf, "\tjmp @%s\n", b->s2->name);
        /* True branch */
        fprintf(outf, "+\n");
        emitphimoves(b, b->s1, fn);
        fprintf(outf, "\tjmp @%s\n", b->s1->name);
        break;
    default:
        if (b->jmp.type >= Jjf && b->jmp.type <= Jjf1) {
            emitload(b->jmp.arg, fn);
            fprintf(outf, "\tbne +\n");
            emitphimoves(b, b->s2, fn);
            fprintf(outf, "\tjmp @%s\n", b->s2->name);
            fprintf(outf, "+\n");
            emitphimoves(b, b->s1, fn);
            fprintf(outf, "\tjmp @%s\n", b->s1->name);
        }
        break;
    }
}

void
w65816_emitfn(Fn *fn, FILE *f)
{
    Blk *b;
    Ins *i;
    int slot;

    outf = f;

    /*
     * Compute allocslot[] offsets from w65816_alloc_size[] (set by abi0)
     * Alloc temps get stack slots first (lowest offsets from SP)
     */
    slot = 0;
    for (int j = 0; j < MAX_ALLOC_TEMPS; j++) {
        if (w65816_alloc_size[j] > 0) {
            allocslot[j] = slot;
            slot += w65816_alloc_size[j];
        } else {
            allocslot[j] = -1;
        }
    }

    /* Reserve slots for allocs in fn->slot so assignslots starts after them */
    fn->slot = slot;

    /* Assign slots to any unassigned temps (when skiprega is set) */
    fn->slot = assignslots(fn);

    /* Total frame size: alloc slots + temp slots + 1 for alignment */
    framesize = (fn->slot + 1) * 2;
    argbytes = 0;  /* Reset argument tracking for this function */

    /* Debug: show slot assignments */
    fprintf(outf, "\n; Function: %s (framesize=%d, slots=%d, alloc_slots=%d)\n",
            fn->name, framesize, fn->slot, w65816_alloc_slots);
    for (int t = Tmp0; t < fn->ntmp; t++) {
        int aslot = (t - Tmp0 >= 0 && t - Tmp0 < MAX_ALLOC_TEMPS) ? allocslot[t - Tmp0] : -1;
        fprintf(outf, "; temp %d: slot=%d, alloc=%d\n", t, fn->tmp[t].slot, aslot);
    }
    fprintf(outf, ".SECTION \".text.%s\" SUPERFREE\n", fn->name);
    fprintf(outf, "%s:\n", fn->name);

    /* Prologue */
    fprintf(outf, "\tphp\n");
    fprintf(outf, "\trep #$30\n");
    if (framesize > 2) {
        fprintf(outf, "\ttsa\n");
        fprintf(outf, "\tsec\n");
        fprintf(outf, "\tsbc.w #%d\n", framesize);
        fprintf(outf, "\ttas\n");
    }

    for (b = fn->start; b; b = b->link) {
        fprintf(outf, "@%s:\n", b->name);

        for (i = b->ins; i < &b->ins[b->nins]; i++)
            emitins(i, fn);

        if (isret(b->jmp.type)) {
            emitjmp(b, fn);
            if (framesize > 2) {
                fprintf(outf, "\ttsa\n");
                fprintf(outf, "\tclc\n");
                fprintf(outf, "\tadc.w #%d\n", framesize);
                fprintf(outf, "\ttas\n");
            }
            fprintf(outf, "\tplp\n");
            fprintf(outf, "\trtl\n");
        } else {
            emitjmp(b, fn);
        }
    }

    fprintf(outf, ".ENDS\n");
}

void
w65816_emitfin(FILE *f)
{
    fprintf(f, "\n; End of generated code\n");
}
