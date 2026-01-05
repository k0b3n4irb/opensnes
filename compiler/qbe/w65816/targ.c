/*
 * QBE 65816 Backend - Target Definition
 */

#include "all.h"

W65816Op w65816_op[NOp] = {
#define O(op, t, x) [O##op] =
#define V(imm) { imm },
#include "../ops.h"
};

/* Caller-save: R0-R3 */
int w65816_rsave[] = {
    R0, R1, R2, R3,
    -1
};

/* Callee-save: R4-R7 */
int w65816_rclob[] = {
    R4, R5, R6, R7,
    -1
};

#define RGLOB 0  /* No globally reserved registers in our model */

static int
w65816_memargs(int op)
{
    (void)op;
    return 0;
}

Target T_w65816 = {
    .name = "w65816",
    .apple = 0,
    .skiprega = 1,  /* No traditional register allocation */
    .gpr0 = R0,
    .ngpr = NGPR,
    .fpr0 = R0,
    .nfpr = 0,
    .rglob = RGLOB,
    .nrglob = 0,
    .rsave = w65816_rsave,
    .nrsave = {NGPS, NFPS},
    .retregs = w65816_retregs,
    .argregs = w65816_argregs,
    .memargs = w65816_memargs,
    .abi0 = w65816_abi0,
    .abi1 = w65816_abi,
    .isel = w65816_isel,
    .emitfn = w65816_emitfn,
    .emitfin = w65816_emitfin,
    .asloc = "@",
    .assym = "",
};

MAKESURE(rsave_size_ok, sizeof w65816_rsave == (NGPS+1) * sizeof(int));
MAKESURE(rclob_size_ok, sizeof w65816_rclob == (NCLR+1) * sizeof(int));
