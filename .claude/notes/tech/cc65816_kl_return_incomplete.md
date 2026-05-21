---
name: cc65816 Kl (s32/u32) return convention — RESOLVED
description: Historical record of a bug found 2026-05-21 and fixed the same day. A function returning s32/u32 (Kl class) only carried the LOW 16 bits across the call boundary; the fix added a global `tcc__retval_hi` for the high half (qbe 3e79c8c, opensnes faaf803). Preserved for context — future contributors hitting Kl return issues should start here.
type: tech
---

# cc65816 Kl return convention — RESOLVED 2026-05-21

**Status as of 2026-05-21**: FIXED. qbe commit `3e79c8c` on
`fix/a6-a7-leaf-opt-kl-frameless` adds the missing convention via a
`tcc__retval_hi` global (declared in `templates/crt0.asm`). Both
sides of every Kl-returning call now propagate the full 32 bits.

This note is preserved because:
1. The bug-class (incomplete return convention) is the kind of thing
   that could recur in a future backend rewrite or new target.
2. The diagnostic pattern (compile a tiny `long bar(long, long)` and
   inspect the asm) is worth knowing.
3. The history of why `tcc__retval_hi` exists is useful when someone
   reads `templates/crt0.asm` and wonders about it.

---

# Historical context (found 2026-05-21, fixed same day)

Any C function returning `s32`/`u32`/`long`/`unsigned long`/`fixed32`
silently drops the **high 16 bits** of its return value. The low 16
bits arrive in A as expected. Discovered while attempting B5 phase 2
(fix32Mul); blocks any further work that needs Kl returns.

## Reproduction

```c
long bar(long a, long b) { return a + b; }
long g_result;
void test(void) {
    g_result = bar(0x11112222L, 0x33334444L);
}
```

`cc65816` emits for `bar`:

```asm
bar:
    rep #$20
    tsa
    sec
    sbc.w #22         ; allocate 22-byte frame
    tas
    ; arg copy ...
@body.4:
    lda 10,s          ; a_lo
    clc
    adc 26,s          ; + b_lo
    sta 18,s          ; sum_lo
    lda 12,s          ; a_hi
    adc 16,s          ; + b_hi (with carry)
    sta 20,s          ; sum_hi  ← CORRECT but never returned
    lda 18,s          ; reload sum_lo
    tax
    tsa
    clc
    adc.w #22         ; deallocate frame
    tas
    txa               ; A = sum_lo  ← only LOW 16 returned
    rtl
```

The 32-bit addition is **correctly computed** (sum_hi at 20,s) — the
arithmetic is fine. The epilogue is where it breaks: only A (= low 16)
crosses back. There is no `sta` of the high half to any caller-visible
location.

The caller (`test`) emits:

```asm
jsl bar
tax
tsa
clc
adc.w #8
tas
txa               ; A = low 16 (passed through TAX/TXA across SP teardown)
sta 6,s           ; spill low 16 to local
sta.w g_result    ; store low 16 to global
lda 8,s           ; ← reads high 16 from local slot 8,s
sta.w g_result+2  ; store high 16 to global
```

The `lda 8,s` reads from a stack slot in `test`'s own frame that
**nothing ever wrote to**. The high half of `g_result` ends up as
whatever stack garbage was at `8,s` from a previous call or
initialisation.

## Visible impact

- `g_result` low 16 = correct (0x5566 for the test inputs)
- `g_result` high 16 = unspecified — usually whatever was on the stack

A non-zero high half by accident can look like the function works. A
test that checks BOTH halves catches it; ours did (during B5
implementation).

## Why this hasn't surfaced earlier

The lib avoids Kl returns by design. The only Kl arithmetic in
existing code is:
- `tcc_mul32` / `tcc_udivmod32` / `tcc_sdivmod32` — these are runtime
  helpers called via JSL with SPECIAL handling in the qbe backend
  (the backend emits `jsl tcc_mul32` THEN `lda.l mul32_hi` to fetch
  the high half from a global). They're not standard C functions.
- Inline arithmetic on Kl temps within a function — works fine because
  the result stays in stack slots / globals, no return crossing.

Every standard C function in `lib/source/*.c` returns void, `u8`,
`u16`, or pointer (which is 4 bytes but uses a different convention
because pointers are special — see the post-A6 pointer ABI).

So the bug class only surfaces when someone tries to write the first
real Kl-returning C function. That was 2026-05-21 with `fix32Mul`.

## What was needed → resolution chosen

Original options considered:

1. **Caller-allocated slot at fixed offset**: the caller's prologue
   reserves N bytes; the callee writes high 16 to a stack offset just
   above its own frame. The current `lda 4,s` pattern in the caller
   suggested this was the intended design — but coordinating the
   offset between callee and caller would require teaching the callee
   about the caller's frame layout. Rejected.

2. **Global "return-high" register**: like `mul32_hi`. Callee writes
   high 16 to a fixed memory location before rtl; caller reads from
   there. Mirrors the existing runtime-helper pattern.

3. **A:Y or A:X register pair**: cleanest perf but requires the qbe
   backend to model Y/X as live across calls in a new way — bigger
   surgery in the register allocator. Rejected for scope.

**Chosen: option 2** — landed in qbe `3e79c8c` (2026-05-21):

  ```cpp
  /* Callee (Jretl in build_block): */
  if (b->jmp.type == Jretl) {
      emit_load_high(b->jmp.arg, fn, 0);
      fprintf(outf, "\tsta.b tcc__retval_hi\n");
  }
  emitload(b->jmp.arg, fn);

  /* Caller (Ocall post-call): */
  if (i->cls == Kl) {
      fprintf(outf, "\tlda.b tcc__retval_hi\n");
      emit_store_high(i->to, fn);
  }
  ```

`tcc__retval_hi` is a 2-byte global declared in
`templates/crt0.asm`'s `.registers` RAMSECTION, alongside the
existing `tcc__r0..r10h` scratch registers. At `$00:0xxx` for
fast direct-page access.

**Re-entrancy note**: a Kl-returning function calling another
Kl-returning function WILL overwrite `tcc__retval_hi` before the
outer caller reads it — but at the point the outer callee writes
its OWN final high before `rtl`, that overwrites whatever the
inner call left behind. Since the outer's caller reads
immediately after the outer's `rtl`, the value is correct. So
the pattern works for arbitrary nesting; only concurrent reads
within a single expression (impossible — sequential statements)
would be a problem.

## Cross-references

- B5 chantier note: [[b5_fix32_orbit_sketch]]
- B5 archived asm draft: `b5_fix32mul_asm_draft.asm` (preserved for
  the eventual phase 2)
- Existing Kl runtime helpers: `lib/source/mul32.asm`, `div32.asm`
- QBE emit.c Jretl handler at line 4197
- QBE emit.c emit_load_high at line 130 (loads high half of a Kl temp
  in non-return contexts — works fine; it's the return-crossing that's
  broken)
