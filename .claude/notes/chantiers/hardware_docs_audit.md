# Hardware docs audit — corrections from the Cartouche cross-check

**Status: EXECUTED 2026-09-02** (edits applied for F1-F10 + extras;
`make lint-docs` green; full rebuild + luna suite run for validation;
commit pending user sign-off). Phase 2 re-audit results and the
retrospective are appended at the bottom. Files touched: OAM.md,
REGISTERS.md, KNOWN_LIMITATIONS.md, tutorials {dma, hdma, window,
scrolling, sram, math, sa1, mode7}, templates/crt0.asm (comments only
— zero emitted bytes), notes tech {enhancement_chips_research,
nmi_context_hardware_muldiv, hdma_notes}. Remaining optional follow-up:
the luna non-repeat window probe (empirical confirmation of F6).
**Origin:** full challenge of the SDK's hardware claims against the
Cartouche RAG corpus (21 `snes_search` queries + 2 WebFetch
counter-checks, session 2026-09-01). Audit report lived in
`/tmp/opensnes_review_cartouche_2026-09-01.md` (ephemeral — everything
actionable is carried here). Companion plan for the RAG side:
"Cartouche v2" (artifact, owner has the link); its Phase 1-2 gates our
Phase 2 below.

**Headline:** the SDK's hardware documentation is broadly sound — most
claims confirmed by arbiter sources (anomie, fullsnes, snesdev-wiki,
undisbeliever). Seven findings, all documentation-side; the lib code
itself needed no fix (in two cases the lib is *more correct than its
own docs*). One finding (F6) may reveal over-constrained HDMA guidance
and needs a luna probe to settle.

---

## Phase 0 — proven doc fixes (no dependency, start anytime)

All Class D (docs only): `make lint-docs` + `make tests`. Evidence is
already gathered and arbiter-backed; no further verification needed.
Single PR, conventional scope `docs`.

| # | Fix | Where | Evidence |
|---|-----|-------|----------|
| F2 | OAM pair-write latch applies to the **low table only**; writes at internal address ≥ $200 (high table, bytes 512-543) apply **immediately**. Current text presents pair-writing as absolute ("if you only write one byte, nothing happens"). | `docs/hardware/OAM.md:41-82` | snesdev-wiki PPU-registers pseudocode for $2104; sfc-dev-wiki ("the high table is affected immediately"); anomie-regs. Bonus from sfc-wiki: alternated write/read never commits the low table (worked example "01 02 01 03"). |
| F3 | "Hiding Sprites" teaches `Y=240` alone. Sprites wrap vertically: a 32x32 at Y=240 shows ~16 lines at the top of the screen; no Y value can fully hide a 64x64. Rewrite on the model of `lib/source/sprite.c:133-141` / `oamClear` (Y=240 **plus** X bit 8) — the lib already does this, with the correct explanatory comment. | `docs/hardware/OAM.md:120-129` | nesdoug (64x64 wrap); consistent with sfc-wiki sprite range rules. Add the refinement: per anomie/sfc-wiki "Drawing the Sprites" §1, X=256 is treated as X=0 for **Range and Time** — a hidden large sprite whose Y wraps onto visible lines still consumes the 32-sprites/34-slivers scanline budget. |
| F4 | "NMI runs for roughly 35,000 master cycles" → actual NTSC VBlank is **37 scanlines** (V=225→261) ≈ **50,470 master cycles** (22 lines ≈ 30,000 with overscan). Keep the ~4 KB user budget (raw ceiling is 37×1324/8 ≈ 6.1 KB before OAM DMA/scroll/joypad/NMI overhead) but fix the number and anchor the derivation. | `KNOWN_LIMITATIONS.md:38`, `docs/tutorials/dma.md:240` | snesdev-wiki Timing (Vertical Blank; 1324 usable clocks/line after 40-clock DRAM refresh); fullsnes (1364 master clocks/scanline). |
| F5 | "1369 cycles per scanline at 3.58 MHz" → it is **1364 master cycles** (1324 usable), and 3.58 MHz is the FastROM *CPU* rate (≈227 CPU cycles/scanline at 6 clocks/cycle). Rewrite the HDMA budget section in master cycles; the percentages stay valid. | `docs/tutorials/hdma.md:365`, `docs/tutorials/math.md:18` | fullsnes Horizontal Timings; snesdev-wiki Timing/CPU. |
| F2b | OAM attribute-byte layout in the hardware doc is garbled (bit 3 counted twice: "Bit 3: Palette (high bit …)" + "Bits 1-3: Palette select"). Align on the tutorial's correct `vhoopppN`. | `docs/hardware/OAM.md:26-32` | snesdev-wiki OAM layout (`VHPP CCCt`). |
| + | Free extras worth absorbing: (a) VMADD **increments even when the VRAM write is ignored** — clean explanation of "shifted" partial DMAs; (b) only OBJ palettes **4-7** participate in color math; (c) VRAM writes are also ignored during **HBlank**, not just active display. | `docs/tutorials/dma.md`, `docs/tutorials/colormath.md` | snesdev-wiki VMDATA; sfc-dev-wiki Sprites/Palettes. |

## Phase 1 — findings needing verification before the doc change

### F1 — SA-1 SIWP ($2229): the "disputed polarity" is likely resolved
Current entries say "the Super Famicom Dev Wiki **and fullsnes** say
bit=1 protects" and keep `$FF` as an emulator-driven bet. The Cartouche
corpus carries an established-error annotation on the wiki page:
fullsnes actually reads **"0=Protect, 1=Write Enable"** (i.e. agrees
with Mesen2/snes9x/luna and with `$FF`), the wiki page is
self-inconsistent (its own $2226 SBWE / $2227 CBWE table uses the
opposite convention), and nesdev thread t=18708 is cited (nocash:
"Unlocking is done by setting the write-enable bits"). WebFetch
counter-check (2026-09-01): the exact nocash quote was not on thread
pages 1-2, but the same thread has Near observing, on the SA-1
write-protection register family: *"Per the documentation, these writes
should fail, and yet they must not fail in order for the game to run."*

Steps:
1. Manually verify fullsnes' SA-1 I/O section (2229h) — our
   "and fullsnes" attribution on the *protect* side appears to be an
   unsourced extension: the original tech note
   (`enhancement_chips_research.md`, 2026-03-21 entry) only listed the
   wiki and PeterLemon on that side.
2. Rewrite from 🟡 "disputed" to "resolved — wiki page wrong; `$FF` =
   write-enable is hardware-correct", citing nesdev t=18708. Keep the
   real-cartridge test as prudence, not as a condition.
3. Touch all four sites in one commit: `KNOWN_LIMITATIONS.md:189-214`,
   `docs/tutorials/sa1.md:59`, `docs/hardware/REGISTERS.md:551`,
   `.claude/notes/tech/enhancement_chips_research.md`.

### F6 — "repeat mode required because scroll/window registers forget" — probable misconception, settle with luna
`docs/tutorials/hdma.md:99-136,324` and the window tutorial claim BG
scroll / window registers are "consumed every scanline" and "forget"
their value, making repeat mode mandatory. **No authority source
supports this** — the Cartouche query returned *only our own docs*
(circularity) with an explicit "no arbiter source" flag. PPU registers
latch; repeat mode exists to write a *different* value each line, not
to refresh a value that would evaporate.

Probe (Class C, one throwaway example or an existing window example
variant): static non-repeat HDMA table on WH0/WH1 and on BG1HOFS,
capture fbhash via luna. Outcomes:
- symptom absent → rewrite the three-classes doctrine (real distinction:
  constant-per-band vs different-per-line) and check whether lib or
  examples pay useless repeat-mode cycles (~8 cycles/byte/line/channel);
- symptom present → genuinely undocumented hardware behaviour; document
  it with the repro and feed it back to the Cartouche corpus.

Also fix while in there: `hdma.md` says twice "the PPU walks the
table" / "let the PPU drive" — HDMA is executed by the 5A22's DMA
controller (CPU halted), not the PPU.

### F7 — fixMul/NMI: mechanism stated as fact is unverified
The #113 symptom (garbage fixed-point values in NMI callbacks) is real
and the shipped mitigation is correct. But "the unit **shares silicon**
with the auto-joypad shift logic" is confirmed by no arbiter: sources
document garbage reads of **$4218-F** during auto-read (sfc-wiki
timing) — nothing about $4214-$4217 corruption. Either reformulate
`KNOWN_LIMITATIONS.md:176-187` + the tech note as "observed
empirically; mechanism hypothesised", or design the luna experiment
isolating hazard 1 (auto-joy window) from hazard 2 (non-reentrancy):
hardware multiply inside the auto-joypad window with **no** in-flight
main-thread multiply. Low priority — user-facing guidance is right
either way.

## Phase 2 — full re-audit, gated on Cartouche v2

Wait for the RAG plan's Phase 1-2 (source-diversity ranking +
`exclude_sources`, fullsnes re-chunking, ideally `contrast=true`).
Then:
1. Re-run the challenge over the **whole** doc surface — this audit
   sampled `KNOWN_LIMITATIONS.md`, `OAM.md`, `REGISTERS.md` and ~6 of
   the 21 tutorials; the rest (mode7, sram, audio, collision, map,
   scrolling, superfx…) is unaudited.
2. Query with opensnes-docs excluded so the corpus can only *challenge*
   us, never confirm us with our own words.
3. Adopt the audit's golden queries (listed in the Cartouche v2 plan
   annex) as the recurring cross-check harness.
4. If opensnes-docs stays in the corpus: make sure the snapshot is
   re-captured after each release (stale snapshots re-serve retracted
   claims — precedent: the QBE forwarding-miscompile retraction,
   `e4b418c8`).

## Phase 2 re-audit results (2026-09-02, Cartouche v2, opensnes-docs excluded)

Cartouche v2 shipped (source diversity, `exclude_sources`, `snes_get`,
contrast mode, anchored citations, new sources incl. the official
Nintendo dev manual). Re-audit of the remaining tutorials run with
`exclude_sources=["opensnes-docs","opensnes-notes-tech"]`.

### F1 — CLOSED as a question (doc rewrite still pending)
Three independent documentary confirmations of bit=1 = write-enable,
all retrieved verbatim through the v2 corpus:
- fullsnes (anchored: `fullsnes.htm#snescartsa1memorycontrol`):
  *"2229h SNES SIWP — 0-7 Write enable flags for eight 256-byte chunks
  (0=Protect, 1=Write Enable)"*.
- **nintendo-devmanual-book2** §4.1.25: *"SIWP0~7 … Setting 0: Write
  disable, 1: Write enable"*.
- nocash, nesdev **p=237542#p237542** (2019-04-14): *"Unlocking is done
  by setting the write-enable bits (not by clearing them)"*; Near
  corroborates at p=237733.
The crt0 `$FF` is hardware-correct. Proceed with the Phase 1 rewrite;
the manual fullsnes check is done (verbatim above).

### F6 — SETTLED against our doctrine (luna probe now confirmatory only)
snesdev-wiki (arbiter), *Drawing window shapes*: a single static window
is shaped with HDMA in **"2 registers write once" mode** on WH0/WH1;
the rectangle worked example states *"All segments can be built using
non-repeating HDMA table entries"* (full pseudocode on the page).
So "repeat required because window registers forget" is wrong.
`window.md` is additionally **self-contradictory**: line 99 says "If
you write them once, the boundaries stay the same on every scanline"
(correct), line 197 says the PPU "forgets" per scanline (wrong).
Rewrite window.md + hdma.md's three-classes table; run the luna
non-repeat probe as empirical confirmation and to explain whatever
symptom motivated the original claim.

### F8 (new) — scrolling.md:252 "latched at the start of each frame" is wrong
snesdev-wiki PPU-registers (Scroll): *"The scroll offset is always
relative to the top-left of the screen, **even when updating mid-frame
with HDMA**"*. Mid-frame writes take effect (next scanline) — that is
exactly why the HDMA parallax pattern described 100 lines earlier in
the same tutorial works, and why unsynced CPU writes tear ("no visible
effect" is wrong too). Fix the "VBlank Timing" paragraph; the shadow
+ NMI-commit design it documents remains the right advice.

### F9 (new) — sram.md cycle-cost section is off by ~6× per byte, ~40× on the 8 KB claim
The lib uses MVN (`lib/source/sram.asm`), and MVN costs **7 CPU cycles
per byte** (6502org arbiter + snesdev-wiki + WDC manual) — not "8
master cycles per byte" (that is the DMA figure; the section even
correctly notes there is no DMA path). Corrected numbers: 256-byte
save ≈ 1.8 K CPU cycles (≈13 K master); full 8 KB ≈ 57 K CPU cycles
≈ 430 K master ≈ **~120 % of a frame**, not "~3 %". SRAM writes don't
need VBlank, so the fix is the numbers (and advising chunked
auto-saves for multi-KB payloads), not the API.

### F10 (new gotcha to ADD) — BG1 scroll and Mode 7 matrix share the write-twice latch
snesdev-wiki **Errata** (+ nesdev p=249422): *"The Mode 7 multiplier
(MPY) result can be corrupted if an interrupt or HDMA transfer writes
to a BG1 scroll register or Mode7 Matrix register in-between the two
M7A writes. (The Mode7 scroll and Mode 7 matrix registers share the
same write-twice latch)."* $210D/$210E are dual BG1HOFS/M7HOFS and
feed the same latch as $211B-$2120. The lib itself never reads MPY
($2134-6 — verified: only the CPU multiplier $4202 is used), but any
user combining the mode7 module with HDMA on BG1 scroll (or the
F-Zero per-scanline M7A pattern in mode7.md) is exposed. Add a 🟡
entry to KNOWN_LIMITATIONS + a gotcha in mode7.md/hdma.md.

### Verified clean in Phase 2
audio.md (32 kHz native rate, VxPITCH $1000 = 32000 Hz, max $3FFF —
snesdev-wiki/fullsnes/anomie-sdsp); mode7.md register table
(write-twice, signed 13-bit M7X/M7Y); sprites.md OBJ name base
($2000-word pages); colormath.md COLDATA 5-bit; sram.md mapping
tables (LoROM $70-$7D / HiROM $30:6000). graphics.md, map.md,
input.md, collision.md, animation.md, panel.md, game_states.md,
mosaic.md, scrolling.md (rest): no contested hardware claims found.

## Retrospective — past struggles these findings explain (2026-09-02)

**F6's origin found.** `.claude/notes/tech/hdma_notes.md` records a
failed non-repeat test: *"Direct Mode (BROKEN on this toolchain) …
Each entry: [count] [data0][data1]… different data per scanline within
the count … Tested with 112+112 split — produced no visible effect
(straight lines). Root cause unknown — may be assembler/linker issue."*
That format is **repeat mode's layout without bit 7**. Per anomie-regs:
non-repeat = count + **one** scanline of data, written once and held;
repeat = count scanlines of data. A table built as [112][112×data]
makes the hardware write the first group once and hold it 112 lines —
*exactly* the "straight lines" observed. Root cause: a misread of the
table format — not the toolchain, not the hardware. That experience
then hardened into the false "registers forget → repeat required"
doctrine in window.md/hdma.md, and left an unjustified suspicion on
the assembler. Update hdma_notes.md when F6's rewrite lands.

**The SA-1 detour was pure bad-source cost.** The 2026-03-21 note:
*"I initially trusted the wiki, flipped crt0 to $00, and it broke SA-1
in Mesen2. Reverted."* → months of "disputed" status, warnings in 4
files, a do-not-touch tripwire. The wiki page was simply wrong
(established error; fullsnes + Nintendo manual + nocash all agree).

**hdma_wave_bug.md (open TODO) can now be instructed.** The old
`fillWaveTable` wrote bank-$7E tables via $2180 with double buffering
→ "massive visual corruption", root cause never found; the current
code sidesteps it (tables in bank $00, plain C writes). The corpus
documents the exact constraints the investigation was groping toward:
DMA cannot copy RAM→RAM via $2180 (snesdev-wiki Errata), $2181-3 are
open bus on read, DMA writes to them behave non-obviously.

**Honest counter-examples:** the wrong figures (F4 VBlank budget, F5
1369, F9 sram cost) never hurt anyone — conservative or never load-
bearing. The OAM-Y "two camps" saga was a real past struggle but an
SDK-convention issue, not a false hardware claim.

Common pattern in the real struggles: one wrong or misread external
source + no arbiter = weeks of friction. That is the RAG's job.

## Confirmed-claims ledger (no action; keep for future audits)

Verified against arbiters on 2026-09-01: VRAM writes ignored outside
VBlank/force-blank; $2180-$2183 NMI race; DMA = 8 master cycles/byte
+ 8/channel; HDMA init overhead 18 + 8 direct / 24 indirect, max
466/line; sprite drawn at Y+1, scanline 0 hidden; OBJ palettes at
CGRAM 128; mul 8 / div 16 machine cycles; auto-joypad = 4224 master
cycles starting dots 32.5-95.5 of first VBlank line (consistent with
the #113 window); IPL handshake $AA/$BB + $CC/index/echo upload.
