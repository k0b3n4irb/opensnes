# OpenSNES → luna — report of 2026-09-11

**From:** OpenSNES SDK (`k0b3n4irb/opensnes`, HEAD `0ffd06bf`, develop)
**Against:** luna v1.18.0 (`tools/luna-test/luna.version`), Linux aarch64
**Context:** the 2026-09-11 gaps review of the SDK
(`.claude/notes/reviews/2026-09-11_gaps_review.md`, branch `wip/gaps-review`)
classified every ROM-side gap as either *harness wiring* (luna already has
the capability; nothing to ask) or *luna request*. This report carries only
the second kind, plus one confirmed bug and the open items already on the
tracker. Per the SDK's Luna-First rule, each request is specified by the
prototype's intended I/O contract, not by a hunch.

Thanks for v1.18.0: the six requests of the 2026-09-08 report (frame-indexed
capture, master-cycle buckets, `--power-on`, `luna diff`, mem-trace origins,
`luna profile`) all landed and are acknowledged in the SDK's pin commit. The
SDK harness now captures at PPU frames (`--until-frame 200`, animated
examples `[200, 400]`); one re-baseline, corpus 85/85 on the new key.

---

## 1. Confirmed bug — `--input` is not applied when the run is bounded by `--until-frame`

First observed 2026-09-03 on v1.17.0 (SDK note
`status/luna_stress_campaign.md`), **re-validated 2026-09-11 on v1.18.0**.

ROM: SDK corpus `examples/mode7/perspective/perspective.sfc` (D-pad Right
increments `sx` every frame; `pad0` mirrors `padHeld(0)`; symbols from the
WLA-DX `.sym`, loaded via `--sym`).

```
# A — bounded by --until-frame: the pad is never latched
luna state --until-frame 130 --input "10:0x0100" --sym perspective.sym \
     --out /dev/null --peek pad0:2 --peek sx:2 --peek frame_count:2 perspective.sfc
  pad0 = 00 00   sx = 00 00   frame_count = 80 00     (130 frames ran)

# B — bounded by -n: the same script applies (checkpoint AND release)
luna state -n 3000000 --input "10:0x0100,60:0" --sym perspective.sym \
     --out /dev/null --peek pad0:2 --peek sx:2 --peek frame_count:2 perspective.sfc
  pad0 = 00 00   sx = 32 00   frame_count = 4B 00     (50 frames of Right)

# C — as A with the release checkpoint too: still nothing
luna state --until-frame 130 --input "10:0x0100,60:0" …   →  sx = 00 00

# D — snapshot at frame 40 while the mask should be HELD: pad0 still 0
luna state --until-frame 40 --input "10:0x0100" …          →  pad0 = 00 00, sx = 00 00
```

Expected: `--input`'s frame-latched checkpoints apply regardless of how the
run is bounded. The `--until-frame` help text is precisely "Makes
input→assert probes land on an exact frame rather than a generous `-n`", so
this is the flag combination the feature exists for. Case D rules out a
release-timing explanation: the mask is not latched at all.

Impact on the SDK: none today (the visual/coverage pillars pass no input;
the 28 `input =` manifests run through `luna test`, and `probes/lib.py`
keeps `-n`). It blocks the planned move of scripted-input runs to
frame-exact bounds.

Not yet checked: whether `luna run --until-frame` / `luna frames` /
`luna diff --input` share the behaviour (`diff` and `frames` take both
flags). Happy to run those if useful.

---

## 2. Requests (owner-validated designs, prototypes to follow)

### R-A. `power_on` key in the `luna test` manifest schema — WITHDRAWN 2026-09-12

> **Correction (2026-09-12):** already shipped. The luna docs (now in the
> Cartouche corpus as `luna-docs`) state that `luna test` manifests take
> `power_on = "random"` plus `seed` (default 1). Verified on the pinned
> v1.18.0: a manifest with `power_on = "random"` / `seed = 7` runs and prints
> `power-on: random (seed=0x0000000000000007)`. We missed it because
> `luna test --help` does not list manifest keys. Nothing to do on your side;
> the text below is kept for the record.


`--power-on zero|ones|random[=seed]` exists on `run`, `state`, `frames`,
`diff`, `profile`, and the SDK will use it from the runner. The manifests
(`luna test`) have no equivalent, so a probe cannot say "this assert must
hold from garbage RAM". Proposed:

```toml
power_on = "random=7"     # top-level; same grammar as the CLI flag; default "zero"
```

Contract: identical fill and seed reporting as the CLI (`--power-on
random=<seed>` prints the seed; the JSON report should carry it so a failing
manifest is reproducible). The v0.40.0 / v0.41.1 boot fixes in the SDK
(forced blank, NMITIMEN/HDMAEN zeroed at reset) are the bug class this
targets; luna zero-fills by default and could not see them.

### R-B. Per-frame maximum cycles of a symbol (NMI / VBlank time budget)

`luna profile` gives master cycles per symbol over the run (totals and
`--top`). The SDK needs the *worst single frame* of a symbol — typically the
NMI handler — to gate "the VBlank work fits". Proposed output addition:

```json
"symbols": [{ "name": "NmiHandler", "cycles": 123456,
              "per_frame": { "max": 5210, "max_frame": 133, "mean": 4980 } }]
```

Optional: `--budget NmiHandler=6000` returning exit 1 when `per_frame.max`
exceeds the value, so a manifest or a CI step can gate on it. Transitory
prototype on the SDK side (to pin the numbers before this ships): peek the
scanline latch of `lib/source/profile.asm` at each frame and keep the max.

### R-C. Executed-PC set for ROM code coverage

Symbol-level coverage is derivable today (`profile --out` JSON, symbols with
cycles > 0, unioned over the corpus, diffed against the `.sym`). C-line
coverage needs the set of distinct PCs executed. Proposed:

```
luna profile <rom> --until-frame N --pc-set out.bin     # sorted u24 PCs, or
luna coverage <rom> --until-frame N --out cov.json       # {"pcs":[...], "symbols":{...}}
```

Contract: every PC at which an opcode fetch occurred, deduplicated, whole
24-bit space (ROM incl. HiROM `$C0-$FF`, WRAM-resident code). The SDK maps
PCs to C lines through the WLA-DX `.sym` v2 source-file records
(`CC65816_G=1` builds emit `@cline`). Transitory prototype: reduce
`state --cpu-trace` to unique PCs on one ROM; its output is the reference.

### R-D. Second joypad in manifests (question, not a request yet)

`luna state` has `--port` / `--mouse` / `--superscope`; the manifests accept
`mouse =` and `superscope =` (28 use `input =` for pad 1). Nothing in the
SDK corpus drives port 2 from a manifest, and the `input` grammar's support
for a second pad is not documented in the SDK. If `input2 =` (or a
`port = 2` checkpoint key) exists, say so and this becomes wiring; if not,
the SDK's `input/two_players` example and crt0's multitap path (`ScanMPlay5`,
ports 3–5) have no scripted probe and would need it.

---

## 3. Already on the tracker — status check

| issue | SDK note | ask |
|---|---|---|
| #207 | STAT78 (`$213F`) reports 5C78 / PPU2 version 2; Mesen2 reports 3 | modelling choice? a one-line answer closes it on our side |
| #210 | `[asserts.blocks]` keyed by offset | still open? |
| #211 | `audio_rms_min` reads a silent ring | still open? — blocks the WAV-hash audio regression the SDK plans (`--audio-out`, hash only) |
| #212 | manifest-coverage gaps | still open? |
| #126 | `state --input` misapplied checkpoint frames | closed in v1.13.0 per SDK notes — the SDK note is being updated; no action |

---

## 4. Wiring the SDK will do on its side (no action for luna, FYI)

So you can see what the next SDK harness commits will exercise, and where a
regression on your side would surface first: `--power-on random=<seed>`
on coverage and compare runs; `luna diff --frames --tolerance` as the
protocol for validating compiler/library changes; `luna profile` replacing
the SDK's static cycle estimate in CI; `--force-region pal` weekly;
`--audio-out` hashed on three examples; `--native-res` for the hires
examples; `state --call-stack` printed on a coverage failure; `luna bench`
nightly over the corpus.
