# luna stress-test campaign — running log

Goal (owner challenge, 2026-08-08): éprouver luna à son maximum — surface
weaknesses/gaps so luna becomes the project's ultimate, most reliable tool.
Each *validated* finding → a detailed issue on `k0b3n4irb/luna` (owner-validate
first, per `.claude/rules/luna_tooling.md`). Plan:
`~/.claude/plans/jolly-wiggling-pascal.md`.

Method per finding: repro (SDK micro-ROM) → observe via luna → oracle
(differential vs **Mesen2** MCP + fullsnes/anomie reference) → classify →
dedup vs open/closed luna issues → owner-validate → file. Pinned luna: **v1.13.0**.

## Wave 1 (2026-08-08)

- **Robustness gauntlet — PASS (no finding).** Empty / 1-byte / truncated /
  garbage (1K–64K) / text-as-ROM / missing file / forced garbage execution
  (LoROM/HiROM/SuperFX) / extreme `-n`. Zero panics, zero hangs, precise error
  messages, clean exit codes. luna is solid here.
- **luna#126 (input replay) — FIXED on v1.13.0 → CLOSED.** Filed on v1.9.0;
  re-tested both symptoms (boot-latch + periodic re-fire) on `apu_switch` with
  frame-exact repro. Both gone; checkpoint applied at the scheduled frame and
  held once. Posted verification repro + closed the issue.
  → *Downstream repo TODO*: re-enable the edge-count-sensitive asserts the
  harness disabled as #126 workarounds (`probes/apu_switch.py` cello→drums
  direction, `probes/soundboard.py` release-checkpoint window).
- **CPU multiply/divide accuracy — bit-exact (no finding).** `stress/hwmath`;
  incl. ÷0 quirk. luna == Mesen2 == reference, byte-for-byte.
- **PPU Mode 7 signed multiply — bit-exact (no finding).** `stress/ppumul`;
  signed 16×8 → signed 24-bit. luna == Mesen2 == reference.
  → **Promoted** hwmath + ppumul to a permanent luna-only regression probe
  (`probes/hw_math.py`, `luna --assert` on the verified blocks).

Key realisation: several harness-documented "gaps" were already shipped
(state `--input`+`--print-fbhash` #85, `--native-res` #115, `--force-mapper`
#95, `bench` freeze/dead-APU detector) or already fixed (#126, #107, #109).
The harness docs trail the pin (README says v1.1.0). luna is more mature than
its internal reputation.

## Wave 2 — open-bus / MDR (DONE 2026-08-08)

- **luna models open-bus / MDR correctly — PASS (no finding).** A C pointer
  read always hits bank $00 (structural limit), so the MDR bank byte can only
  be exercised from asm. `stress/openbus/ob.asm` reads the $2100 mirror through
  banks via `lda.l bb:2100`; luna returns the bank byte every time
  (`3F 01 20 10`), `$00` for the control, and the real value for a readable
  register — matching Mesen2 **byte-for-byte** and the fullsnes/anomie rules.
  The earlier "all $00" was the compiler's bank-$00 `lda.l` (where $00 *is* the
  correct MDR), not a luna gap.
  → **Promoted** to a luna-only regression (`probes/open_bus.py`).
- Only divergence in the whole probe: `$213F` STAT78 — luna reports PPU2
  (5C78) version **2**, Mesen2 **3**. Chip-revision modelling choice (real
  consoles ship rev 1/2/3), not a bug → **owner question, not filed**.

## Wave 3 — luna v1.14.0 "OpenSNES DX release" adoption (2026-08-08)

luna shipped #168–#181 in v1.14.0, explicitly built for this project (native
`luna test` runner, full CLI↔MCP parity, determinism oracles, debugging API
v2, JSON `--peek`). Their release asks OpenSNES to test three things.

- **Pin bumped v1.13.0 → v1.14.0.** Validated render- and behaviour-identical:
  coverage 81 OK/0 FAIL, visual 83/83 fbhash match, probes 19/19, WRAM 81/81.
  No baseline re-capture. Committed.
- **#181 acceptance — MET.** Ported `probes/hw_math.py` to a native
  `luna test` manifest (`stress/hwmath/hwmath.toml`, `[asserts.values]` on the
  div/mul result slots incl. the ÷0 quirk + `wdm_empty`). `luna test` returns
  the same verdict: PASS on correct values, FAIL (precise expected/got) on a
  tampered value, exit contract 0/1/2 confirmed. This proves the native runner
  can subsume the Python harness → path to deleting transitory code (luna-first).
- **#175 adopted.** `probes/lib.py:peek()` now reads the structured `peeks`
  array from the `--out` JSON (`{spec, space, addr, bytes_hex}`) instead of
  regex-scraping the stderr hexdump. Behaviour-identical — all 19 probes pass
  (symbol + BANK:OFFSET + signed-word paths). The stderr dump still exists but
  the JSON is the supported channel.
- **MCP debug session — validated (3rd ask done).** `stress/mcp_probe.py`
  drives `luna mcp` over stdio (newline-delimited JSON-RPC): handshake + the
  full 94-tool catalogue, serverInfo `luna v1.14.0` (#167 handshake fix). A
  real session on `apu_switch` exercised: `resolve_symbol`, `peek_memory`
  (by symbol), `run`/`step_until_frame`, symbol-annotated `call_stack`
  (WaitForVBlank JSL frame) and `cpu_trace`, memory search sessions
  (`search_begin u8` → `refine eq/changed`, `remaining` counts narrow),
  `freeze_add` (peek returns the frozen value across a run), and the
  determinism oracles — **`frame_hash` matches the CLI `--print-fbhash`
  byte-for-byte** (`714a220e2daaa1e4`) + `wram_page_hashes`.

All three v1.14.0 release asks are now done (#181 acceptance, #175 JSON peeks,
MCP debug session).

## Wave 4 — first probe migrated to `luna test` (2026-08-08)

Started the luna-first endgame (retire the Python harness). **Migrated the
`hw_math` probe** off Python onto native `luna test` manifests:

- `stress/hwmath/hwmath.toml` + `stress/ppumul/ppumul.toml` assert the
  multiply/divide + Mode 7 result slots via `[asserts.values]` (+ `wdm_empty`).
- New `make test-manifests` target (builds the stress ROMs, runs `luna test`,
  exit 0/1/2); wired into `make tests` after the probes.
- **Deleted `probes/hw_math.py`** — the checks now live in luna's own runner.
  Probe suite 19 → 18; the migrated coverage runs via `luna test` (2 passed).
- Fixed a stale comment in `ppumul/main.c` (32767×127 = 4161409 = 0x3F7F81,
  not 0x3F7F01 — luna's result was always correct; only the comment was wrong).

Pattern proven: a fixed-value-assert probe ports 1:1 to a manifest. Next
candidates are the other pure-assert probes (controller, dma_cgram slots);
probes with Python logic (RMS, directional, JSON state) stay Python for now.

## Wave 6 — v1.15.0 + migrate movement & dma_cgram (2026-08-09)

luna shipped **#205 in v1.15.0** (asserts v2: checkpoint/delta, thresholds,
blocks/spaces, trace-min, audio_rms_min) and **answered/fixed #207** (STAT78
→ PPU2 rev 3, via #209, lands next release). Bumped pin v1.14.0 → v1.15.0,
validated render/behaviour-identical (coverage 81/0, visual 83/83, WRAM 83/83).

Ported the two probes luna asked for (the #205 acceptance):
- **`movement.py` → 5 `[[checkpoint]]`/delta manifests** (aim_target, tiled,
  perspective, likemario, collision_demo) under `tools/luna-test/manifests/`.
  baseline checkpoint, then a leg holds a direction and asserts the delta
  (`increased`/`decreased`) vs the prior checkpoint. All pass.
- **`dma_cgram.py` → 2 `[asserts.blocks]` manifests** (VRAM font_tiles 144B,
  CGRAM bg_palette). All pass.
- Deleted both Python probes; suite 18 → 16; `make test-manifests` now runs
  9 manifests (2 stress + 7 example), all green.

**Second batch (same day):** migrated `map_scroll` (delta), `state_machine`
(2: dynamic_map `changed`, scene_stack push/pop with `width=1` — scene_top is a
1-byte counter with a noisy high byte), `open_bus` (`[asserts.blocks]` on the
`res` symbol, ROM built by the target), and `coproc`'s Super FX / SA-1 cases
(`[asserts.trace] {min=1}` + sa1_hello handshake via `[asserts.values]`).
`coproc.py` slimmed to the **firmware-gated DSP-1** check only (a manifest
can't express the `dsp1b.rom` skip). Probe suite **19 → 13**; `make
test-manifests` now runs **17 manifests**, all green. **Third batch (same day):** migrated `controller` (6 checkpoints, held button →
`[checkpoint.values] pad_keys`), `soundboard` (idle then A → play_count/
last_voice), `sprites_random` (random `delta changed` + simple_sprite OAM via
`[asserts.blocks]` on the `oamMemory` symbol). Probe suite **13 → 10**;
manifests **21** run, all green.

**Pinch found (filed):** `audio_rms_min` reads a **silent ring (RMS 0.0)** under
`luna test` for a ROM that is demonstrably playing (`state.apu.active_voices=5`,
`luna run --audio-out` RMS≈3968 at the same point). So `audio` and the RMS half
of `apu_switch` can't migrate yet — both stay Python. Also learned: for a
CPU-addressable WRAM block, the `[asserts.blocks]` key must be a symbol or
BANK:OFFSET (a bare offset with `space="wram"` is rejected); `space` is for
vram/cgram/oam/aram.

Still Python (by need): audio_v2 (DSP register file), dma_budget (DMA-budget
metric), mouse/superscope (peripheral input), sram (srm round-trip), vram_aram
(non-zero-count assert), audio + apu_switch (audio_rms_min pinch), coproc-dsp1
(firmware gate).

**Pinch reported to luna (as asked):** `[asserts.blocks]` keys the block by its
offset, so two spaces at the same offset (VRAM[0] and CGRAM[0]) collide as a
duplicate TOML key — dma_cgram needs two manifests instead of one. A free-label
key + explicit `offset` field is rejected ("invalid digit"). Suggest an
`offset` field or `[[asserts.blocks]]` array-of-tables. (Filed: see below.)

Note (not a bug): `--peek NAME:COUNT` parses COUNT as **hex** (documented for
BANK:OFFSET, applies to symbols too) — 144 → 0x144 = 324 bytes. Our
`probes/lib.py` sends decimal and truncates, so it over-reads harmlessly.

## Wave 5 — full MCP surface sweep (2026-08-08)

Answering "did you test everything v1.14.0 delivered?" — the earlier waves
covered the 3 asks + a representative slice, not the whole catalogue. So:

- **`stress/mcp_sweep.py`** calls **all 94 MCP tools** once, dependency-ordered
  (setup → action → observe → mutate → cleanup), schema-correct args:
  **94/94 OK, 0 errors, 0 not attempted.** Covers the untested surface —
  pokes (VRAM/CGRAM/OAM/ARAM/mem), all enable/take traces, breakpoints v2
  (add/list/set_enabled/remove/clear), symbols v2 (`load_symbols_str`,
  `clear_symbols`, `symbol_for_addr`, ARAM/SPC space), `wram_snapshot`,
  `loop_probe`, `render_*`, `decode_sprites`, `export_spc`, `sram_get/set`,
  `save_state`/`load_state`, `load_rom_bytes`, `set_port_device`,
  `set_cpu_register`, `run_until_pc/mem_read/mem_write/break`, etc.
- A first sweep flagged 6 "errors" — all self-inflicted: a malformed
  `load_symbols_str` text (`count:0`) **REPLACED** the symbol table and wiped
  `current_song`. Lesson recorded: `load_symbols_str` replaces, not appends.
- **#167 `search_memory` $7F fix verified**: poked `AB CD EF` into `$7F:1234`
  and searched — luna returns `0x7F1234` (correct), not the old bogus
  `$7E:1xxxx`. `poke_memory`/`peek_memory` handle bank `$7F` correctly too.

Net: the entire luna v1.14.0 delivery (94 MCP tools + the search_memory fix +
`luna test` + JSON peeks + the CLI surface exercised by probes) is validated.

## Does luna meet our needs? (assessment 2026-08-09)

Core needs — **met**: headless run/coverage/visual-regression/WRAM oracle,
native chips (SA-1/SuperFX/DSP-1), full MCP debug surface (94/94), `luna test`,
determinism oracles, DSP/APU visibility. Remaining needs:

1. **`luna test` assert expressiveness** — the manifest grammar (value/fbhash/
   log) can't express ~13 of our 18 probes (deltas/directional, thresholds,
   audio energy, trace-count, block/non-WRAM-space). Blocks retiring the Python
   harness. **Filed: luna#205** (the prototype = our probes = the spec).
2. **Cross-arch WRAM determinism — RESOLVED (no bug), 2026-08-09.** Ran a
   one-off CI matrix (x86_64 `ubuntu-latest` + arm64 `ubuntu-24.04-arm`) that
   `wram-trace`s the two formerly-excluded ROMs on the pinned luna v1.14.0:
   **CI x86_64 == CI arm64 == the committed aarch64 baseline, bit-identical**
   (mapandobjects `56571bbf3ba2f27d`, slope_collision `824adfbd24c4b613`). So
   there is NO cross-arch bug on v1.14.0 — the old exclusion was a stale/older-
   luna artifact. **Removed `CROSS_ARCH_EXCLUDE`** (now empty, kept as an escape
   hatch); the WRAM oracle now gates **83/83 on both arches** (was 81/81 + 2
   skipped). The pre-drafted cross-arch bug issue was **dropped** (nothing to
   file). Method note: GitHub gives free x86_64 *and* arm64 Linux runners, so
   cross-arch questions can be settled entirely in CI.

   Original characterisation (v1.14.0, aarch64) that made the CI check
   conclusive:
   - power-on WRAM = **all zero** (131072 B, 0 non-zero, hash `c74b47c8c74a2325`)
     → a zeroed Rust buffer is arch-independent;
   - **same-arch fully deterministic** (run-to-run identical);
   - active region = WRAM pages `0x0000-0x5FFF`.
   Logic: zeroed power-on + same-arch determinism + integer emulation ⇒ WRAM
   after N frames MUST be bit-identical cross-arch UNLESS luna has a
   host-dependent path (HashMap order / float / UB cast / uninitialised read in
   luna's own Rust). So **if the divergence still reproduces on v1.14.0 it is a
   genuine bug**, not uninitialised RAM. Can't confirm mono-arch — needs an x86
   run. aarch64 v1.14.0 stream fingerprints (`wram-trace -n 0 -c 90`):
   mapandobjects `56571bbf3ba2f27d`, slope_collision `824adfbd24c4b613`.
   Repro protocol: run the same on x86_64; differ ⇒ file (first differing line =
   frame,page); identical ⇒ the old note was a stale/version artifact, drop it.
3. **Audio *content* analysis** (melody/tempo) — the audio_analyze graveyard.
   We prove DSP registers, not "right notes". Hard; maybe not luna's job yet.

## Filed / closed on luna so far
- #126 — CLOSED (verified fixed on v1.13.0).
- #205 — CLOSED: richer `luna test` asserts — SHIPPED in luna v1.15.0.
- #207 — OPEN (question): STAT78 ($213F) reports 5C78/PPU2 version 2; Mesen2
  reports 3. Modelling choice, not a bug — asked whether it's intentional.
- #210 — OPEN (enh): [asserts.blocks] keys by offset; two spaces at the same
  offset can't share a manifest (dma_cgram pinch). Suggested offset field / array.
- #211 — OPEN (bug): audio_rms_min reads a silent ring (RMS 0.0) under luna
- #212 — OPEN (enh): remaining manifest-coverage gaps (peripheral input,
  DSP regs, footprint floor, DMA-budget, SRAM round-trip, firmware-skip) —
  the last 10 probes, each probe = the spec.
  test for a ROM that is playing (blocks audio + apu_switch migration).

## Resolved / no longer open
- Cross-arch WRAM determinism — RESOLVED, no bug (CI x86==arm on v1.14.0;
  CROSS_ARCH_EXCLUDE removed, WRAM gates 83/83 both arches). See above.
- Audio *content* analysis — parked (no reliable prototype = no sound spec).

## Nothing else fileable
Full campaign turned up no other defect: 94/94 MCP tools, robustness,
CPU/PPU math, open-bus all pass. Remaining "luna should own this" items
(WRAM-stream digest, DMA unsafe-write metric, `_sizeof_` via luna#77) are
marginal or folded into #205 — not worth standalone issues.

## Next
- Deeper waves: mid-scanline raster/HDMA timing; DSP/audio fidelity;
  SA-1 / Super FX contention.
- Standing takeaway so far: luna v1.13.0 passed every corner tested
  (robustness, CPU math, PPU Mode 7 math, open-bus/MDR). Findings to date are
  one closed bug (#126) + hard regression coverage, not defects.

## Wave 7 — v1.16.0 finale: harness retirement to 2 probes (2026-08-09)

luna v1.16.0 shipped #210/#211/#212 + #207 (STAT78→rev3) same-day. Bump caused
a benign corpus-wide drift (STAT78 folds into `rand_seed` via console.c → RNG
shift; only basics_random changed visually) — re-baselined from a clean rebuild
with justification. Then migrated the last-10:
- **Batch A**: audio (audio_rms_min #211 + sfx footprint), apu_switch (checkpoints
  + audio_rms_min), vram_aram ([asserts.footprint] ×4).
- **Batch B**: mouse, superscope ([[checkpoint]] mouse=/superscope= scripts),
  coproc/dsp1 (firmware gate + [asserts.trace]).
- **Batch C**: sram (srm_out/srm_in round-trip, 3 sorted manifests), audio_v2
  ([asserts.dsp] register file).
**Probe suite 19 → 2**; `make test-manifests` runs 34, all green.

Remaining 2 Python probes (both justified):
- **dma_budget** — luna's [asserts.dma] disagrees with the probe's classification
  (force-blank boot uploads counted in max_vblank_bytes; unsafe_writes count) →
  **filed #217**.
- **oam_struct** — decoded x/y/tile/count semantics; a raw OAM block golden is
  coarser. Candidate for a future [asserts.oam]. Kept Python.

## Filed this session (recap): #205✅ #207✅ #209✅ #210✅ #211✅ #212✅ (all shipped)
Open follow-ups: #217 ([asserts.dma] classification), #218 ([asserts.oam]
 decoded sprite fields — retires the last probe, oam_struct).

## Wave 8 — v1.17.0: ZERO PROBES, harness retired (2026-08-09)

luna v1.17.0 ("the zero-probe release") shipped #217 (`[asserts.dma]` now
buckets exactly like the probe — VRAM ports only, force-blank excluded; the
parallax_scroll "712 unsafe" were HDMA scroll-register writes, correctly
dropped) and #218 (`[asserts.oam]` decoded, proposed grammar verbatim). Bump
v1.16.0 → v1.17.0 clean (test-runner only; visual 83/83, WRAM 83/83, no drift).

Migrated the final two:
- **dma_budget** → 8 `[asserts.dma]` manifests (all pass — reconciled).
- **oam_struct** → 3 `[asserts.oam]` manifests (simple_sprite exact fields +
  visible count; metasprite/animated visible ≥ 1).

**Python functional probes: 19 → 0.** `make test-manifests` runs **45**, all
green. `probes/` retains only `lib.py` (used by `project_test.py`, the
user-facing `make test` — a legitimate keep per luna_tooling's orchestration
exception) and `run_all.py` (graceful 0/0 no-op + extension point). The
transitory Python harness is retired; `luna test` is the harness. luna-first
end state reached.

Campaign arc: luna v1.13.0 → v1.17.0 in ~2 days; issues #126,#205,#207,#209,
#210,#211,#212,#217,#218 all shipped; 19 probes → 45 native manifests.

## Wave 9 — post-retirement accuracy probe: decimal mode (2026-08-09)

Fresh corner (owner: "keep éprouver luna and the SDK"): 65816 **decimal-mode
(BCD) ADC/SBC** via `stress/bcd` (asm — the D flag isn't reachable from C).
7 cases incl. the invalid-nibble adjust (0A→10), 99+1 wrap, 50+50=BCD-100, and
the borrow (00−01→99). luna == Mesen2 **byte-for-byte** (values + carry + V/Z/N
flags: `102c...dec0`) == documented 65816. No finding — luna's decimal unit is
correct and the toolchain assembled/ran it fine. Locked as a luna-only
regression (`bcd.toml`); test-manifests now 46. No new luna request warranted.

## Wave 10 — sprite-per-line overflow (2026-08-09)
40 small sprites on one scanline (`stress/sprite_overflow`): STAT77 range-over
(bit6) sets, time-over (bit7) correctly does NOT (range caps eval at 32 → 32
slivers ≤ 34). luna == Mesen2 byte-for-byte (`0141414141414141`) == documented
range/time interaction. No finding — luna's OBJ evaluation and the SDK's
oamSet+NMI-OAM-DMA path both correct. Locked; test-manifests now 47.

## Observation pending owner validation (2026-09-03, reduced 2026-09-05, luna v1.17.0)

- **`--input` is ignored when the run ends with `--until-frame`.** Minimal
  repro on a stock corpus ROM (mode7/perspective: D-pad Right increments `sx`
  every frame, `pad0` mirrors `padHeld(0)`):

  ```
  luna state --until-frame 130 --input "10:0x0100" --out - \
      --peek pad0:2 --peek sx:2 --peek frame_count:2 perspective.sfc
      → pad0=0000 sx=0000 frame_count=0081   (130 frames ran, pad never latched)
  luna state -n 3000000 --input "10:0x0100,60:0" --out - … perspective.sfc
      → pad0=0000 sx=0032 frame_count=004A   (50 frames of Right, released at 60)
  ```

  Same with the checkpoint at frame 2 and with `--until-frame 400`: the
  `--input` script never applies under `--until-frame`, while `-n` honours
  it (checkpoint AND release). Expected: the frame-latched checkpoints apply
  regardless of how the run is bounded — `--until-frame` is exactly the flag
  the help text sells for "input→assert probes land on an exact frame".
  Workaround in the harness: keep `-n` for scripted-input runs (what
  `probes/lib.py` and the manifests do). Owner to validate, then file on
  `k0b3n4irb/luna` with the repro above.
