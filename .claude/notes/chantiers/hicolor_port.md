# Chantier: HiColor port (#107) — H-IRQ surface + CGRAM streaming

Status: IN PROGRESS (started 2026-07-13). Branch: `wip/hicolor`.

## Premise correction (posted on #107)

krom's HiColor is NOT indirect HDMA. `HDMA/HiColor64PerTileRow` uses an
**H-timer IRQ every scanline** (HTIME=190) whose handler fires a
**16-byte general DMA into CGDATA** — 4× beyond HDMA's per-line ceiling.
CGRAM ping-pongs between two 64-color banks: even tile rows render
colors 0-63 while the stream refills 64-127 for the odd row, and vice
versa. CGADD resets every 16 lines (`(line & $F) == 8`); the tilemap
alternates palette bits 0-3 / 4-7 per row, sub-palette by screen
quarter (64px). VBlank reloads the first 128 bytes and resets A1T0;
the .pal (28 rows × 128 bytes = 3584) streams sequentially via the
auto-advancing DMA source. Net: 28×64 = **1792 colors from a 4bpp BG**.

Reference facts (full-ASM read, `luna_tests/PPU/HDMA/HiColor64PerTileRow`):
- Mode 3, **BG2** 4bpp (BGMODE=%00001011), BG2SC=$7800 map, tiles at $0000
- BG2VOFS = 31 (aligns tile-row boundaries with the reset cadence)
- Tilemap: sequential tiles (no dedup, 896 unique), even rows pal 0-3,
  odd rows pal 4-7, one sub-palette per 64px screen quarter
- Tiles: 28672 B at VRAM $0000; map 1792 B at $7900 (why $7900 not
  $7800: VOFS 31 ⇒ first visible map row is map row 1 — row 0 unused)
- IRQ: ack $4211, latch line via $2137/$213D, skip if line ≥ 216,
  reset CGADD when (line & $F)==8, always DMA 16 B ch0 → $2122
- VBlank: CGADD=0, A1T0=pal start, DAS=128, fire ch0
- NMITIMEN = %10010000 (NMI + H-IRQ; krom drops auto-joypad, we keep it)
- Converter ships with the demo: `GFX/SNESBGPAL64tilerow.py` — per
  64×8 segment: Pillow quantize to 15 colors, index 0 = black, 4bpp
  planar tiles sequential. Ours follows the same contract.

**luna validation of the reference**: renders correctly, 3 consecutive
frames identical (0 diff px). vs krom's shipped PNG: 18.8% px differ,
concentrated on the first scanline of each 8-line band = IRQ-latency
race (different emulator timing), inherent to the technique. ⇒
acceptance metric is **luna-vs-luna** (both ROMs, same timing), per the
port-validation pattern. No luna blocker.

## SDK gap and design

The real gap: **no IRQ surface at all**. `IrqHandler` in crt0 is a
hardcoded ack+rti stub; `Start:` does `sei` and nothing ever `cli`s;
`interrupt.h` only has nmiSet/nmiSetBank/nmiClear.

### 1. crt0.asm (Class A — nmi_audit checklist applies)

- Append `irq_callback dsb 4` at the END of the compiler-register DP
  enum (after `tcc__fp` — appending shifts nothing; PVSnesLib-compat
  offsets for nmi_callback untouched).
- Boot init points it at `DefaultIrqHandler` (ack $4211 + rti),
  mirroring the nmi_callback init.
- `IrqHandler: jml [irq_callback]` — zero imposed overhead; the
  callback owns everything (save/restore, ack, rti). NMI path
  untouched (audit items: order/budget/handshake/DP all N/A).

### 2. lib interrupt module (irq*.h surface in interrupt.h)

- `irqSet(void *handler)` / `irqSetBank(handler, bank)` / `irqClear()`
  — **raw-vector contract, ASM handlers only**, documented loudly:
  handler must save/restore registers it uses, ack $4211, `rti`.
  A C-callback trampoline is deliberately NOT offered: per-scanline
  IRQs can't afford C prologue latency/jitter, and a slow handler
  produces silent visual corruption (worst kind). Escape-hatch level
  API per PHILOSOPHY; revisit iff a V-IRQ (once/frame) use case shows
  up where a C callback is affordable.
- `irqSetHTimer(u16 h)` → $4207/8; `irqSetVTimer(u16 v)` → $4209/A.
- `irqEnable(u8 flags)` (IRQ_HTIMER $10 / IRQ_VTIMER $20 / both $30):
  RMW into the NMITIMEN shadow + `cli`; `irqDisable()` clears bits +
  `sei` is WRONG (would mask nothing since NMITIMEN gates) — just
  clear the NMITIMEN bits, keep I-flag clear.
  NMITIMEN shadow: crt0 writes literal $81 at boot — introduce
  `nmitimen_shadow` (crt0-owned, init $81) so irqEnable doesn't
  clobber auto-joypad/NMI bits.
- Constraint doc: an IRQ handler that fires general DMA owns that
  channel; don't run lib DMA calls concurrently in the main loop.

### 3. Example `examples/graphics/effects/hicolor_1792/`

- `main.c`: init via lib (consoleInit, setMode BG_MODE3, BG2 setup,
  dmaCopyVram loads), tilemap **generated in C** (fully regular
  pattern — no committed map), DMA ch0 pre-config, nmiSet C callback
  for the VBlank 128-byte reload (VBlank-timed, C is fine there),
  irqSet(asm handler) + irqSetHTimer(190) + irqEnable(IRQ_HTIMER).
- `irq_stream.asm` (ASMSRC): krom's handler shape verbatim + register
  save/restore (our IRQ interrupts arbitrary C, krom's only ever
  interrupted `wai`).
- Assets: own 256×224 art (rich smooth gradients — must genuinely
  exceed 256 on-screen colors), converted by `devtools/hicolor64.py`
  (same contract as krom's converter, credited). **Generated .pic/.pal
  committed** (wavetable.bin precedent) so builds don't depend on
  Pillow; the tool + source PNG committed for regeneration.
- README: register-fidelity table + luna-vs-luna parity (fbhash /
  pixel-diff both ROMs in luna), krom credited.

### #109 note

64PerTileRow does NOT exercise direct color — #109 keeps needing its
own consumer later in the arc (posted on #107).

## Validation

Per `.claude/notes/patterns/peterlemon_port_validation.md` + Class A
(crt0 touched): `make clean && make`, full `make tests`, triage table
(IRQ path affects nothing unless enabled — but crt0 DP enum grew:
check likemario/tetris RAM budget lines), fresh-checkout proof,
CI green before announcing.

## Debugging log (2026-07-13/14) — two SDK bugs found by the port

Symptom chain and root causes, in order:

1. **White screen** → `nmiSet(hicolorVblank)` with the callback linked
   in a SUPERFREE section that landed in bank 1; nmiSet assumes bank 0.
   Diagnosed via luna's `dma.channels[]` (source had run $62E0 past a
   $E00 table = VBlank rewind never ran). Fix: `nmiSetBank` + the
   far-pointer bank-extraction idiom. Example code carries the warning.

2. **8-line white stripes (odd tile rows)** → the OPVCT hi/lo read
   pointer was mid-sequence for the entire session: **`consoleInit()`
   seeded rand with SINGLE reads of OPHCT/OPVCT** (console.c:73). Every
   subsequent latched pair read (hi,lo) instead of (lo,hi) → X =
   (lo&1)<<8|hi → half the scanlines misread as V≥256 → skip → stream
   at half cadence. THE KEY MEASUREMENT that unmasked it: an in-ROM
   probe counting entries/fires/skips per frame gave the SAME half-rate
   in luna AND Mesen2 (via the mesen2-rpc client) — two independent
   emulators agreeing means OUR bug, not an emulator bug. Fixed in
   console.c (`rand_seed ^= REG_STAT78` after seeding — the STAT78 read
   resets both pointers, and it must stay AFTER the counter reads).
   The IRQ handler ALSO reads $213F defensively (an IRQ can interrupt
   another reader mid-pair; inherent hardware hazard, documented).

3. **nmiSetBank clobbered NMITIMEN with a literal** — would have killed
   IRQ bits on any nmiSet after irqEnable. Fixed via `nmitimen_shadow`
   (all $4200 writes compose through it; crt0-owned, consoleInit
   resyncs it deliberately).

Cross-emulator proof of final cadence: in-ROM probe reads `$4302` back
at V==100 → `$86C0` = pal + 128 + 100×16 in BOTH luna and Mesen2.
Residual vs static expectation concentrates on band-first-lines exactly
like the reference (structure identical; amplitudes in the example
README). Initially suspected a luna H-IRQ bug — WRONG, and the
Mesen2 cross-check prevented a bogus luna issue. Lesson reinforced:
never file an emulator bug without a second-emulator repro.
