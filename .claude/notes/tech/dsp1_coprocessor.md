# DSP-1 coprocessor — support paths for OpenSNES

Status: **Path A scoped** (2026-08-02) — research merged in
[`dsp1_reference.md`](dsp1_reference.md); ready to plan the `dsp1` module +
pseudo-3D example. Path B watches upstream (wla-dx#392).

## The chip

The DSP-1 is an **NEC µPD77C25** DSP running **Sony's fixed mask-ROM
firmware**. It does fixed-point matrix / vector / projection math — the
pseudo-3D workhorse of Pilotwings, Super Mario Kart, Ballz 3D, etc.

- **DSP-1 / 1A / 1B / 2 / 3 / 4** = the same µPD77C25 silicon with different
  firmware ROMs.
- **ST010 / ST011** = the µPD96050 cousins (bigger, same family).
- The firmware is **mask-programmed at manufacture** — you cannot change what a
  *genuine* chip computes.

## Two very different meanings of "DSP-1 support"

Only one of them needs a wla-dx change. This distinction is the whole point.

### Path A — *use* the stock DSP-1 from 65816/C (MMIO command interface)

The CPU talks to the DSP-1 through memory-mapped **data (DR)** and **status
(SR)** registers: write a command word + its parameters, poll SR for
request-for-data / data-ready, read the results. The function menu is fixed by
Sony's firmware (~30 commands: matrix set, vector transform, projection,
attitude, coordinate math…).

- **Needs no wla-dx change** — it is pure MMIO from C.
- **luna runs DSP-1 natively** (CLAUDE.md / KNOWN_LIMITATIONS), so we can build
  *and test* it in the harness today.
- This is what a **`dsp1` library module + a pseudo-3D example** would use, and
  what we are scoping now.

### Path B — *assemble* custom µPD77C25 microcode

Writing your *own* DSP program (not Sony's fixed one) — the analog of
`wla-superfx` for the GSU. This is what **vhelin's wla-dx issue #392** is about.

- Issue #392: opened **2021-03-18** by vhelin (maintainer), labels
  `enhancement` + `long-term`, **open / unassigned / no PR / no spec**. A
  wishlist item, not active work — do not expect it soon.
- If it lands, it becomes the natural **4th `wla-*` assembler** beside
  wla-65816 / wla-spc700 / wla-superfx, and drops into our existing
  **two-stage build pattern** (`.sfx → wla-superfx → wlalink -b → .sfx.bin →
  .incbin`) with almost no new build-system design.
- **Caveat:** custom microcode cannot run on a *genuine* DSP-1 (fixed
  mask-ROM). It is an **emulator + FPGA-flashcart** target (luna,
  SD2SNES/FXPak) — the same delivery reality as most homebrew, so not a big
  diminisher, but worth stating.

## Why OpenSNES is well-positioned (either path)

- **luna executes DSP-1 natively** — the piece usually missing for exotic chips
  is already there for us.
- **wla-dx pin is clean** — v10.7, **0 local patches** — a pin bump picks up
  Path B the day it lands.
- **We've built the shape twice** — the GSU and SPC700 two-stage flows mean
  Path B is mostly a known integration, not a research problem.
- Path A completes the **coprocessor triad**: SA-1 (65816 ISA) + SuperFX (GSU
  ISA) + DSP (µPD77C25) — "write code for every user-programmable SNES
  coprocessor in this SDK."

## Open questions for the Path A scope — RESOLVED (2026-08-02)

Answered by the merged research in [`dsp1_reference.md`](dsp1_reference.md):

1. **luna's firmware handling** — ✅ **luna is LLE and needs an external
   `dsp1b.rom`** (confirmed by binary inspection: a `luna-cpu-upd96050` crate +
   `dsp1b.rom` refs + a firmware-install subcommand + "missing coprocessor
   firmware" errors; and by the user). NOT a hard blocker: the dev installs the
   dump once (like `install-luna.sh`); the example's luna test is **gated on
   firmware presence** and skipped in CI — the `INPUT-DEP` treatment. See
   reference §10.
2. **ROM mapping** — ✅ cartridge-type `$03` (ROM+DSP) / `$05` (+RAM+battery);
   DR/SR at LoROM `$30-$3F:8000`/`C000` or HiROM `$00-$0F:6000`/`7000`. New
   `hdr_dsp1.asm` + `memmap_dsp1.inc`; verify if a `$FFD5`-style patch is
   needed. Reference §2, §7.
3. **Fixed-point formats** — ✅ typed per slot: I (int), T (signed 1.15), A
   (16-bit angle, full circle = 2¹⁶). Reference §4.
4. **Command subset** — ✅ first target **Attitude ($01) → Objective ($0D) →
   Project ($06)**, with a software-perspective `Polar` ($1C) fallback for
   bring-up. Reference §5, §6.

**Still verify-before-code** (the ◆ items, resolve during bring-up against
bsnes/snes9x source or a luna trace): DR byte order + RQM per-byte/word;
Parameter operand scaling; Objective/Subjective direction; exact out-counts for
Polar/Objective/Gyrate.

## Watch

- Upstream: **wla-dx#392** (github.com/vhelin/wla-dx/issues/392) — Path B.
- If it moves, revisit this note and the pin.

## Cross-references

- `.claude/notes/chantiers/ecosystem.md` — where a DSP-1 chantier would land.
- `KNOWN_LIMITATIONS.md` / `CLAUDE.md` — luna's native SA-1/SuperFX/DSP-1 note.
- The GSU two-stage build in `make/common.mk` — the Path B integration template.
