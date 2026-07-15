# NMI-context hardware mul/div corruption (#113) — root cause & fix

## Symptom chain

Found during the Mode 7 port (#110): `hdmaSetup` calls from an `nmiSet`
callback "did nothing". The minimal repro proved hdmaSetup itself was
innocent — **`u16 * u16` inside the callback returned 0** (so the HDMA
offset was always 0 and the writes were real but constant).

## Root cause (two stacked hazards)

`tcc_mul16` / `tcc_div16` (the compiler runtime behind C's `*`, `/`,
`%`) use the **hardware mul/div unit** ($4202-$4217):

1. **Auto-joypad window**: the unit shares silicon with the auto-joypad
   shift logic; while HVBJOY bit 0 is set (V≈225-227, ~4K master
   clocks into VBlank) reads return garbage. The user NMI callback
   (step 4) runs EXACTLY in that window — the NMI handler's own joypad
   read (step 5) waits for the bit, but the callback runs before that
   wait. Observed garbage: 0x2A/0x00 patterns from the shift register.
2. **Non-reentrancy**: NMI is non-maskable. A main-thread multiply
   interrupted between its `sta $4203` and `lda $4216` gets its result
   destroyed by any callback multiply. No wait can fix this one —
   the callback must not touch the unit at all.

## Fix (shipped with #113)

- crt0: `in_nmi_ctx` flag ($00 sysvar) set/cleared around the user
  callback invocation only.
- `tcc_mul16`: tests the flag (long addressing — DBR is $7E in
  callbacks) → `@soft_mul` shift-and-add (worst ~330 cycles, early
  exit; same tcc__r2 clobber contract, DP-relative so it lands in the
  NMI-isolated register page).
- `tcc_div16`: flag → the pre-existing general `@software_div` path.
  mod16/sdiv16/smod16 wrap div16 → inherit.
- libtest vectors `r_nmi_mul/div/mod` (volatile operands force the
  runtime call) — proven non-vacuous: 20/23 pre-fix, 23/23 post-fix.

## Deliberately NOT fixed here

- `fixMul` / `fixLerp` (lib/source/math.asm) also use the hardware unit
  plus absolute-WRAM temps (fmul_*): **not NMI-callback-safe**, headers
  carry a @warning instead — a software fixed-point fallback wasn't
  worth the cycles until a real consumer needs it.
- The signed-division-by-negative-constant miscompile discovered by the
  same repro is UNRELATED (reproduces in main thread) → #114.

## Watch out

- Any future runtime/lib routine touching $4202-$4217 must either gate
  on `in_nmi_ctx` or document NMI-unsafety loudly.
- The main-thread path is untouched (one 6-cycle flag test per mul/div).
