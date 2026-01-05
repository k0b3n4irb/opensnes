/*
 * QBE Backend for WDC 65816 (SNES)
 *
 * Strategy: Define 8 "virtual registers" that map to direct page.
 * QBE will allocate to these, and emit.c generates code using
 * A as scratch and direct page for storage.
 */

#include "../all.h"

typedef struct W65816Op W65816Op;

/*
 * Register model: 8 virtual registers mapped to direct page
 */
enum W65816Reg {
    /* Virtual registers (direct page $00-$0F) */
    R0 = RXX + 1,   /* $00-$01 - also return value */
    R1,             /* $02-$03 */
    R2,             /* $04-$05 */
    R3,             /* $06-$07 */
    R4,             /* $08-$09 */
    R5,             /* $0A-$0B */
    R6,             /* $0C-$0D */
    R7,             /* $0E-$0F */

    /* Counts */
    NGPR = 8,       /* 8 virtual registers */
    NFPR = 0,       /* No floating point */
    NGPS = 4,       /* Caller-save: R0-R3 */
    NFPS = 0,
    NCLR = 4,       /* Callee-save: R4-R7 */
};

MAKESURE(reg_not_tmp, R7 < (int)Tmp0);

/*
 * Direct page address for a register
 */
#define DP_ADDR(r) (((r) - R0) * 2)

struct W65816Op {
    char imm;
};

/* targ.c */
extern int w65816_rsave[];
extern int w65816_rclob[];
extern W65816Op w65816_op[];

/* abi.c */
bits w65816_retregs(Ref, int[2]);
bits w65816_argregs(Ref, int[2]);
void w65816_abi0(Fn *);
void w65816_abi(Fn *);

/* Alloc tracking - set by abi0, used by emit */
#define MAX_ALLOC_TEMPS 256
extern int w65816_alloc_size[MAX_ALLOC_TEMPS];  /* size in words, 0 if not alloc */
extern int w65816_alloc_slots;                   /* total slots reserved for allocs */

/* isel.c */
void w65816_isel(Fn *);

/* emit.c */
void w65816_emitfn(Fn *, FILE *);
void w65816_emitfin(FILE *);
