# Chantier superfx-runtime — a CPU that keeps running while the GSU draws

**Status:** DONE, phases A to F (2026-09-24 to 2026-09-29; header corrected
2026-10-03, it still said "phases C-F open"). Phase F's last part, linking
the GSU program at its real ROM address, shipped 2026-10-03 (`GSU_BANK`,
fixed placement at `$n:8000`). One thing is left open, written below and
not on a branch: a save for Super FX games (the last section). **Catalogue entry:** `.claude/STRUCTURAL_DEFECTS.md` §E3.
**Origin:** `.claude/notes/reviews/2026-09-24_superfx_game_gaps.md` (G1,
G2, G3). **Risk:** High (crt0, memory model). **Effort:** several weeks.

## 1. Why

What the SDK ships for the Super FX is a demo pipeline: `gsuLaunch()`
copies a 128-byte stub to WRAM, disables NMI, starts the GSU and busy-waits
for STOP. During a GSU job nothing else happens on the CPU — no joypad, no
OAM upload, no audio message pump, no game logic. A game is precisely what
the CPU does *while* the GSU owns the cartridge.

The hardware has a designed answer (Nintendo manual Book II §5.4.1, table
2-5-1, chunk `6e4f8ad80b420504`; sneslab "Bus Conflicts"
`4a1e3a154e8eb7c7`): while the GSU owns the ROM the CPU's vector fetch is
answered with a dummy vector — `$0100` (BRK/ABORT), `$0104` (COP), `$0108`
(NMI), `$010C` (IRQ) — and the manual prescribes a jump at each of those
WRAM addresses to handlers kept out of the Game Pak ROM. Stunt Race FX
does exactly that: NMI handler DMAed to `$7E:A2D9` at boot, trampoline at
`$0108` (`stuntrace-recomp` `5fea98124a05e98d`).

## 2. Phases — each one luna-measurable, each one shippable

| Phase | What | Test | Status |
|---|---|---|---|
| **A** | Vectors + WRAM stubs: `.gsu_vectors` RAMSECTION at `$0100` (Super FX builds only), crt0 installs four `JML` at boot from a ROM table, `hdr_superfx.asm` native vectors = `$0100/$0104/$0108/$010C`. Behaviour identical while the GSU is idle. | `coproc_superfx_hello.toml` asserts `$5C` at the four stubs; fbhash of both GSU examples unchanged; WRAM oracle re-captured for the two (the `.system` section moved 16 bytes) | done 2026-09-24 (this commit) |
| **B (done 2026-09-25)** | As shipped, smaller than planned: `gsu_nmi_blob` (93 bytes, copied to `$7E:gsu_nmi_wram` at boot) is the `$0108` target; flag clear → `JML NmiHandler`; flag set → ack, `frame_count++`, OAM DMA if flagged, everything else deferred (dirty flags persist). Pads are NOT read under the GSU: edge detection would be corrupted (the next ROM NMI would copy a GSU-time sample into `pad_keysold` and lose the press). `gsuLaunch` sets / clears `gsu_owns_cart` and no longer writes `$4200` (the old `#$81` cancelled armed IRQs). Measured: `superfx_3d` `frame_count` 400 → 597 at frame 600. | `coproc_superfx_nmi.toml` (negative control: fails on phase A) | done |
| B (plan) | A WRAM-resident NMI path: `NmiHandlerGsu`, position-independent, copied to `$7E` at boot (the same mechanism as `gsuLaunch`'s stub, scaled up). Does only cart-free work: OAM DMA from `$7E`, joypad auto-read, `frame_count`, `vblank_flag` handshake, HDMA re-enable from WRAM tables. Skips the user callback and the audio pump while `gsu_owns_cart` is set; when clear, `JML`s to the full ROM handler. `$0108` points at it. `gsuLaunch` stops disabling NMI. | a new example `superfx_game_skeleton`: a GSU job spanning several frames while `frame_count` advances and a pad-driven sprite moves — manifest on `frame_count` delta and OAM; fbhash of the rendered frame unchanged | next |
| **C0 (done 2026-09-26)** | The IRQ, BRK and COP stubs, which still jumped into ROM: `gsu_irq_blob` (flag clear → `IrqHandler`; flag set → read `$4211` and SFR high, `rti`) and `gsu_rti_blob` share the WRAM copy (130 bytes of 160). `gsuSetProgram(const void *)` moved into the lib; `superfx_hello` drops its NMI-disabling launcher for `gsuLaunch`. | `coproc_superfx_nmi.toml`: superfx_3d arms a V-timer IRQ on line 200 — `irq_ticks` 396 at frame 600, `bus_violations` 0, `bus_vector_fetches` 788. Negative control (old `$010C` → ROM): CPU lost at PC `$84`, `frame_count` 27, 5446 bus violations | ✅ |
| **C** | Non-blocking launch: `gsuStart()` / `gsuBusy()` / `gsuWait()`, completion by **IRQ on STOP** (CFGR bit 7 clear, SFR bit 15 tells the GSU from the V-timer; manual §5.4.2 `a938cb6359382bbd`) through the `$010C` stub to a WRAM IRQ handler. `gsuLaunch` becomes start + wait. | manifest: IRQ count == jobs; `WaitForVBlank` still handshakes **first half done 2026-09-27, by another road**: a *cache-resident* job needs neither phase E nor the IRQ. Manual §6.1.2 (`3a7f008a1a412302`): a program running from the cache does not stop with RON = 0, so the CPU keeps the ROM. Shipped `gsuCacheLoad` / `gsuStartCached` / `gsuBusy` / `gsuWait` (lib `superfx.c`), fixture `devtools/libtests_gsu` + manifest `libtest_gsu_cached.toml`: 7 game frames during a ~7-frame job, 0 bus violations (149 340 with RON forced to 1). The job's last RAM write, lost in luna from the cache, was a luna bug (a stopped GSU no longer clocked its RAM buffer), fixed in v1.28.1; the manifest now asserts the $C0DE marker (fails on v1.28.0). **Second half done 2026-09-27**: IRQ on STOP through the `$010C` entry — `gsu_irq_blob` reads SFR high first, counts the GSU IRQ in `gsu_stop_irqs` and `rti`s, so it never reaches the user handler (before, IRQ unmasked + I clear = CPU stuck in the IRQ entry, reproduced on the fixture). No separate `gsuStart()` for ROM jobs: without phase E the main thread has nowhere to run while RON = 1. | ✅ |
| **D** | Presentation: `gsuPresent()` — split-frame double buffer (half the framebuffer per VBlank, `BG12NBA` swap only when both halves landed, SCBR toggle), driven from the WRAM NMI; SCMR height and depth as parameters. Stunt Race's MVN-to-`$7F` variant as the 30-fps reference. | fbhash at capture points; `luna diff` between the polled and the presented pipelines | **done 2026-09-29** — see §2b. `gsuPresentInit` / `gsuPresent` / `gsuPresentBusy` / `gsuPresentWait`, NMI step `gsu_present_step` (RAM window, both NMI paths + opt-in lag path), `gsu_scmr_live`, example `superfx_game_skeleton` (game loop 60 fps, 30 presented), oracle `vram_dma_blank.py`. One variant only (§2b: WRAM staging costs the GSU the same RAM time). Not done: using the bottom band (would need the lib to own a V-timer IRQ at the band's first line — conflicts with the user's IRQ). `superfx_3d` stays on `gsuDmaFullFrame` (53 fps for its light scene against 30-43 through gsuPresent). |
| **E** | Code in RAM as an SDK feature: a section stored in ROM, copied by the data-init loop, linked at its `$7E` address — asm first, then C functions marked `RAM_CODE`. Replaces the hand-copied blobs of B and C. Touches wlalink usage, `common.mk`, crt0, the RAM budget. | libtests vector calling a `RAM_CODE` function while `RON=1` | **asm half done 2026-09-29**: `RAM_CODE_SIZE` + `RAM_CODE_SECTION` (templates `ram_code_start/end.asm`, crt0 copy). Mechanism: a `.SECTION` with `BANK 1 SLOT 0 ORGA $10000-N BASE $7D FORCE` is stored at the top of ROM bank 1 and its labels resolve to `$7E` (1 + $7D); appended sections repeat `BASE $7D`; the source for the copy is `$01:` + the same 16-bit address on every map (HiROM through the `$01:8000` mirror). Fixture: a RON = 1 job waited on from the window, 33 frames, 0 violations; negative control in ROM loses the CPU. C half done 2026-09-29: `__ramcode` / `RAM_CODE` (cproc function specifier, QBE `APPENDTO ".ram_code"` + `.FAIL` guard), fixture's fourth job in C. B/C blobs migrated 2026-09-29 (crt0 `ram_code.gsu_interrupts`, lib `ram_code.gsu_launch`; `RAM_CODE_SDK` = 256 on Super FX builds, 235 used). (HiROM `[ramsections]` listing carrying BASE: fixed 2026-09-29, wla-dx `8077133`.) |
| **F** | C ↔ GSU contract: job table + `gsuCall`, symbol bridge (`wla-superfx` symbols → generated `.h` / `.inc`, ROM addresses in the GSU's linear view). | second GSU job in the skeleton | **done 2026-09-29, without the last part**: `<name>.sfx.h` generated by the `.sfx` rule (global labels → offsets), `gsuCall(entry)` (C, shifts `gsu_prog_addr`), fixture's `mul_job` run by `gsuCall` and `gsuStartCached`. Linking the GSU program at its real ROM address: **done 2026-10-03** by fixed placement — `GSU_BANK := n` assembles the `.sfx` at `$8000` (`memmap_gsu.inc`) and `GSU_SECTION` (`assets.inc`) forces the binary to `$n:8000`; the `.sfx.h` keeps offsets. Fixture `rom_job` (table read through ROMB / GETB + absolute jump) returns `$3CA5`; negative control: the same binary in an `ASSET_SECTION` lands at `$07:82A9` and returns 0. One `.sfx` per ROM in that mode. No `.inc` for asm yet (asm can use the label offsets the same way when asked). |

## 2b. Phase D design (2026-09-29)

**Starting point, measured** (luna profile, superfx_3d frames 100-400):
263 GSU jobs in 300 frames (~53 fps); `gsuDmaFullFrame` 36.5 % of CPU
time (poll for line 184, then one 16 KB DMA, CPU halted), the job 0.38
frame at worst. Every frame serialises job → poll → DMA; the GSU is idle
during the transfer and the CPU cannot compute the next frame.

**The mechanism: time-sharing Game Pak RAM.** Manual Book II §5.3
(`82e720ad547b984d`): the CPU writes RAN = 0 during a job, the GSU WAITs on
its next RAM access, the CPU accesses the RAM, RAN = 1 resumes the GSU. So
the NMI can DMA a finished framebuffer out of Game Pak RAM while the GSU
draws the next one in the other buffer.

- **Two framebuffers in Game Pak RAM** (`$70:0000` and `$70:size`, size
  from SCMR: 32 columns × H/8 tiles × 8·bpp bytes — 16 KB at 256×128×4bpp;
  fullsnes SCMR `725e8061d576404e`). luna maps 64 KB for real (probe:
  `$70:8000` is not a mirror).
- **Two char blocks in VRAM**; the tilemap is the same for both, only
  BG12NBA's BG1 nibble swaps — **after the whole frame landed**, never per
  chunk (the expert note's blinking bug).
- **`gsuPresent()`** (main thread): waits until the previous frame is on
  screen, queues the buffer the last job drew (`gsu_scbr`), flips
  `gsu_scbr`. The next job draws in the other buffer while the NMI moves
  this one.
- **The NMI step** (`gsu_present_step`, in the RAM code window, called by
  the ROM NMI when the main thread is parked in `WaitForVBlank` and by the
  WRAM NMI when a job owns the cart — never on a lag frame, the tilemap
  rule): read the V counter (STAT78 → SLHV → OPVCT twice, snesdev-wiki
  `1035792eed78d163`, `a3ca0260ef9cedac`), compute the lines left in the
  blank window, DMA `lines × 152` bytes at most on channel 7 (8 mclk/byte,
  1324 usable mclk/line: snesdev-wiki `f5a9e4b1de6e7045`; 152 leaves room
  for refresh and HDMA), with RAN revoked around the DMA. The window is the
  VBlank (37 lines NTSC), extended into the top letterbox of
  `gsuSetupHdmaBlanking` when there is one and the bottom band keeps force
  blank across line 0 (undisbeliever `b38b7898272dafdd`). No hardware
  multiplier in the NMI (the main thread may be mid-multiply).
- **`gsu_scmr_live`**, a shadow of the write-only SCMR, is what the step
  restores. Writers set it *before* taking the buses and clear it *before*
  giving them back, so an NMI in between never re-grants a stopped GSU the
  ROM.

**Why not Stunt Race's WRAM staging** (`MVN`/DMA `$70 → $7F`, then WRAM →
VRAM): the GSU loses the cart RAM for 8 mclk per byte either way; staging
only saves the second cart buffer, for 16 KB of WRAM and a copy that must
itself avoid VBlank. One variant.

**Throughput, measured** (skeleton, luna v1.30.0; the game loop at 60 fps
throughout): no letterbox 4 712 B per VBlank, 15 fps; 40/40 10 640 B, 30
fps; 76/4 16 112 B, 30; 84/4 the whole 16 KB in one VBlank, still 30 — the
job, which waits for Game Pak RAM while the previous frame moves (worst
135 000 clocks of work + 137 000 of waiting), is then the limit. PAL: 196
frames presented in 300. The expectation of 60 fps with an 80-line band
was wrong for a RAM-bound job.

**Found on the way, the bigger bug:** `gsuDmaFullFrame` read OPVCT once
per poll and never STAT78, so every other call read the high byte (PPU2
open bus = the previous exit value) and started at once: 481 866 of
1 359 872 bytes of `superfx_3d`'s framebuffer landed on visible lines. Its
"53 fps" were partly lost frames; fixed, it shows ~30 whole ones. The
corpus-wide `unsafe_writes = 0` check (`vram_dma_blank.py`) now guards the
class.

**Oracles (luna):** `--dma-trace` — every framebuffer byte written with
`blank || force_blank`, and its source/destination; `--dump-vram` — the
shown char block equals the last presented buffer; `luna frames` — no
half-landed frame shown; `gsu_pres_frames` for the rate; profile
`stalled` for what the GSU pays.

## 3. Facts the chantier stands on (arbitrated)

- Dummy vectors and dummy data (table 2-5-1, 2-5-2): manual Book II
  §5.4.1 — reference source; sneslab agrees.
- RON/RAN: the GSU WAITs without access; the CPU reads garbage (manual
  §5.3 `82e720ad547b984d`, sneslab). Cache execution needs no RON (manual
  §6.1.2 `3a7f008a1a412302`).
- STOP → IRQ, SFR bit 15, CFGR bit 7 mask: manual §5.4.2, §5.2.1.
- Game Pak RAM size at `$FFBD`, `$FFDA = $33`: snesdev-wiki, fullsnes
  (shipped in G5, 2026-09-24).
- Not arbitrated, from complement sources or measurement: the split-frame
  pipeline details (expert note), the MC1 store→STOP quirk (expert note),
  ~~FXPak Pro not running Super FX (by omission …)~~ — **withdrawn
  2026-09-26**: the sd2snes author's v1.10.3 notes (April 2019, cartouche
  `c94f64959972104e`) fix "SA-1 and SuperFX RAM write cycles" on Mk.II and
  Pro, so the cart runs a Super FX core. Hardware validation can use it,
  with the firmware version noted and a second reference before blaming
  the SDK (its core is a reimplementation).

## 4. Rules of the road

- `.claude/rules/nmi_audit.md` before every crt0 change (phase B and C are
  NMI changes: VBlank-critical order, DMA budget, handshake, DP isolation,
  the `$2180` port never in NMI).
- Every phase keeps the corpus at `diff_corpus` 85/85 MATCH; the two GSU
  examples are the only ROMs whose bytes change until phase B adds the
  skeleton.
- WRAM oracle drift is explained per phase (phase A: `.system` moved).
- Partner asks in flight: luna R1-R4 (`partners/luna/2026-09-24_to_luna_report.md`),
  snes-rag §2 (`partners/snes-rag/2026-09-24_to_snes-rag.md`). Phase B's
  manifest can only assert on WRAM until luna's `gsu` block exists.

## 5. Log

- 2026-09-29 — phase D. Measured before designing (superfx_3d: 53 fps,
  36.5 % of CPU in gsuDmaFullFrame). luna made every claim checkable:
  `--dma-trace` (blank / force_blank per byte, src, VRAM word) gave "no
  byte outside blank", "whole frames", "alternating blocks"; `--trace-writes
  210B` gave "swap only after a complete frame" (its rows carry `N` / `I`
  interrupt markers too — filter `kind == W`); negative controls: a window
  16 lines too long → 142 298 bytes outside blank from line 41; a swap per
  piece (the expert note's blinking bug) → "a frame of 10 640 bytes". The
  skeleton's first cut lost one game frame in three: gsuCacheLoad in C took
  105 000 mclk (a third of a frame) for 224 bytes → asm. Throughput table
  (skeleton): none 15 fps, 40/40 30, 76/4 30 (16 112 B, 272 short), 84/4
  30 (whole frame in one VBlank; the job, waiting for RAM during the
  transfer, is the limit). luna's `[asserts.dma]` refused past 1 000 000
  trace events, so Super FX examples were checked over 55 frames; luna
  v1.30.2 counts without storing the trace, 200 frames again.
  (Corrected later on 2026-09-29, before the release: the first write-up blamed the
  counter latch — "luna does not re-latch without STAT78". A probe showed
  luna re-latches on every SLHV read; a memory trace showed the real
  mechanism, OPVCT's read-twice flip-flop left on the high byte, whose
  bits 1-7 are PPU2 open bus: 1 281 reads in one poll, then 1 read
  returning the previous exit value `$B8`. The two partner items built on
  the wrong reading were withdrawn.)

- 2026-09-29 — phase E closed: the hand-copied blobs moved into the window.
  Found on the way: crt0 enabled the NMI before installing the `$0100`
  stubs (a VBlank in between would have jumped into empty RAM) — the GSU
  init now runs first. A label starting with `_` is section-local in
  wla-dx: `_gsu_wram_stub`, now in its own section, had to be renamed. The
  VM clock went back seven hours mid-session: every build after it needs a
  clean, or make skips targets dated in the future.

- 2026-09-29 — phase E, C half. cproc: `__ramcode` as a function
  specifier (the attribute route was closed: cproc parses GNU attributes
  into nothing but a few kinds). QBE hands the section name over with its
  quotes (`"\".ram_code\""`), like `.rodata` — the first build compared
  the bare name and emitted nothing. A tail call from ROM to the window is a
  `jml`, fine: the window function's `rtl` returns to the ROM caller's
  caller.

- 2026-09-29 — phase E, asm half. Two wla-dx facts found on the way: an
  empty section is dropped (both window markers carry a placeholder byte,
  as data_init_start does), and an APPENDTO section keeps its own BASE, not
  the target's. `symmap.py` counted wlalink's `RAM_USAGE_SLOT_2_BANK_126_*`
  markers (printed `00:ffc0`) as bank-$00 ROM: fixed, corpus figures
  unchanged. A first probe failure (`phk` + 16-bit `pla`) was the probe's.

- 2026-09-27 — phase C, second half: IRQ on STOP. Reproduced the latent
  lock first (fixture's second job, `gsu_cfgr = 0` + `irqEnable(IRQ_VTIMER)`:
  PC `$7E:205D` at frame 60, `r_done` 0), then the fix in the WRAM IRQ
  entry. Open, unsettled in fullsnes ("also set if IRQ masked?"): whether a
  masked STOP sets SFR bit 15; the doc tells users to keep the mask when they
  poll. `diff_corpus` 84/84; WRAM oracle re-captured for the two GSU
  examples (the blob grew, bank-0 code shifted, the vector stubs' targets
  and DP temps moved with it).

- 2026-09-27 — phase C (first half) through the code cache. The corpus
  settled the mechanism (manual §6.1.2, §6.8.4; fullsnes SCMR bits, CFGR
  bit 7; krom's GSUCACHEINJECT for the loading). `superfx_3d`'s program
  cannot use it (it has `CACHE`); the fixture's job has neither `CACHE`
  nor ROM reads. Found on the way: luna drops the last Game Pak RAM writes
  of a job run from the cache (same binary, same instruction count, full
  from ROM) — reported with a ROM pair. luna's answer the same day: its
  bug (the GSU stopped clocking its RAM buffer at STOP; ares keeps
  stepping), fixed in v1.28.1. Two of our observations were wrong: our
  variant script replaced a line that was not there, silently, so the
  "12 NOPs" and "STW $BEEF" runs were the unmodified program. Rerun on
  v1.28.0: 12 NOPs do drain the buffer; the extra STW loses only its own
  last byte. Lesson: every scripted source edit asserts it matched.

- 2026-09-24 — branch opened; phase A implemented and validated.
- 2026-09-26 — phase C0 (the IRQ half of the vectors) on luna v1.27.0,
  after merging develop (its crt0 had gained the 17th-bit pad read; the two
  merged without conflict). The first measurement read 400 frames: a stale
  lib/build, since the branch switch does not rebuild the lib — `make lib`
  first. FXPak claim of §3 withdrawn.
- 2026-09-25 — phase B. Measured the symptom before coding: one VBlank in
  three lost in superfx_3d. Scoped down on purpose: with a *blocking*
  launcher the main thread is parked in WRAM, so the only VBlank work that
  can happen under the GSU is ROM-free and handshake-free. Input and game
  logic during a job need the main thread to run while the GSU owns the
  cartridge — phase C (non-blocking launch) plus phase E (code in RAM), or
  a GSU program that runs from its cache with RON = 0.

## Open: a save for Super FX games (2026-10-02)

`USE_SRAM=1` with `USE_SUPERFX=1` is refused by the build (`412b15e3`): the
`sram` module writes bank $70, which on a Super FX cart is the GSU's Game Pak
RAM. A save needs its own design: header `$FFD6 = $15` (ROM+GSU+RAM+Battery,
fullsnes `b27db0e0b51e670e`), the save is the whole GSU RAM sized by `$FFBD`,
and the 65816 can only touch it while RAN is 0. luna v1.30.3 saves it
(`srm_out` / `srm_in`), so the power-cycle manifests are the test when this
opens. Source: luna's `2026-10-02_from_luna_v0.47.0-et-v1.30.4.md` §2.
