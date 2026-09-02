---
name: cc65816/QBE — "lda 8,s pushed twice" for a Kl arg is CORRECT, not a bug (non-bug note)
description: Forwarding a 4-byte (Kl) pointer parameter to another call emits `lda 8,s / pha / lda 8,s / pha`. The two identical-looking reads hit DIFFERENT words because the pha between them shifts SP by 2. Verified correct at runtime. Recorded so nobody re-misdiagnoses it (I did).
type: project
---

## Status: NON-BUG (investigated 2026-08-16, confirmed correct at runtime)

Forwarding a 4-byte far pointer (`Kl` — pointer or function pointer,
post-A6 ABI) that arrived **as a parameter** straight into another call
emits an arg push that *looks* wrong:

```asm
    lda 14,s ; sta 6,s      ; ptr LOW  word -> local 6,s
    lda 16,s ; sta 8,s      ; ptr HIGH word -> local 8,s
    lda 8,s  ; pha           ; push HIGH
    lda 8,s  ; pha           ; push LOW  (NOT a duplicate — see below)
    pea.w 0
    jsl sink
```

The two `lda 8,s` **look** like they read the same word. They do not.
The `pha` between them decrements SP by 2, so the *same* stack-relative
offset `8,s` points 2 bytes lower in the frame the second time:

- Before the first `pha`, SP = X. The pointer's low word is at physical
  `X+6`, high word at `X+8`. `lda 8,s` → `X+8` = **HIGH**. Push it.
- `pha` moves SP to `X-2`.
- `lda 8,s` now → `(X-2)+8` = `X+6` = **LOW**. Push it.

Push order is HIGH then LOW, so the callee reads the low half at the
lower stack offset — exactly what `abi.c` lays out for its params. A
bank-$01 pointer (`$01:9303`) forwarded this way arrives **intact** at
the callee (`lo=03 hi=93 bank=01`), proven at runtime — see the repro.

## Why the codegen is right, in the source

`compiler/qbe/w65816/emit.c`, the `Oarg`/`Kl` non-constant push:

```c
emit_load_high(r0, fn, argbytes);        /* HIGH word, sp_adjust = argbytes   */
fprintf(outf, "\tpha\n");                /* SP -= 2                           */
emitload_adj(r0, fn, argbytes + 2);      /* LOW  word, sp_adjust = argbytes+2 */
fprintf(outf, "\tpha\n");
```

Both reads resolve to the same printed offset because
`high_word_offset + argbytes` == `low_word_offset + (argbytes+2)`
(the high word is +2 within the value; the low read adds +2 to the
sp_adjust to cancel the `pha`). Identical text, different SP — correct.

## The repro (proves it)

[`cc65816_kl_pointer_param_forward_nonbug_repro.c`](cc65816_kl_pointer_param_forward_nonbug_repro.c)
forwards a fn-ptr / data-ptr parameter to a sink and records the 24-bit
value the sink actually received. Built as a ROM and run under luna:
forwarding `$01:9303` yields `lo=03 hi=93 bank=01`. No corruption.

## What actually went wrong in nmiSet (the real bug)

`nmiSet` hung for bank-$01 callbacks because it called
`nmiSetBank(callback, 0)` — **hardcoding bank 0**, discarding the
callback's real bank. The *offset* was always correct (the forward
works). Fixed in v0.36.1 by deriving the bank from the far pointer:
`(u8)((u32)callback >> 16)`. See commit `a40fb20f`. The earlier claim in
this note's first version — that forwarding a `Kl` param miscompiles —
was **wrong**; I read the two `lda 8,s` as a duplicate without
accounting for the `pha`'s SP shift, and did not verify at runtime
before writing it down. Corrected here.

## Lesson

`lda N,s` appearing twice around a `pha`/`pla` is normal — stack-relative
offsets are relative to a *moving* SP. Before calling a repeated `N,s`
read a bug, account for every push/pull between the two, or just verify
at runtime (`luna state --peek`). This note exists because I didn't.

## Cross-references

- `compiler/ABI.md` — cc65816 calling convention; `Kl` = 4-byte slot,
  left-to-right push.
- Commit `a40fb20f` (v0.36.1) — the real nmiSet fix (bank derivation).
- [[nmi_context_hardware_muldiv]] — another subtlety in the NMI path.
