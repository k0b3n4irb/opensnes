# Tracking — the 42 actions of the 2026-09-26 review, and what is left (2026-10-03)

The review (`reviews/2026-09-26_etat_des_lieux.md`) ended with 42 numbered
actions and no tracking table; this note is that table, built on 2026-10-03
by matching each action against `git log --since=2026-09-26`, the CHANGELOG
and the notes. **26 done, 11 partly, 2 not started, 3 wait on the owner** on the day of the inventory; rows 9, 12 and 28 were closed the same day (29 done, 8 partly).
Update the row when an action moves; do not rebuild the inventory.

The global picture and the order of work are in the plan approved the same
day (summarised at the end of this note).

## The 42 actions

| # | Action | Sev | State | Evidence / what is left |
|---|---|---|---|---|
| 1 | Release zip ships devtools, CI smoke test from the zip | 🔴 | done | `f984cfe4`, `6508304e` |
| 2 | Reviewer agent fixed, retired-tools anchor, audit agents versioned | 🔴 | done | `b55494ee`, `2c2d6905`, `f0bc8ed2` |
| 3 | luna bump, reply to the 09-25 note, golden queries | 🟠 | done | `f7367cdf`, `012c11d6`; pin now v1.31.0 (`75599363`) |
| 4 | Wrong API lines in docs | 🟠 | done | `2c2d6905` |
| 5 | FXPak Pro claim corrected | 🟠 | done | `b55494ee`, `2c2d6905` |
| 6 | IRQ/BRK/COP stubs in WRAM + manifest | 🟠 | done | `7e7fa817`, `20638bc3` |
| 7 | `test-pal` builds its fixture; scheduled workflows on develop | 🟠 | done | `f7367cdf` |
| 8 | Build guards (NMI lint, config stamp, headers, `LIB_MODULES`, smconv IMPM) | 🟠 | done | `412b15e3`, `3db1a9fb` |
| 9 | `BENCHMARK.md` regenerated, ROADMAP and `ABI.md` fixed | 🟠 | done | `2c2d6905`; the table is anchored in `check_doc_drift.py` since 2026-10-03 (anchor 13) |
| 10 | `padIsConnected` validity flag | 🟠 | done | `2caa6894` |
| 11 | `gsuSetProgram` in the lib; Mesen2 out of chip sources | 🟠 | done | `7e7fa817` |
| 12 | `nmi_budget.py --report`, per-example baseline | 🟡 | done | `f7367cdf`, `6fcb10e1`; `superfx_3d`, the skeleton and `gsu_present_step` added 2026-10-03 (9 rows) |
| 13 | `OPEN_luna.md`, `OPEN_snes-rag.md`, corpus note | 🟠 | done | `b55494ee`, `012c11d6`; both lists empty on 2026-10-03 |
| 14 | `ATTRIBUTION.md`: cmdparser, stb_image, cute_tiled, LodePNG | 🟠 | done | `f984cfe4` |
| 15 | Stale example comments, `port-example/SKILL.md` | 🟠 | done | `38c4b7ed` |
| 16 | Governance cleanup | 🟡 | done | `e9ecf489`, `cdba4544` |
| 17 | Replace Nintendo / PVSnesLib assets, `docs/ASSET_PROVENANCE.md` | 🔴 | **owner** | deferred by the owner on 2026-09-26 (`06930df8`); 66 files in 23 examples; v1.0 must-have |
| 18 | D1-D5, freeze criteria, ROADMAP v1.0 table | 🟠 | **decided and applied 2026-10-03**, struct variants included (`dsp1SetCamera`, `oamDrawMetasprite`); what is left is for 1.0: remove the aliases, bring `hdmaEnable(channel)` back | all rows as recommended (table at the top of the sheet); application: one row per commit, D4 first |
| 19 | `check_doc_drift.py` extended (a-e) | 🟠 | done | `2c2d6905`, `8ed23df4`, `fbbacace`, `295d16f7` |
| 20 | Compiler invariant on Kl defs | 🟠 | done | `6ecb255a` |
| 21 | Constant multiply inlined | 🟠 | done | `e01ffa29`, `6d62c443` |
| 22 | Breakout: game-over loop, levels, one SFX, manifest | 🟠 | partly | `f8db2a61` (loop, manifest). Left: the SFX, levels really used |
| 23 | WRAM oracle excludes the stack band | 🟠 | done | `b106ad43` |
| 24 | `install-luna.sh` multi-OS, a CI leg that runs ROMs | 🟠 | done | `7dbdaea7`, `b8e2be4f`, `c22be86a` |
| 25 | SA-1 BW-RAM saves; `sa1.h` and the speed claim | 🟠 | done | `621cd9f9`, `f51786d1`, `d2b4bd51`, `b2a08f27` |
| 26 | Lib: structs for >= 4-argument functions, dead API deprecated, hot setters, `sram.asm` | 🟠 | partly | done 2026-10-03: dead API deprecated (`a775e7e9`), struct forms for the two native functions past five per-call arguments (`dsp1SetCamera`, `oamDrawMetasprite`). Left: hot setters, `sram.asm` |
| 27 | Window and `hdma_wave` examples on the lib | 🟠 | done | `007c92ce`, `855bf2c7` |
| 28 | Per-API frame costs in `docs/PERF.md`, cited in headers | 🟠 | done | `52e807d6`; the ten measured functions cite the page and the page is in the generated docs since 2026-10-03 |
| 29 | Header dependencies for lib and examples | 🟠 | done | `d2718a43`, `412b15e3`, `3db1a9fb` |
| 30 | Richer GSU manifests, `libtests_gsu` | 🟡 | partly | `f7367cdf`, fixture and manifest exist. Left: `scmr_ron`, `--gsu-pc-set`, sha cache |
| 31 | Compile-only fixtures checked, runtime `u32 f(void)` | 🟡 | done | `5aa7868c`, `01bb42cf`, `f143c6c7`, `e428d940` |
| 32 | Two-snapshot liveness, scripted pads, audio covered | 🟡 | done | `18ed12f4`, `a6fabb57`, `b12a752a`, `d2b4bd51`, `114993e3` |
| 33 | First console session | 🟠 | not started | **owner** (hardware). `docs/HARDWARE_VERIFICATION.md`: "No session recorded yet"; row 23 added (`841385db`) |
| 34 | Super FX phases C, E, D, F | 🟠 | done | phases A-F merged (catalogue E3 resolved). GSU program linked at its real ROM address (`GSU_BANK`) and the Super FX save (`USE_SRAM` + `USE_SUPERFX`): both done 2026-10-03 |
| 35 | Showcase game: version it or drop the line | 🟠 | **owner** | ROADMAP says it moved to its own repository (`06930df8`) |
| 36 | Stack pressure: slot coalescing, no frame over 256 bytes | 🟠 | partly | `c1631377` (518 -> 214). Left: the `sta/lda N,s` pair removal, compare-to-branch fusion |
| 37 | Fork resync plan and upstreaming | 🟡 | partly | counts tracked (`f143c6c7`, `f1df92ba`); nothing upstreamed (catalogue A5) |
| 38 | Orphan functions documented, "chantier" jargon out of public docs | 🟡 | done | `0e11c91d`, `2c2d6905`; jargon removed 2026-10-03 (this commit) |
| 39 | Coverage gaps (Mode 4/6, pseudo-hires, EXTBG, PAL), three example merges | 🟡 | partly | `2997d2c2`, `ecba7387`, `9c34b8cd`, `ff665c5c`, `e6651016`, `7b776e0b`. Left: the three merges, to do or to drop in writing |
| 40 | Differential compiler test, host vs luna | 🟡 | partly | only `01bb42cf` (32-bit-return vectors); no host-vs-luna tool |
| 41 | Fuzz on `tools/**` and weekly, replay on push | 🟡 | done | `7ce057b8`, `ca326132` |
| 42 | Bus factor: `CODEOWNERS`, takeover page | 🔴 | partly | `a4e95efa`; the habit half is not measurable, still one maintainer |

## Found since the review, still open

| Item | Where |
|---|---|
| ~~`snesmodInit` writes `$81` to NMITIMEN~~ fixed 2026-10-03 (restores `nmitimen_shadow`; `r_irq` 0 -> 10) | `chantiers/snesmod_65816_side.md` |
| ~~IT 2.15 detection to re-read~~ settled 2026-10-03 on the raw OpenMPT and Schism sources: ours is right, modlib differs (owner's call whether to tell Mukunda). New small gap: uncompressed delta PCM (`Cvt` bit 2 without compression) is read as plain PCM | `partners/snesmod-upstream/README.md` |
| `fixSqrt` stays at 4 fractional bits by choice (one 16-bit root, ~80 cycles; 8 bits need a 32-bit root). Comment and tutorial say so since 2026-10-03; widen only if a user needs it | `lib/source/math_sqrt.c` |
| "The C RAM budget does not know the stack"; `check_lib_rodata.py` "looks stale" (owner call) | `status/api_audit_findings.md` |
| Five issues at `mukunda-/snesmod` (#6-#10), weekly check, PVSnesLib after his answer | `partners/snesmod-upstream/README.md`, next check 2026-10-10 |
| Console photos owed to luna and snes-rag: empty port trace, Mode 6 bit 3 card | `docs/HARDWARE_VERIFICATION.md` rows 14 and 23 |

## The freeze criteria (sheet `2026-09-26_fiche_decision_D1-D5.md`)

| # | Criterion | State on 2026-10-03 |
|---|---|---|
| 1 | D1-D5 and associated decisions landed | **landed 2026-10-03** (`05e86951` to the metasprite commit); alias removal and `hdmaEnable(channel)` are the 1.0 release itself |
| 2 | No open red except the PVSnesLib assets | **not held** (re-read 2026-10-04): the 09-26 reds are closed, but the 2026-10-03 audit opened new ones that only the owner can close — the Nintendo assets without attribution (F PF1), the three undated 1.0 gates (H PF14), a bus factor of 1 (H PF19); the one about deprecations without clang is closed (`6a583912`) |
| 3 | Release zip built and tested by CI | done |
| 4 | Drift sentinel covers function names cited in docs | done |
| 5 | A console session filled rows 1 to 7 | open (owner, hardware) |
| 6 | Super FX finished or bounded as experimental | **finished 2026-10-03**: `GSU_BANK` (`26856d8a`) and the save |
| 7 | Two weeks without discovering a silent defect (reworded 2026-10-03) | hunting campaign open; the fortnight starts when it closes — `status/silent_defects_log.md` |

## Order of work agreed on 2026-10-03

1. Stale notes brought up to date and this table (done with this commit),
   then the small leftovers: 12, 9, 28, the SNESMOD items, `fixSqrt`.
2. Owner session on D1-D5 and the three associated decisions, D4 first (the
   only row no alias can fix after the freeze); then one row per commit,
   and action 26.
3. Release v0.48.0 on the owner's go: everything under `CHANGELOG.md`
   `[Unreleased]` (the count moves daily; the silent-defect fixes are in
   `status/silent_defects_log.md`).
4. Criterion 6: finish or bound the Super FX (action 34).
5. Console session when the hardware is at hand.
6. Then one large chantier, not several: the OpenSNES music engine
   (`chantiers/audio_beyond_snesmod.md`) is the recommendation; the
   alternative is to reach the 1.0 freeze first and keep SNESMOD, now fixed.

Large chantiers not started: the music engine (P0-P7), the example authoring
backlog (`chantiers/examples_reorg_by_usecase.md`, 15 examples), the
compiler forks' upstreaming (catalogue A5: 86 QBE / 32 cproc / 4 wla-dx
patches), streaming audio.

CI: develop green at `c7d58b8a` on 2026-10-04 (build, lint, sanitizers);
the sanitizer job had been red from `c3952a1e` to `e1db1ab2` on the cproc
suite ratchet. Weekly PAL, fuzz, nightly luna bench and the monthly MSYS2
diagnostic green on 2026-10-03.
