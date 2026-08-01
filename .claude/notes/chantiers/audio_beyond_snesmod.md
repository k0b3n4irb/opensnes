# Chantier — Beyond SNESMOD: an OpenSNES-owned music engine

Status: PLANNING (2026-07-27). The **larger** of the two frontier projects
(the other is [`examples_reorg_by_usecase.md`](examples_reorg_by_usecase.md));
open it **after** the examples reorg. This version incorporates an expert
code-grounded deep-dive (2026-07-27).

## Goal

Retire the last big PVSnesLib dependency — **SNESMOD** (the SPC700 module
player + the `smconv` IT→SPC converter) — and replace it with an
OpenSNES-owned **music/sequencer engine + converter**, then surpass it on
footprint, tooling, and (above all) *maintainability*. This is the milestone
that makes "we surpassed the father" concrete — measured, not asserted.

## 1. The real debt (code-grounded)

**OpenSNES already owns the entire chip + voice layer; SNESMOD's *only*
remaining exclusive contribution is the sequencer — and that sequencer
exists in this repo only as an opaque 5522-byte binary blob with no source.**

- `lib/source/sm_spc.asm` is `.byte $CD,$00,$E8,…` hex — **not** source.
  `tools/smconv/src/spc_program.h` is the *same* blob as a 5522-entry C
  array. There is no SPC700 assembly source for the tracker anywhere in the
  tree. OpenSNES cannot read, fix, extend, or luna-trace one instruction of
  it. **That is the debt** — not "we lack a sequencer."

### What audio v2 already owns (all live, all luna-verified)

| Layer | Where | Detail |
|---|---|---|
| C command API | `lib/source/audio.c` (457 l., cc65816 C) | Full 22-fn `audio.h` surface |
| Resident SPC driver | `lib/source/audio_driver.spc700.asm` (447 l. → **627-byte** blob, built from source via `wla-spc700`) | 16 opcodes, reactive poll loop, hot-swap |
| Voices | driver `cmd_kon/koff/vvol/vpitch/vadsr/vgain` | 8 hw voices: SRCN, VOL L/R, 14-bit PITCH, ADSR1/2, GAIN, KON/KOFF |
| Master + echo | `cmd_mvol/echo_cfg/echo_fir/echo_on` | MVOL, ESA `$C000`, EDL, EFB, EVOL, 8-tap FIR, EON |
| Dynamic BRR | `cmd_load*` + `audioLoadSample` | IPL-shaped stream to ARAM; 64-entry dir at `$0A00`; bump alloc + LIFO reclaim |
| Voice allocation | `audioPlaySampleEx` | round-robin `audio_rr_voice` + ENVX-guided free-voice preference |
| State readback | `cmd_envx` | the single DSP→CPU path (`VxENVX`) |

audio-v2 ARAM map: driver `$0200–09FF` (2 KB enforced, 627 B used); dir
`$0A00`; samples `$0B00→$C000` (~46 KB); echo ring `$C000–$F7FF`.

**Decisive enabler — the regression harness already exists.**
`probes/audio_v2.py` does `luna spc-dump` → asserts the DSP register file
directly (MVOL, VxVOL/PITCH/ADSR/GAIN, DIR page, BRR bytes, echo
ESA/EDL/EFB/EVOL/FIR/EON/FLG). The **same method verifies every sequencer
capability below** — a harness SNESMOD never had.

### The exact gap SNESMOD fills — the sequencer, and nothing else

`lib/source/snesmod.asm` (1220 l.) is **not** the sequencer — it is 65816
glue (C wrappers, a 256-byte FIFO, soundbank/bank management, per-frame
streaming). The tracker runs entirely inside the blob. What OpenSNES does
**not** own: order list, patterns/rows (IT-compressed), instruments
(envelopes/fadeout/sample-map), effect columns (arpeggio/porta/vibrato/
volslide/…), **tempo/BPM tick** (v2's driver runs *no timer at all* — the
single biggest thing to add), note→pitch table, per-channel state,
channel↔voice mapping, SFX priority/stealing, volume fade.

**So the milestone is well-scoped:** add a timer-ticked sequencer state
machine + note/effect processor to the resident driver, plus a converter
and a compiled format — reusing v2's voice/echo/BRR machinery verbatim.

## 2. Music-format decision — **Option B**

*IT as the authoring format, an OpenSNES-owned compiled module format
(`.osm`), an offline converter.*

| Option | Authoring | ARAM | Control | On-console cost |
|---|---|---|---|---|
| A. Parse IT/XM on-console | best | **poor** | low | high (parser resident) |
| **B. Own compiled format, author in IT, offline convert** | **best** | **best** | **highest** | **low** |
| C. New authoring format + editor | worst | best | high (must ship an editor) | low |

Rationale: this is **SNESMOD's winning strategy — keep the strategy, own the
artifacts** (SNESMOD's flaw was never the approach; it's that OpenSNES owns
none of the three artifacts). Reject on-console IT parsing — ARAM is the
scarcest resource, compile offline. Reject a bespoke format — the SNES
chiptune world lives in Impulse Tracker/OpenMPT; inventing a format means
shipping an editor. IT specifically (not XM/MOD): matches OpenMPT + SNESMOD,
existing example `.it` convert, instrument/envelope model maps to S-DSP.

**`.osm` sketch**: ROM-side header (magic, orders, pattern-offset table,
instrument table {sample_id, ADSR1/2, fadeout, env nodes}, channel vol/pan,
echo block) + ARAM payload (BRR bank in v2's directory format + row-packed
compiled pattern blob). **Reuse IT's row/mask compression — the algorithm is
already in `it2spc.c:260–376`; port it, don't reinvent.** Note→pitch: resident
12-entry semitone LUT + octave shift (per-sample c5speed folds in, as
`it2spc.c` already does); DSP PITCH write reuses `cmd_vpitch`.

## 3. Sequencer architecture — extend the existing driver

Do **not** fork a second driver. The sequencer is a *superset* of the v2
command engine: v2's reactive loop keeps running (SFX + live C control), a
**new timer-driven tick** advances music underneath it.

- **Tick engine (the one genuinely new mechanism):** SPC700 **Timer 0**,
  fractional accumulator so non-integer tick periods stay accurate (SNESMOD's
  approach, now in *our* source). Main loop: `APU_CHECK_RESET` → poll `T0OUT`
  → run `music_tick` × pending → service one CPU command (v2 dispatch) → loop.
  **v2's C API keeps working while music plays.**
- **Per-tick state machine:** per channel, on new row fetch cell → apply
  note (pitch from note+instrument, SRCN/PITCH/KON), instrument (ADSR, reset
  envelope), volume column (VxVOL, pan from channel_pan[]), latch effect;
  every tick run effect update (arp/porta/vibrato/volslide) → PITCH/VOL;
  advance tick→row→order. Channel state in zp/low ARAM. **Budget ≤ 4 KB**
  (vs SNESMOD's 5522 B → reclaims ~1.5 KB of sample ARAM).
- **Effect set (min bar), each behind its own probe:** arpeggio, porta
  up/down + tone-porta, vibrato, volume slide, set-vol/volcol, set-speed/
  tempo, position-jump/pattern-break, volume fade. Wave 2: tremolo, sample
  offset, note cut/delay, retrigger. Everything else → converter rejects loud.
- **Note-off:** prefer ADSR **release** (KOFF bit) over hard KOFF — the
  `apu_switch` lesson (`spc700_arc.md:86`: hard KOFF truncates audibly).
- **Voice allocation — music ⇄ SFX (the real design problem):** static split
  + **priority stealing** (SNESMOD's model made explicit). Each voice tagged
  `MUSIC`/`SFX`/`FREE` + priority. `audioPlaySampleEx` picks FREE → lowest-ENVX
  SFX → lowest-priority MUSIC voice; on SFX end (ENVX→0) it reverts to MUSIC
  and the sequencer re-keys it. **v2 already has the primitives**
  (`audio_rr_voice`, ENVX-guided alloc, WRAM voice mirror) — extend, don't
  invent. `audio.h` (SFX) + new `music.h` share the one resident driver; the
  "one engine per ROM" rule persists only against the raw `apu` path + deleted
  snesmod.

## 4. Converter `osmconv` (replaces `smconv`)

Reuse the good, ownable parts of `smconv`: `itloader.c` (IT parse),
`brr.c` (BRR encode), the `it2spc.c` pattern compression. **Discard**
`spc_program.h` (the blob) and the SNESMOD-ARAM-layout soundbank emission.

```
osmconv [-v] -o <name> music.it [sfx.it ...]  ->  <name>.osm.asm + <name>.h
```

**Fail-loud discipline** (`gfx4snes -c` / `tmx2snes` house style):
- **Reject unsupported effects by name + location:** `error: pattern 3, row
  12, channel 5: effect 'Yxy' not supported (supported: Jxy Exx Fxx Gxx Hxy
  Dxy …). Remove it in OpenMPT.` Never silently drop.
- **ARAM budget, computed + named:** `error: 51.2 KB samples + 8.1 KB
  patterns + 14 KB echo exceeds ARAM by 6.9 KB. Reduce sample rate or echo
  delay.` (bank-0-ratchet philosophy for ARAM.)
- Validate the IT subset (>8 channels → error; unsupported NNA/envelopes →
  named warn+degrade). `--dump` mode for luna cross-checks. Converter unit
  test round-tripping a fixture `.it` (like `tools/smconv/tests/`).

## 5. Probe-first phased plan

Each capability behind a `luna spc-dump` DSP/ARAM assertion before the next.
New `probes/music.py` + a deterministic fixture ROM per phase.

| Phase | Capability | luna assertion |
|---|---|---|
| P0 | Format + upload one instrument, trigger one note | V0SRCN, V0PITCH, KON bit0, ENVX>0; pattern blob at ARAM |
| P1 | Order/pattern/row advance + **timer tick** | V0PITCH differs row a↔b; T0 programmed; `music_row` readback advances |
| P2 | Instruments: multi-sample, ADSR, vol envelope | VxADSR match table; ENVX decays between captures |
| P3 | Volume column + note-off/cut | VxVOL match volcol×pan; KOFF/release after note-off |
| P4 | Effects (arp, porta, vibrato, volslide) | multi-capture in one row: PITCH steps/ramps/oscillates; VOL ramps |
| P5 | Multi-channel (≤8 voices) | all N KON set; each VxSRCN/PITCH/VOL correct; no bleed |
| P6 | SFX ⇄ music coexistence + stealing | stolen voice SRCN→SFX then reverts; `voice_owner[]` mirror correct |
| P7 | Echo + master fade polish | EON/EVOL/EDL/FIR match module; MVOL walks down on fade |

Interactive owner listening pass at P4/P6/P7 (audible-quality gates).

## 6. "Surpass" metrics — the bar, set before building

Baselines from the repo (2026-07-27):

| Axis | SNESMOD (measured) | Target |
|---|---|---|
| Channels | 8 hw voices + 1 streamed digi (`snesmod.asm`) | 8 hw voices w/ explicit priority (8 is a DSP hard limit) |
| **Driver ARAM** | **5522 B** (`sm_spc.asm` `$1592`) | **≤ 4096 B** → +1.4 KB samples |
| Sample ARAM | ~46 KB (`SPC_RAM_SIZE=58000`) | **≥ 48 KB** (from the smaller driver) |
| Effect coverage | IT subset in an unverifiable blob | documented subset, per-effect probe-verified, named-error on the rest |
| CPU/frame | `snesmodProcess` streams a FIFO on the 65816 each frame, unmeasured | measure ours; sequencer resident on SPC → 65816 does *less*, structurally cheaper |
| **Maintainability** | **zero** — 5522-B hex blob, no source, no tests | **full SPC700 source (`wla-spc700`), luna DSP regression on every capability** |
| Tooling errors | inherited `smconv`, silent drops | fail-loud, effect-named, ARAM-budget-named |

**Honest framing:** you cannot "surpass" 8 hardware voices without software
mixing (which the DSP cost model punishes and v2's non-goals reject). The
surpass story is: *same voice count, smaller driver, more sample RAM, fully
owned & source-maintained, luna-regression-tested, provable effect parity,
better tooling.* Measured, not asserted.

## 7. Retirement plan

**Dependents are broader than assumed: `USE_SNESMOD=1` is in 6 examples**,
not 4 — the four `audio/snesmod_*` **plus `games/tetris` and
`games/likemario`** (full games shipping music via SNESMOD; the
migration-risk items). Do not delete SNESMOD until both are ported + pass
their probes.

1. Land the new engine + `osmconv` + `music.h` (module `music`, requires
   `apu`+`audio`), P0–P7 green. SNESMOD stays alive in parallel (separate
   modules).
2. Migrate examples one at a time (reconvert `.it`, swap calls), games last;
   each a validated commit (probe + listening).
3. Delete `snesmod.asm`, `sm_spc.asm`, `snesmod.h`, `tools/smconv/` (fold
   `itloader.c`/`brr.c`/pattern logic into `osmconv` if reused; delete
   `spc_program.h` unconditionally).
4. `make/common.mk`: replace `USE_SNESMOD`/`SMCONV`/`SOUNDBANK_*` wiring with
   `osmconv`/`.osm`. The HiROM `.ORG $8000` soundbank sed hack disappears —
   our loader uses far pointers (v2's `apuUpload` honours the bank byte).
5. **`ATTRIBUTION.md`**: the SNESMOD entry must *change, not vanish* — credit
   Mukunda Johnson for design inspiration + the IT authoring workflow;
   OpenSNES engine an independent from-source implementation; keep the IT
   format acknowledgement, and the zlib attribution for any reused
   `brr.c`/`itloader.c` files.
6. Docs/counts: audio tutorial, `KNOWN_LIMITATIONS.md`, examples README,
   `make lint-docs`. The new SPC engine is `wla-spc700` (exempt from ASM ABI
   lint like `audio_driver.spc700.asm`); its C layer is cc65816-clean.

**Sequencing:** do the examples reorg first (faster payoff), then this — and
the reorg may relocate the 6 SNESMOD dependents, so reorg-first also avoids
double-migrating those ROMs.

## Corrections to the original draft (for the record)

- SFX/music coexistence was flagged "open" — the code shows v2 already has
  the primitives; the plan extends them.
- Retirement was scoped to `snesmod_*` — **`games/tetris` and
  `games/likemario` also depend on SNESMOD** and are the harder migrations.
- The under-stated crux: SNESMOD's sequencer is an **opaque 5522-byte blob
  with no source**. The chantier's true deliverable is *owning that source*;
  the "surpass" bar is dominated by maintainability + a smaller driver +
  probe coverage, not channel count (hardware-capped at 8).

## Prior art to read at kickoff

`spc700_arc.md` (#119 raw-APU path), `audio_v2.md` (the shipped v2 engine),
`.claude/notes/tech/audio_legacy_pvsneslib_abi.md`, `lib/source/audio.c`,
`lib/source/audio_driver.spc700.asm`, `lib/source/snesmod.asm`,
`tools/smconv/`, `probes/audio_v2.py`.
