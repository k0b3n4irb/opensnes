# Chantier superfx-runtime — a CPU that keeps running while the GSU draws

**Status:** IN PROGRESS (opened 2026-09-24) on `wip/superfx-runtime`.
Squash-merge into develop as `feat(runtime,lib,build,examples,docs): …`,
MINOR release. **Catalogue entry:** `.claude/STRUCTURAL_DEFECTS.md` §E3.
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
| **B** | A WRAM-resident NMI path: `NmiHandlerGsu`, position-independent, copied to `$7E` at boot (the same mechanism as `gsuLaunch`'s stub, scaled up). Does only cart-free work: OAM DMA from `$7E`, joypad auto-read, `frame_count`, `vblank_flag` handshake, HDMA re-enable from WRAM tables. Skips the user callback and the audio pump while `gsu_owns_cart` is set; when clear, `JML`s to the full ROM handler. `$0108` points at it. `gsuLaunch` stops disabling NMI. | a new example `superfx_game_skeleton`: a GSU job spanning several frames while `frame_count` advances and a pad-driven sprite moves — manifest on `frame_count` delta and OAM; fbhash of the rendered frame unchanged | next |
| **C** | Non-blocking launch: `gsuStart()` / `gsuBusy()` / `gsuWait()`, completion by **IRQ on STOP** (CFGR bit 7 clear, SFR bit 15 tells the GSU from the V-timer; manual §5.4.2 `a938cb6359382bbd`) through the `$010C` stub to a WRAM IRQ handler. `gsuLaunch` becomes start + wait. | manifest: IRQ count == jobs; `WaitForVBlank` still handshakes | |
| **D** | Presentation: `gsuPresent()` — split-frame double buffer (half the framebuffer per VBlank, `BG12NBA` swap only when both halves landed, SCBR toggle), driven from the WRAM NMI; SCMR height and depth as parameters. Stunt Race's MVN-to-`$7F` variant as the 30-fps reference. | fbhash at capture points; `luna diff` between the polled and the presented pipelines | |
| **E** | Code in RAM as an SDK feature: a section stored in ROM, copied by the data-init loop, linked at its `$7E` address — asm first, then C functions marked `RAM_CODE`. Replaces the hand-copied blobs of B and C. Touches wlalink usage, `common.mk`, crt0, the RAM budget. | libtests vector calling a `RAM_CODE` function while `RON=1` | |
| **F** | C ↔ GSU contract: job table + `gsuCall`, symbol bridge (`wla-superfx` symbols → generated `.h` / `.inc`, ROM addresses in the GSU's linear view). | second GSU job in the skeleton | |

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
  FXPak Pro not running Super FX (by omission; hardware validation of this
  chantier needs a donor cart or the FX3 board).

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

- 2026-09-24 — branch opened; phase A implemented and validated.
