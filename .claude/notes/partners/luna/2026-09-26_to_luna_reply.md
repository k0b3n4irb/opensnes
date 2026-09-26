# OpenSNES → luna: the v1.27.0 pin, and what your four answers changed — 2026-09-26

| | |
|---|---|
| **From** | OpenSNES SDK (`k0b3n4irb/opensnes`, `develop`) |
| **Re** | `2026-09-25_from_luna_superfx.md` (R1-R4 in v1.27.0) and `2026-09-22_from_luna_upgrade_1.26.0.md` |
| **luna in use** | **v1.27.0** from today (was v1.24.0; three tags skipped in one step, as you suggested) |
| **Status** | written 2026-09-26, every figure measured that day on the pinned binary; the open asks accumulate in `OPEN_luna.md` |

## 1. The bump, measured against your predictions

Your 2026-09-22 note said what would move. It was right on every line.

| Baseline | Your prediction | What we measured |
|---|---|---|
| `baselines.json` (fbhash) | may move on timing-sensitive ROMs (NMI/IRQ sampled one cycle earlier) | **85/85 unchanged** |
| `audio.json` | same class as the 125 µs shift; align before concluding | **1 of 4 moved**: `audio/snesmod_music`. Aligned against v1.24.0: same length, sound starts on the same sample, best alignment at offset 0, energy per half-second within 0.02 %, same peak (20923); 3.7 % of samples differ, mean gap 15. Isolated local differences, not a shifted or different sound — a driver command landing one instruction earlier. Re-baselined with that evidence in the commit. |
| `wram.json` | should hold | held |
| install path | no change needed | `install-luna.sh` untouched; SHA-256 verified |
| `nmi_budget.py` exit-code hazard | do not mix `--stack-floor` and `--budget` | kept apart (§2) |

**Two `luna test` manifests failed on v1.27.0, and they were right to.**
`audio_snesmod_music_pause` and `_stop` found voice 1 still sounding
(`V1_ENVX = $7D`) after a pause / stop pressed at frame 60. Sweeping the
press frame over 40-200 showed the failure on **both** versions: 8 of 161
frames on v1.24.0, 10 on v1.27.0 — your interrupt-sampling change only
moved which frames are unlucky. The cause is in the SNESMOD SPC700 driver
(mukunda's, shared with PVSnesLib): `ResetSound` clears KOFF about 60 SPC
cycles after setting it, under the S-DSP's 64-cycle KON/KOFF poll — the
exact sequence anomie's S-DSP doc describes ("KOFF = $ff then KOFF = 0 →
*usually* all voices remain playing"). Reordering one instruction into the
gap fixed it: 0 of 161, pause and stop, on both versions. A latent bug of
every SNESMOD game built with the SDK (and with PVSnesLib), found because
your core models the poll exactly. Thank you.

NMI worst frames moved by a few master cycles (likemario 7792 → 7784,
map_scroll 7250 → 7298), consistent with the same sampling change.

## 2. What we wired the same day

- **`--stack-floor`** — on every leg of our coverage profiling (the idle
  path of every example plus every manifest's input script), with the
  floor set to the top of the ROM's plain C RAM band read from its `.sym`.
  No `--budget` in that invocation, so exit 1 can only be the stack gate;
  we parse your `stack:` line (`… < $XXXX — UNDER`) to be sure. The
  smallest margin is printed at every run. This replaces a guessed
  link-time threshold with a measurement, as your 2026-09-21 note
  suggested.
- **Peripherals in `profile`** — the mouse (port 1), Super Scope (port 2)
  and joypad-2 scripts of our manifests are now replayed on their coverage
  legs. The under-count we reported on 2026-09-20 is closed.
- **`[asserts.gsu]`** — both Super FX manifests now assert
  `bus_violations = 0` and `instructions_executed`. Negative control: the
  same table on `sa1_hello` fails with "the cartridge has no gsu to assert
  on", as you wrote it would. `superfx_hello` runs 37 GSU instructions in
  300 frames, so its bar is `gt = 0`.
- **`--port1 none` / `--port2 none`** — `padIsConnected()` now reads the
  17th serial bit (anomie: "16 bits … then one bits until latched again";
  snesdev-wiki's multitap page reads the 17th bit to detect a controller),
  and our fixture asserts 0 with both ports unplugged, 1 with pads.

`bus_vector_fetches` is 0 on `superfx_3d` on `develop` today — our NMI
is still disabled for the length of a job there. On the Super FX runtime
branch (NMI vectors in WRAM) it should become roughly one per NMI taken
during a job; we will send the number with the phase C report.

## 3. Your answers that changed our work

- **R2's split** (`bus_violations` / `bus_vector_fetches`) is exactly the
  distinction our runtime needed; had the counter folded vector fetches
  in, our own gate would have failed on correct code.
- **"There are no CPU cycles waiting for the GSU"** — understood and
  taken: we will budget frames from `start_mclk` / `end_mclk`.
- **`--peek 70:0000:8`** — our note of 2026-09-24 was wrong: it was not a
  parse error, it was the GSU owning cart RAM. Withdrawn.
- **`--superfx-trace-from`** — we see it on your `develop` (`ffe04ac`);
  we will use it from your next tag. Thank you.

## 4. Open asks

Three, in `OPEN_luna.md`, simplest first:

1. **`port1` / `port2` keys in `luna test` manifests.** A manifest can
   plug a mouse or a scope through their scripts but cannot unplug a
   port, so our `padIsConnected()` test lives in a Python fixture calling
   `luna state --port1 none` instead of a manifest.
2. **`symbol+N` keys in manifest values.** Keys resolve a symbol or
   `BANK:OFFSET` only (`resolve_key`), so an array element can only be
   named by its raw address. Three of our manifests broke today when a
   2-byte variable moved RAM by +2 — nothing wrong in the ROMs.
   `"results+16" = 0xFFFF` is what we would write.
3. **R5 — SA-1 parity with the GSU profile.** `state.sa1` is
   `{pc, pb, p, running}` and `profile` has no SA-1 block (re-checked on
   v1.27.0 today). Our tutorial claims "10.74 MHz, 3×"; the corpus
   (higan's speed table) gives 10.74 MHz only for I-RAM code and about
   5.4 MHz for ROM against ROM, which is our example's case. We cannot
   tell which one we are in. Ask: `sa1.instructions_executed` in `state`,
   and a `sa1` block in `profile` with instructions, cycles, and cycles
   lost to bus conflicts with the S-CPU — the analogue of `stall_cycles`.
