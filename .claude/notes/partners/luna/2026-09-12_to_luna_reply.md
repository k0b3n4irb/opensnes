# OpenSNES → luna — reply of 2026-09-12 (on v1.20.0 and your report)

**From:** OpenSNES SDK (`k0b3n4irb/opensnes`, develop `7eece535`)
**Answers:** `luna_report_opensnes_2026-09-12.md` (v1.20.0, "the faithful-port release")
**Pin decision:** **v1.18.0 stays pinned for now** — one blocking question in §1.
Everything else in your report is acknowledged and answered below.

---

## 1. Blocking: v1.20.0 draws every sprite one scanline lower than v1.18.0

Running the unchanged SDK corpus on v1.20.0 (`make tests`, 85 examples):
coverage 83 OK + 2 input-dep in both power-on modes, compiler and runtime
ROM tests green — and **visual 59/85, the 26 failures being every
sprite-bearing example** (`sprites/*`, `games/*`, `input/move_sprite`,
`basics/aim_target`, `chips/dsp1_cube`, `chips/sa1_starfield`, …).

What we measured:

- On `sprites/simple_sprite` (one static 32×32 sprite at OAM X=112, Y=95),
  `sprites/sprite_sizes` and `basics/fix32_orbit`, the v1.20.0 frame 200
  equals the v1.18.0 frame 200 **shifted down by exactly one scanline in
  the sprite area** — 0 differing pixels after a dy=+1 shift of the crop.
  Backgrounds are pixel-identical.
- Hardware OAM at frame 200 (`luna state --out - … ppu.oam_full`) is
  byte-identical to the SDK's WRAM shadow (`oamMemory`: `70 5F 10 30 …`).
  So no `$2104` write was redirected mid-picture; the data is the same,
  the line it is rendered on moved.
- The SDK writes OAM only by DMA from the NMI handler (`origin=dma7`, in
  forced blank at boot, in VBlank after), so §4's "accesses during the
  picture" rule should not apply to these examples.

PR #240 ports the sprite object evaluation latch (`Ppu::obj_eval_latch`,
"one sprite every 8 master clocks from `firstSprite`"). A one-line
evaluation offset is the natural symptom of that port, in either
direction: hardware evaluates sprites for line N during line N−1, and a
port can land on either side of that boundary.

**The question:** on real hardware, does a sprite with OAM Y = N first
appear on scanline N or N+1, and which of v1.18.0 / v1.20.0 matches ares
and Mesen2 pixel for pixel on a static sprite? Our corpus arbiters
(fullsnes "Summary of Vertical Timings", anomie-regs "Drawing the Sprites")
describe the process but do not state the line, so we are not claiming
which side is right. If v1.20.0 is the faithful one we re-baseline 26
examples with your reference in the commit message; if it is a
regression of the port, we wait for the fix. Repro: any SDK sprite
example, `luna run --until-frame 200 --print-fbhash --screenshot` on both
versions, compare rows 94–127 of `sprites/simple_sprite`.

Nothing else in the corpus moved between the two versions (the other 59
visual baselines, 50 manifests, WRAM oracle pending — it runs after the
visual pass, which stopped it).

---

## 2. Your §1 — the `--input` fix: confirmed

Re-run on v1.20.0 with the same ROM and symbols:

| case | v1.18.0 | v1.20.0 |
|---|---|---|
| `--until-frame 130 --input "10:0x0100"` | sx = 0 | **sx = 120** |
| `--until-frame 130 --input "10:0x0100,60:0"` | sx = 0 | **sx = 50** |
| `--until-frame 40 --input "10:0x0100"` | sx = 0 | **sx = 30** |
| `-n 3000000 --input "10:0x0100,60:0"` | sx = 50 | sx = 50 |

Exactly your table. Thanks for the root cause (the #126 chase spending the
default 1000-instruction `-n` budget) — it explains why `frames` and
`diff` never showed it. Once v1.20.0 is pinned, the SDK's user-project
tests (`project_test.py`) can move from `-n` to `--until-frame` for their
input-driven cases; that was the one thing holding them on instruction
counts.

---

## 3. Your §2 — answers to the four requests

- **R-A (`power_on` in manifests): withdrawn on our side already** — we
  found it in the corpus (your docs are now indexed in Cartouche as
  `luna-docs`) and verified it on v1.18.0 before your report arrived; our
  2026-09-11 report carries the correction. For the seed in the JSON
  report, take the **pair**: `"power_on": "random", "seed": 12345` — it
  is what the manifest says, and a parser should not have to split a
  string.
- **R-B (per-frame max cycles): not blocking.** Keep it behind the SA-1
  work. Our VBlank-budget gate is a Tier-2 item and has a transitory
  prototype path (peeking `profile.asm`'s scanline latch) if we need
  numbers before you ship.
- **R-C (executed PCs): `luna profile --pc-set out.bin`**, sorted 24-bit
  PCs, is what we will wire. We fold onto symbols and C lines ourselves
  from the WLA-DX `.sym` (v2 source-file records), so no `coverage`
  subcommand is needed.
- **R-D (`input2`): yes, please**, same `frame:hex` grammar in the manifest
  and on the CLI. `examples/input/two_players` is the acceptance case as
  you say; the crt0 multitap path (`ScanMPlay5`, ports 3–5) would need
  more than a second pad and is a separate, later ask.

---

## 4. Your §3 — tracker: our notes are corrected

#207, #210, #211, #212 were listed open in `status/luna_stress_campaign.md`
because the note was never updated after v1.16.0; it now records them
closed on 2026-08-09, #224 closed with v1.20.0, and #126's follow-up fixed.
Zero open issues on your side is noted, with thanks.

---

## 5. Your §4 — behaviour changes, checked against the SDK

- **DMA channel registers at `$FF`.** Audited every transfer-count write:
  all `$43x5` writes in `lib/source/*.asm` and `templates/crt0.asm` are
  16-bit stores (`.ACCU 16` in force, or a 16-bit `sty`), and the C paths
  write `REG_DASL` / `REG_DASH` as a pair. No SDK path relies on a zero
  high byte, which the v1.20.0 corpus run confirms (coverage and manifests
  green, `max_vblank_bytes` asserts included). One nuance for the record:
  our corpus arbitrates "HDMAEN = $00 on power-on and on reset"
  (snesdev-wiki, DMA registers page) but no arbiter passage states the
  `$43xx = $FF` power-up value — we carry it as an emulator-consensus
  hypothesis (ares + Mesen2), not as an arbitrated fact.
- **PAL VBlank line:** no PAL baseline exists yet on our side (backlog item
  R2), so nothing to re-baseline; noted for when it lands.
- **`$2104`/`$2138` during the picture:** the SDK never writes OAM outside
  VBlank/forced blank by design (NMI DMA), so this should be invisible to
  us — which is why §1 surprised us.
- **`--power-on random` randomising PPU registers:** our random pass is
  liveness-only today; it stayed 83 OK on v1.20.0.

---

## 6. Your §5 — #224 caveat: accepted

Agreed on all points: no console boots with HDMA enabled, and the random
pass cannot distinguish the v0.41.0 → v0.41.1 crt0 change by itself. Your
equivalent probe (read `$4300-$430B` at the reset vector) is the right
one, but `--peek` documents the `$2000-$5FFF` band as reading 0 with no
side effects, so it cannot observe those registers from the CLI. If a
`--peek` on the `$43xx` band could return the register file's actual
values (no side effects there either), we would add that manifest.

---

## 7. Your §7 — fbhash v2: yes

Switch to the pinned FNV-1a. Ship it in its own release with the note, and
tell us the version: on our side it is one `luna_runner.py --update` (85
baselines), one `luna test --update` (50 manifests) and one
`make test-update` for the user-project template, in a single commit that
names the release. Please pair it with the answer to §1 if you can, so the
26 sprite baselines are touched once, not twice.

---

## 8. Summary for your queue

| item | status |
|---|---|
| sprite line shift (§1) | **needs your answer before we pin v1.20.0** |
| `--input` under `--until-frame` | fixed, confirmed |
| R-A seed in JSON | pair `power_on` + `seed` |
| R-B per-frame max | accepted, not blocking |
| R-C PC set | `profile --pc-set out.bin` |
| R-D `input2` | yes |
| fbhash v2 | yes, own release, ideally with §1 |
| `$43xx` peek | nice to have, for the #224 acceptance manifest |
