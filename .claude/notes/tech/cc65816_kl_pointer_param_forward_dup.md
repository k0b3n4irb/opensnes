---
name: cc65816/QBE — forwarding a 4-byte pointer PARAMETER to another call duplicates the high word (OPEN)
description: QBE mislowers "pass a Kl (4-byte far) pointer received as a function argument straight into another call" — it pushes the pointer's high word twice instead of [low, high], corrupting the callee's offset. Found via nmiSet; worked around by consuming the pointer inline. Live compiler bug.
type: project
---

## Status: OPEN (found 2026-08-16, worked around in nmiSet; QBE not yet fixed)

A function that receives a 4-byte far pointer (`Kl` — pointer or
function pointer, post-A6 ABI) **as a parameter** and forwards that
pointer *by value* into another function call gets miscompiled: QBE
pushes the pointer's **high word twice** instead of `[low, high]`, so
the callee reads a corrupt 24-bit address (high-word bytes where the
low-word/offset should be).

This is distinct from the far-pointer bugs already on file:
- [[ternary_addr_const_bank_drop]] — phi/ternary of an address constant
  dropped the bank byte (FIXED, shipped v0.36.0).
- [[cc65816_kl_return_incomplete]] / [[cc65816_kl_shift_high_half]] —
  other `Kl` value-tracking gaps.

Here the pointer is neither returned, dereferenced, nor computed — it is
a *pass-through argument*, and the forwarding push is what breaks.

## The concrete case (confirmed)

`lib/source/console.c` before the fix:

```c
void nmiSet(VBlankCallback callback) {   /* VBlankCallback = void(*)(void), 4-byte far */
    nmiSetBank(callback, 0);             /* forward the 4-byte ptr param + a bank arg */
}
```

Generated (`lib/build/lorom/console.asm`), annotated:

```asm
nmiSet:
    ...
    lda 14,s        ; callback LOW  word (incoming arg)
    sta 6,s         ;   -> local 6,s
    lda 16,s        ; callback HIGH word
    sta 8,s         ;   -> local 8,s
@body:
    lda 8,s         ; push callback HIGH word
    pha
    lda 8,s         ; push callback HIGH word AGAIN   <-- BUG: should be `lda 6,s` (LOW)
    pha
    pea.w 0         ; bank = 0
    jsl nmiSetBank
```

`nmiSetBank(VBlankCallback callback, u8 bank)` reads the 4-byte
`callback` as low@`30,s`, high@`32,s` (after its own `sbc #24`) and
`bank`@`28,s`. Because the caller pushed `[high, high, 0]`, `nmiSetBank`
takes the **high word as the offset** and stores a wholly wrong pointer
into `nmi_callback[0..1]`. (The bank byte was *also* wrong here, but for
an unrelated reason — `nmiSet` hardcoded `0`; see the release fix. The
codegen bug is the duplicated word.)

## Minimal repro (CONFIRMED 2026-08-16)

Reduced to a standalone golden —
[`cc65816_kl_pointer_param_forward_dup_repro.c`](cc65816_kl_pointer_param_forward_dup_repro.c):

```c
typedef void (*fp)(void);
extern void sink(fp cb, unsigned char bank);   /* 4-byte ptr + 1 byte, another TU */
void forward(fp cb) { sink(cb, 0); }            /* pass-through a fn-ptr PARAM */

extern void dsink(char *p, unsigned char b);
void dforward(char *p) { dsink(p, 0); }         /* pass-through a data-ptr PARAM */
```

`bin/cc65816 fwd.c -o fwd.asm` → **both** `forward` and `dforward` emit:

```asm
    lda 14,s ; sta 6,s      ; ptr LOW  word -> local 6,s
    lda 16,s ; sta 8,s      ; ptr HIGH word -> local 8,s
    lda 8,s  ; pha           ; push HIGH
    lda 8,s  ; pha           ; push HIGH AGAIN   <-- BUG (should be `lda 6,s`)
    pea.w 0
    jsl sink / dsink
```

So it is **not** function-pointer-specific — it is any 4-byte pointer
(`Kl`) argument passed straight through to another call. The signature
is the two arg pushes reading the **same** stack slot instead of
`N,s` then `N+2,s`. Local/computed pointers are not affected; the value
must arrive *as a parameter* and be forwarded by value.

## Workaround (what nmiSet does now)

**Don't forward a 4-byte pointer parameter into another call — consume it
inline.** `nmiSet` was rewritten to write `nmi_callback[0..3]` itself
(offset from `(u16)callback`, bank from `(u8)((u32)callback >> 16)`)
instead of delegating to `nmiSetBank`. No inter-function forward of the
`Kl` param → no duplicated push. See commit `a40fb20f` (v0.36.1).

Equivalent escapes if you hit this elsewhere:
- Inline the callee (macro or same-TU `static` that QBE folds), so there
  is no cross-call argument to lower.
- Split the pointer at the call site: pass `(u16)p` and `(u8)(p>>16)` as
  separate scalar args and reassemble in the callee (this is essentially
  what `nmiSetBank`'s explicit-`bank` signature already allows — the
  offset-only `(u16)callback` path is fine; it is the *4-byte* forward
  that breaks).

## Why it stayed hidden

`nmiSet` is the only public API that forwards a caller-supplied function
pointer, and **no example calls it with a real callback** — the whole
corpus registers no VBlank callback. `projects/rpg` is the first real
user; it originally shipped a hand-rolled bank-$00 ASM trampoline to
dodge the (separate) bank bug, which also masked this one. The defect
will resurface for any lib/user function that takes a pointer/`u32`
argument and passes it straight through to another call.

## Diagnosis route if it resurfaces

1. `grep` the caller's `.asm` for the two arg pushes; a correct 4-byte
   forward reads two *different* stack slots (`lda 6,s` / `lda 8,s`), the
   bug reads the same one twice.
2. Check the callee actually received the right pointer: at runtime
   `luna state --peek <dest>` (e.g. `nmi_callback:4`) — a corrupt entry
   with the high byte duplicated into the low position is the signature.
3. Apply the inline-consume workaround; file/patch QBE's `Ocall`
   argument lowering for `Kl`-by-value forwards.

## Escalation

Worth a `.claude/STRUCTURAL_DEFECTS.md` entry (a QBE call-lowering
chantier) if a second site is found or a fix is scheduled — the class is
"pointer/`u32` argument pass-through", which is broader than the single
`nmiSet` occurrence. Until then this note is the record.

## Cross-references

- `compiler/ABI.md` — cc65816 calling convention; `Kl` = 4-byte slot.
- [[nmi_context_hardware_muldiv]] — the other subtlety in the same NMI
  callback path.
- Commit `a40fb20f` (develop / v0.36.1) — the nmiSet fix + workaround.
