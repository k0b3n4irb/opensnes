# What a real Super FX game needs that the SDK does not have — 2026-09-24

Owner's question: "what is missing to make a game using Super FX (LoROM) on
our SDK?" Answered from the tree at `c9ddef1d`, the corpus (queries and
chunk ids below) and luna v1.24.0. Hardware claims are cited; the two we
could not arbitrate are marked.

## 0. Framing: every Super FX game is LoROM

All eight commercial GSU titles (Star Fox, Star Fox 2, Yoshi's Island,
Stunt Race FX, Doom, Vortex, Dirt Trax FX, Winter Gold — `ghidra-superfx`
`72721042c9df39c5`) are map mode `$20`: Stunt Race FX's header reads map
mode `0x20`, ROM type `0x1A` (`stuntrace-recomp` `280e783fd3fed838`). The
65816 sees the ROM the LoROM way (`$00-$3F:8000`), the GSU sees it linearly
(`$40-$5F`), cart RAM is `$70-$71` (`sneslab` Super FX memory map). Our
`USE_SUPERFX=1` build is exactly that: `hdr_superfx.asm` + `memmap.inc`.

## 1. What the SDK has today

- Build: `wla-superfx` assembles `.sfx` → `.sfx.bin`, `.incbin`'d as a blob
  (`make/common.mk`). No symbol sharing between the two sides.
- Lib (`superfx` module, 4 routines + 2 inlines): `gsuLaunch` (copies a
  128-byte stub to WRAM, sets SCMR, starts the GSU, **disables NMI and
  busy-waits for STOP**), `gsuSetupBitmapTilemap`, `gsuDmaFullFrame`
  (V-counter poll, 16 KB DMA), `gsuSetupHdmaBlanking` (INIDISP HDMA:
  40 + 40 blanked lines), `gsuInit` / `gsuIsPresent`. Globals for SCBR /
  CFGR / SCMR / DMA source.
- Examples: `superfx_hello` (59 GSU lines, boot diagnostic),
  `superfx_3d` (219 GSU lines: wireframe cube, Bresenham, 60 fps with the
  40+40 letterbox). Tutorial `docs/tutorials/superfx.md` (the four ISA
  rules, PLOT setup, "NMI must be disabled").
- Test: luna runs the GSU natively (corpus liveness, fbhash, manifests),
  `--superfx-trace` (per-opcode CSV), `--force-mapper superfx`, cart-RAM
  dump. `KNOWN_LIMITATIONS.md`: "SuperFX C support is intentionally absent".
- Prior expert input: `.claude/notes/tech/superfx_expert_feedback.md`
  (delay slots, POR, CACHE, the split-frame double buffer we never shipped).

That is a **demo pipeline**: launch, block, transfer, repeat. A game is
what happens on the CPU *while* the GSU draws, and that is the hole.

## 2. The gaps, ranked by what blocks a game

### G1 — The CPU has nothing to run while the GSU owns the cartridge (blocking)

Hardware (arbitrated): only one processor owns Game Pak ROM/RAM at a
time (SCMR RON/RAN). The GSU WAITs when it lacks access; the CPU does not —
it reads garbage. For ROM the bus returns a **dummy byte keyed on the low
nibble of the address** (`$00` for 0/2/6/8/C, `$04` for 4, `$08` for A,
`$0C` for E, `$01` otherwise), and that is deliberate: an interrupt vector
fetched from `$FFEA/$FFEB` while the GSU owns the ROM reads **`$0108`**, so
Super FX games put `JML` stubs at `$000100` (BRK), `$000104` (COP),
`$000108` (NMI), `$00010C` (IRQ) **in WRAM** and keep their interrupt
handlers and GSU-time CPU code in WRAM (`sneslab` "Bus Conflicts"
`4a1e3a154e8eb7c7`; Stunt Race FX's vectors are `$0108` / `$010C`,
`280e783fd3fed838`). Code executing from the GSU cache runs with RON = 0,
which frees the ROM for the CPU (manual Book II §6.1.2 `3a7f008a1a412302`)
— but PLOT needs RAN and polygon data needs RON, so a renderer owns the
cartridge for most of the frame. Star Fox and Stunt Race set `SCMR |= $18`
for the whole GSU run (`620f6f2ed9a067fa`).

Ours: `hdr_superfx.asm` points NMI/IRQ at `NmiHandler` / `IrqHandler` in
ROM; crt0 writes no WRAM stubs; the tutorial's rule is "disable NMI". So
during a GSU job there is no VBlank: no joypad, no OAM upload, no audio
message pump, no HDMA table refresh, no C game logic — the CPU sits in a
128-byte WRAM loop. That is the difference between a demo and a game.

Needed:
1. Vectors `$0108` / `$010C` (+ BRK/COP) in `hdr_superfx.asm`, and crt0
   installing the four `JML` stubs at `$0100-$010F` at boot.
2. A **WRAM-resident NMI/IRQ path**: the part of crt0's handler that touches
   only PPU/APU/WRAM (OAM DMA from `$7E`, joypad, audio driver mailbox,
   HDMA tables in WRAM), copied to WRAM at boot and used whenever the GSU
   owns the cartridge. The current handler is ROM code with ROM tables.
3. **Code in RAM as an SDK feature**: a section kind that is stored in ROM,
   copied to `$7E:xxxx` by the existing data-init loop, and *linked* at its
   RAM address — for asm first, then for C functions marked for it (a
   `RAM_CODE` attribute / section, the way `FAR` marks data). This is the
   crux; it touches wlalink usage, `common.mk`, crt0 and the RAM budget.
4. **IRQ on STOP** (CFGR IRQ, SFR bit 15 says "it was the GSU" — manual
   Book II §5.4.2 `a938cb6359382bbd`) instead of polling GO, so the CPU is
   told the frame is ready. Free once the IRQ handler lives in WRAM.
5. A **bus-ownership discipline** the toolchain can check: while RON/RAN
   are set the CPU must touch neither ROM nor cart RAM. Static: hard.
   Dynamic: a luna diagnostic (see §4).

### G2 — Presenting the framebuffer (blocking for frame rate)

16 KB (256×128×4bpp) cannot cross the ~4 KB VBlank budget in one frame.
Today: HDMA-forced blank on 40 + 40 lines and a V-counter-polled full DMA,
buffer swap by hand through two globals. The expert note documents the
right pattern and we never shipped it: **split-frame double buffering**
(half the framebuffer per VBlank, swap `BG12NBA` only after both halves,
SCBR toggle on the GSU side) — 30 fps at full letterbox height; and Stunt
Race FX's variant, `MVN $70→$7F` of a 12 KB frame then DMA from WRAM
(`f684c0b9cc3ed1c0`), which returns the cartridge to the GSU before the
transfer even starts. Needed: `gsuPresent()` integrated with the NMI (not a
poll), SCMR heights 128/160/192 and 8bpp as parameters, the letterbox
derived from the height, and the WRAM-staging variant as the 30-fps
reference.

### G3 — The C ↔ GSU contract (blocking for anything beyond one job)

Today each example writes its own `gsuSetProgram` in asm, parameters go
through hand-chosen cart-RAM addresses, and C cannot name a GSU label.
Needed:
1. A **job table + `gsuCall(job, ...)`** with a documented ABI (registers
   in, register/mailbox out), asynchronous launch, completion by IRQ or
   poll, `gsuBusy()`.
2. A **symbol bridge**: `wla-superfx`'s symbol output → a generated
   `<name>_gsu.h` (job entry offsets, cart-RAM variables) and a `.inc` for
   the GSU side carrying ROM data addresses translated to the GSU's linear
   view (`$40-$5F` ↔ LoROM bank:offset) — today those numbers are computed
   by hand.
3. **Placement**: GSU code bank / PBR, 16-byte cache-line alignment of hot
   loops, CLSR (21 MHz on GSU-1/2) — as build knobs, not folklore.

### G4 — A GSU library and its asset pipeline (useful, not blocking)

`.sfx` include modules with the ISA rules baked into macros (branch + delay
slot, the `SUB R0` dummy-opcode trick, NOP padding before STOP, CACHE
before hot loops): clear/fill, line, **flat-filled triangle** (the cube is
wireframe; Star Fox is filled polygons with depth sort), sprite scale /
rotate in OBJ mode (Yoshi's Island), 8bpp blit. Tools: OBJ → vertex/edge/
face lists, PLOT palettes, `gfx4snes` has nothing for framebuffers.

### G5 — ROM and RAM sizing (cheap, blocks a big game first)

`ROMBANKS 8` is hardcoded in `memmap.inc` and the headers: 256 KB. Star Fox
is 1 MB, and a GSU game carries 3D data plus fat GSU code. ROM size must
become a per-project knob (banks, `ROMSIZE_VAL` already follows). Cart RAM:
`hdr_superfx.asm` declares `SRAMSIZE $00` and nothing in the expansion-RAM
field the expert note points at (`$FFBD`); emulators and carts size the RAM
from it. And the linker has no slot for `$70-$71`, so GSU variables and
framebuffers are magic numbers instead of `RAMSECTION` labels.

### G6 — Tooling and tests (luna asks + ours)

luna today: native GSU, `--superfx-trace`, cart-RAM dump, mapper override.
`luna state --out -` has **no `gsu` block** (checked on `superfx_3d`), the
manifests cannot assert GSU state, `luna profile` counts no GSU cycles, and
nothing reports a CPU access to ROM/cart RAM while the GSU owns it — the
bug class of G1, silent by construction. Asks filed in
`partners/luna/OPEN_luna.md`. Ours: `rom_coverage.py` does not measure
`.sfx` code; there is no GSU library fixture (the `fx` fixture is LoROM
without a chip — a `libtests_gsu` under `USE_SUPERFX=1` asserting PLOT
output through cart-RAM peeks is possible today).

### G7 — Real hardware

The FXPak Pro chip list in the corpus (`sfc-dev-wiki` "Testing Code / Real
Hardware / SD2SNES" `4ec0785bcc6b469b`: DSP1-4, ST-010, Cx4, S-RTC) does not
include Super FX — the SuperFX3 RP2350 cartridge project exists for that
reason and is "under active development", with the caveat that FX3 adds
features the real GSU lacks (`4ac1598847196c16`). So the GSU rows of
`docs/HARDWARE_VERIFICATION.md` cannot run on our FXPak Pro; they need a
donor cartridge or the FX3 cart. **Unmeasured** until then; luna is the
reference, and the corpus is the arbiter of what luna does.

### G8 — "C for the GSU": still no, and that is fine

No C compiler for the GSU exists in the corpus: Argonaut worked in
assembly; modern options are `casfx` (a ca65 macro pack, used by libSFX)
and DiscoC (a custom systems language with a GSU backend, not C —
`56faa581d042b393`). Our documented position holds: GSU = assembly, made
livable by G3 + G4, not by a compiler.

## 3. The path

One example forces the four blocking gaps to exist as library code, and
is measurable by luna at every step: **`superfx_game_skeleton`** — a CPU
game loop and NMI that keep running (input read, sprites moving, music
playing) while the GSU renders every frame, presented at 30 fps by the
split-frame pipeline, completion by IRQ. Order:

1. G5 (a day: ROM-size knob, expansion-RAM byte, a `$70` slot).
2. G1 (the chantier: vectors + WRAM stubs, the WRAM NMI path, then RAM
   code for C) — validate with the skeleton's "music keeps playing while
   the GSU draws" manifest.
3. G2 on top of it (`gsuPresent`, two variants).
4. G3 (job table, symbol bridge) — the skeleton's second GSU job.
5. G6 asks to luna now, in parallel; `libtests_gsu` as soon as G1 lands.
6. G4 grows from what the skeleton and the showcase need.

Effort: G1-G3 together are a multi-week chantier; G1.3 (code in RAM) is
the part that changes the SDK's memory model and needs the same care as
chantier B2 had.

## 4. Queries (reproduce)

- Mapping and titles: `snes_search("Super FX (GSU) cartridge memory mapping:
  is it LoROM (mode 20) … which commercial games use it")`.
- Bus ownership: `snes_search("Super FX: while the GSU is running with
  RON/RAN set, what happens when the SNES CPU reads ROM or RAM …")` →
  `4a1e3a154e8eb7c7` expanded with `snes_get(context=2)`.
- Framebuffer: `snes_search("Super FX framebuffer: screen height and bpp
  modes (SCMR HT, MD bits) …")`.
- IRQ on STOP: `snes_search("Super FX GSU interrupt to the SNES CPU on
  STOP …")`.
- Toolchains: `snes_search("GSU development kit and toolchain …")`.
- FXPak Pro: `snes_search("Does the sd2snes / FXPak Pro flash cartridge
  support Super FX …")`.
