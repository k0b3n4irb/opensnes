---
name: cc65816 Kl (s32/u32) return convention incomplete
description: A function returning s32/u32 (Kl class) only carries the LOW 16 bits across the call boundary. The HIGH 16 bits are computed inside the callee but never transmitted to the caller. Blocks B5 fixed32 phase 2 and any future helper returning a 32-bit value from C code.
type: tech
---

# cc65816 Kl return convention — incomplete (found 2026-05-21)

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

## What needs to happen

Pick a convention for the high half and implement it in the qbe
backend (`Jretl` handler at `emit.c:4197`). Options:

1. **Caller-allocated slot at fixed offset**: the caller's prologue
   reserves N bytes; the callee writes high 16 to a stack offset just
   above its own frame (in the caller's reserved region). The current
   `lda 4,s` pattern in the caller suggests this was the intended
   design — just never implemented on the callee side.

2. **Global "return-high" register**: like `mul32_hi`. Callee writes
   high 16 to a fixed memory location before rtl; caller reads from
   there. Mirrors the existing runtime-helper pattern. Simple but
   not re-entrant (a Kl-returning function calling another Kl-returning
   function would overwrite the global before the outer caller reads
   it).

3. **A:Y or A:X register pair**: callee leaves high 16 in Y (or X);
   caller reads from there immediately after the call. Cleanest from
   a perf standpoint (no memory traffic) but requires the qbe
   backend to model Y/X as live across calls in a new way.

Recommended: option 1, matching what the existing caller-side codegen
already expects. The work is in the `Jretl` handler: before rtl, emit
`lda <high>; sta <caller_offset>,s` where `<caller_offset>` is the
stack slot the caller's prologue allocated.

## Cross-references

- B5 chantier note: [[b5_fix32_orbit_sketch]]
- B5 archived asm draft: `b5_fix32mul_asm_draft.asm` (preserved for
  the eventual phase 2)
- Existing Kl runtime helpers: `lib/source/mul32.asm`, `div32.asm`
- QBE emit.c Jretl handler at line 4197
- QBE emit.c emit_load_high at line 130 (loads high half of a Kl temp
  in non-return contexts — works fine; it's the return-crossing that's
  broken)
