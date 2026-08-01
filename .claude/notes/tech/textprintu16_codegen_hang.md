# textPrintU16 codegen-fragility hang (FIXED)

Status: **root-caused and fixed** 2026-08-01 (`fix(lib): rewrite textPrintU16`).
Found while authoring `basics/game_skeleton`.

## Symptom

A game loop that links the `sprite` module (calls `oamClear`) **and** prints a
number with `textPrintU16` hangs the CPU: the main thread spins forever in
`textPrint`'s `while (*str)` loop on a non-terminated buffer (X wraps to
`0xFFFE` — a pointer underflowed). Frames keep rendering, so it looks frozen.

## Root cause

**A codegen-fragile implementation of `textPrintU16`, not a hardware or NMI
bug.** The original built digits into a reversed stack buffer via a decrementing
pointer:

```c
char buf[6]; char *p = buf + 5; *p = '\0';
while (value > 0) { p--; *p = '0' + value % 10; value /= 10; }
textPrint(p);
```

Under some **link layouts** (linking `sprite.o` shifted the code into the
failing case) cc65816 miscompiles this loop: the digit loop underflows `p`
below `buf`, corrupts the stack, and `textPrint` then walks a non-terminated
pointer forever. It is **deterministic per layout and timing-independent** —
`oamClear(); textPrintU16(nonzero);` hangs with no NMI, no OAM DMA, no loop.

## Ruled out (each tested with a minimal repro, kept in scratchpad/repro)

- NMI / OAM-flush race — hang reproduces with OAM flush disabled and even
  deterministically at init with no NMI work.
- Auto-joypad window — `NMITIMEN=$80` (auto-joypad off), still hangs.
- Hardware mul/div unit corruption — standalone `v/10`, `v%10`, `a*b` all
  return correct values after `oamClear`.
- Data Bank register clobber — DB = $00 after `oamClear`.
- Memory overlap — `oamClear` writes only `$7E:0300-051F`; text state
  (`cursor_x/y`, `text_config`, `tilemapBuffer` @ $0520) is elsewhere.

The trigger bisected cleanly to `oamClear` (not `oamInit`), then to
`textPrintU16`'s non-zero (loop) path specifically — `textPutChar`, ROM
strings, and `textPrintU16(0)` are all fine.

## Fix

Rewrote `textPrintU16` MSD-first with leading-zero suppression — no reversed
buffer, no stack pointer:

```c
u16 place = 10000; u8 started = 0;
if (value == 0) { textPutChar('0'); return; }
while (place > 0) {
    u8 d = value / place;
    if (d || started) { textPutChar('0'+d); started = 1; }
    value %= place; place /= 10;
}
```

Output is byte-identical (every text example passes visual regression except
`basics_timer`, whose displayed vblank count shifts by a few frames because
the new routine's per-frame cycle cost differs — a benign fbhash rebaseline,
not a render change).

## Follow-up worth doing

This is a *symptom fix* at the lib layer. The underlying cc65816 miscompile of
"decrementing pointer into a stack array in a divide loop" may bite other code.
A compiler-side investigation (minimal C → bad ASM) belongs in a codegen
chantier; the repro in `scratchpad/repro` is a starting point. Until then,
avoid reversed-stack-buffer + pointer-underflow patterns in lib C.

## Cross-references

- `lib/source/text.c` — `textPrintU16` (fixed).
- `examples/basics/game_skeleton` — the example that surfaced it.
