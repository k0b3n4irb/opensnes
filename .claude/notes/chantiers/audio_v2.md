# Audio System v2 — replace audio.asm on the apu foundation

Status: COMPLETE — all 4 phases shipped 2026-07-19 (driver+boot,
sample pipeline, echo+ENVX, soundboard example + audio.asm retirement).
Decision record for the chantier
that retires `lib/source/audio.asm` (legacy PVSnesLib ABI, zero
callers, the ABI lint's only skip-file — see
`.claude/notes/tech/audio_legacy_pvsneslib_abi.md`, resolution
"Path E": rebuild on the raw-APU path shipped by #119).

## Goal

Make `lib/include/snes/audio.h`'s 22-function surface REAL: a
sample-playback engine (voices, ADSR, echo, dynamic BRR loading)
driven from C in the cc65816 ABI, backed by an OpenSNES-owned
resident SPC700 driver uploaded via the `apu` module.

## Non-goals

- No sequencer/tracker: SNESMOD remains the music path, untouched.
- No mixing/DSP-bypass tricks: the S-DSP's 8 hardware voices ARE the
  engine.
- No binary blob checked in: the driver builds from source
  (`wla-spc700`) at lib build time.

## Architecture

```
 C API (lib/source/audio.c — pure C, cc65816 ABI for free)
   │  WRAM mirror: per-voice {sampleId, vol, pan, pitch}, sample
   │  directory copy, next_free_addr, master state
   ▼
 command protocol over $2140-$2143
   ▼
 resident driver (lib/source/audio_driver.spc700.asm, ~≤2 KB)
   command loop → DSP register writes; APU_CHECK_RESET compatible
```

- **65816 side in C.** None of these calls are per-frame hot paths;
  `volatile` MMIO is honoured by QBE (chantier A2). No new ASM means
  no new ABI-lint surface — the skip-file marker disappears WITH the
  legacy file. If phase 2 measures the byte-stream loop as too slow
  in C, a single tiny ASM helper may be added (lint-verified).
- **Driver blob plumbing**: lib/Makefile gains
  `audio_driver.spc700.asm → wla-spc700 → wlalink -b → .bin`,
  incbin'd by a 6-line `audio_blob.asm` (SUPERFREE section, symbols
  `audio_driver_blob`/`_end`) — the `sm_spc.asm` pattern, but from
  source. Same driver binary for all 4 lib variants (ARAM is
  mapper-agnostic).
- **Module deps**: `audio` requires `apu` (upload). Wire in
  common.mk's `_resolve_deps` (module link order is alphabetical and
  order-independent).

## ARAM memory map

| Range | Contents |
|---|---|
| `$0000-$01FF` | zero page, stack, driver variables |
| `$0200-$09FF` | driver code (2 KB budget — enforce at build: hard fail if `.bin` > $0800) |
| `$0A00-$0AFF` | sample directory: 64 entries × 4 bytes (start, loop) |
| `$0B00 → ESA` | BRR sample area (grows up; CPU-side allocator) |
| `ESA → $FFC0` | echo buffer (top of ARAM; see echo policy) |

Echo policy: `ESA` fixed at `$C000` (16 KB echo ceiling = EDL up to
7; `audioSetEcho` rejects delay > 7 unless samples top is low enough
— v2 keeps it simple: EDL 1-7 supported, 8-15 returns
`AUDIO_ERR_NO_MEMORY`). `audioGetFreeMemory() = $C000 - next_free`.
Echo writes stay disabled (`FLG` bit 5) until `audioSetEcho`, and the
buffer region is NOT cleared at init (only on first `audioSetEcho`,
driver-side clear loop) so init stays fast.

## Command protocol

Port roles (CPU view — input latches CPU→APU, output ports APU→CPU):

| Port | CPU→APU | APU→CPU |
|---|---|---|
| `$2140` | command byte | ack (echo of command byte) |
| `$2141` | param0 (8-bit) | result0 |
| `$2142/43` | param1 (16-bit) | result1 (16-bit) |

**Command byte** = `(seq << 6) | opcode`, `seq` cycling 1→2→3→1
(never 0), opcode 0-61. Consecutive commands therefore always differ
— the echo-ack is unambiguous even for repeated opcodes. `$FE`
(= seq 3, opcode 62) is forbidden as a command: it is
`APU_RESET_MAGIC`, and the driver's main loop runs `APU_CHECK_RESET`
so `apuReset()` hot-swap keeps working over the driver.

**CPU sequence** (every command): write params → write command byte →
bounded-wait for ack (echo) → read results if any. All waits bounded
(~2 frames) returning `AUDIO_ERR_TIMEOUT` — unlike the apu module's
blocking primitives, the v2 C layer never hangs the game.

### Opcode map (↔ header functions)

| Op | Name | p0 | p1 (16-bit) | result | Serves |
|---|---|---|---|---|---|
| $01 | MVOL | vol | — | — | audioSetVolume |
| $02 | KON | voice | dirIndex | — | audioPlaySample/Ex (final step) |
| $03 | KOFF | voice mask | — | — | audioStopVoice/StopAll |
| $04 | VVOL | voice | volL:volR | — | audioSetVoiceVolume + pan math (CPU-side) |
| $05 | VPITCH | voice | DSP pitch | — | audioSetVoicePitch (CPU converts 0x1000-relative) |
| $06 | VADSR | voice | adsr1:adsr2 | — | audioSetADSR (CPU packs) |
| $07 | VGAIN | voice | gain | — | audioSetGain |
| $08 | ECHO_CFG | EDL | EFB:EVOL | — | audioSetEcho (driver clears buffer first use) |
| $09 | ECHO_FIR | index | coef | — | audioSetEchoFilter (8 commands) |
| $0A | ECHO_ON | voice mask | — | — | audioEnableEcho/Disable |
| $0B | DIR_SET | dirIndex | start (loop via $2141 2-step? no → see below) | — | sample registration |
| $0C | LOAD | — | addr | — | enter block-receive mode (below) |
| $0D | ENVX | voice | — | ENVX | audioGetVoiceState.active |
| $0E | PING | — | — | version | audioIsReady |

`DIR_SET` needs start AND loop (2×16-bit) — two commands:
`DIR_START(index, start)` + `DIR_LOOP(index, loop)`, or fold into the
LOAD flow: after a load completes, the driver knows `addr`; C sends
`DIR_SET(index, loopOffset)` and the driver writes
`{addr, addr+loopOffset}`. Folded version chosen (one command).

### Sample upload (LOAD block mode)

IPL-shaped per-byte handshake, driver-side implementation:
`LOAD(addr)` → driver enters receive loop → CPU streams each byte:
data on `$2141`, index low-byte on `$2140`, driver echoes index after
storing. Index `$FF` wraps naturally; end signaled by writing the
command-byte of the next `DIR_SET` command... no — explicit:
CPU sends byte count up front in `LOAD`'s p1? p1 carries `addr`;
byte count goes in a preceding `LOAD_SIZE(size)` command (op $0F).
Rate expectation ≈ IPL upload (fine: loads happen at level
transitions, not mid-frame). During LOAD the driver does not run the
command loop (document: don't call audio functions from NMI —
already the SDK stance).

audioLoadSample flow: check free memory → `LOAD_SIZE(size)` →
`LOAD(next_free)` → stream bytes → `DIR_SET(id, loopPoint)` → update
WRAM mirror {spcAddress, size, loopPoint, flags=LOADED} → bump
next_free. audioUnloadSample: mirror-only free ONLY when it is the
last-loaded sample (LIFO reclaim); otherwise marks the slot unloaded
without reclaiming (no compaction in v2 — documented).

## API decisions (vs the existing header)

- **Full 22-function parity.** The surface stays; the semantics
  finally work. Signatures unchanged except documented behavior:
- `audioUpdate()`: no-op kept for source compatibility (engine is
  command-driven; nothing to pump). Documented as such.
- `audioPlaySample(id)`: voice = round-robin auto-allocation.
  `audioPlaySampleEx` with `AUDIO_VOICE_AUTO`: prefer a voice whose
  ENVX reads 0 (one `ENVX` poll per candidate, mirror-guided),
  fall back to round-robin.
- `audioGetVoiceState`: filled from the WRAM mirror + one `ENVX`
  command for `active`.
- `audioGetSampleInfo`, `audioGetVolume`, `audioGetFreeMemory`:
  WRAM mirror only (no APU round-trip).
- Error codes: existing `AUDIO_OK/ERR_*` constants, now real.
  Every port wait bounded → `AUDIO_ERR_TIMEOUT`.
- `audioInit()`: `apuWaitBoot()` (or `apuReset()` if driver already
  resident — detectable via PING) → upload blob → execute →
  PING-verify. Safe to call after a snesmod session only via full
  `apuReset()`-style reboot — v2 documents "one engine per ROM"
  exactly like the apu module does today.

## Driver skeleton (phase 1 target)

Main loop: `APU_CHECK_RESET` → read `$F4`; if != last_cmd and != 0
and != $FE → dispatch opcode; write results; echo command byte to
`$F4` out; store last_cmd. Jump table ≤ 16 entries. DSP writes via
the WDSP idiom. No timers needed (pure reactive loop → lowest
latency, ~30 SPC cycles/poll).

## Validation plan

- **Probes** (`probes/audio_v2.py`): PING round-trip; LOAD then
  spc-dump ARAM assert (BRR bytes at expected addr + DIR entry);
  PLAY then DSP assert (KON'd voice SRCN/pitch/vol) + ENVX > 0;
  echo config assert (EDL/EFB/FIR in DSP dump); timeout path
  (call before audioInit → AUDIO_ERR_TIMEOUT, no hang).
- **Example**: `examples/audio/soundboard` — pad-driven: face
  buttons play different samples (our own BRR assets: reuse
  speech_synth phonemes + cello), D-pad adjusts pitch/volume live,
  START toggles echo. Exercises load/play/voice/echo end-to-end.
- **Audio captures**: RMS/spectral checks per feature as in the
  #119 arc; listening pass by the owner.
- Full suite + `make lint` (ABI lint runs WITHOUT the audio.asm
  skip after retirement — the marker documentation in
  `.claude/rules/abi_lint.md` gets its "Current users" table
  emptied).

## Phases

1. **Driver core + boot**: blob plumbing, command loop, MVOL/KON/
   KOFF/VVOL/VPITCH/VADSR/VGAIN, PING; audioInit/IsReady/SetVolume/
   Stop*/voice setters in C; first probe.
2. **Sample pipeline**: LOAD_SIZE/LOAD/DIR_SET streaming,
   audioLoadSample/Unload/PlaySample/Ex, allocator + mirror,
   free-memory accounting; ARAM probes.
3. **Echo + state readback**: ECHO_*/ENVX commands, audioSetEcho/
   Filter/Enable/Disable, audioGetVoiceState/SampleInfo; echo probe.
4. **Retirement + example**: delete audio.asm, drop skip-file
   marker + update abi_lint.md + tech note (Path E executed),
   rewrite audio.h docs (warning block → quick start that is TRUE),
   soundboard example + README + screenshot, docs counts, CHANGELOG.

Each phase lands as one validated commit on develop (suite + lint +
owner listening where audible).

## Risks

- **Driver size creep**: 2 KB budget enforced mechanically at build.
- **Port-protocol races**: the seq-bits scheme removes the repeated-
  command ambiguity; LOAD mode is the only stateful stretch —
  bounded by LOAD_SIZE, and the driver ignores $FE only outside
  APU_CHECK_RESET points (reset stays possible between commands, not
  mid-LOAD; document).
- **NMI reentrancy**: audio API is main-thread-only (like dma queue
  rules). Stated in the header.
- **snesmod coexistence**: unchanged rule — one engine per ROM.
