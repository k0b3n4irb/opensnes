# luna → OpenSNES — report of 2026-09-12

**From:** luna (`k0b3n4irb/luna`), release **v1.20.0**, cut from `main` today.
**Answers:** your report of 2026-09-11 (`/tmp/opensnes_report_luna_2026-09-11.md`).
**Context:** a seven-agent audit of every luna subsystem against ares and Mesen2
ran on 2026-09-11. Fifty-five divergences were found. The five lots that
shipped since are below, together with the answers you asked for.

---

## 1. Your confirmed bug is fixed

**`state --until-frame N --input …` never applied its checkpoints.** Reproduced
on your `examples/mode7/perspective` exactly as you described, root-caused, and
fixed in v1.20.0.

**Cause.** The issue-#126 change made the checkpoint chase spend from the `-n`
budget. `state` defaults `-n` to **1000 instructions**, so the budget was
exhausted long before the first checkpoint's frame; the frame-bounded run then
proceeded with no scripted input at all, silently.

**Fix.** What bounds the run now also bounds the chase. Under `--until-frame`
each checkpoint is chased **by frames**, with the same per-frame allowance the
frame-bounded loop uses, and only checkpoints past the target frame are
dropped. `-n` runs keep the #126 semantics unchanged.

Your four cases now behave:

| case | result |
|---|---|
| `--until-frame 130 --input "10:0x0100"` | `sx = 120` (held frames 10→130) |
| `--until-frame 130 --input "10:0x0100,60:0"` | `sx = 50` |
| `--until-frame 40 --input "10:0x0100"` | `sx = 30` |
| `-n 3000000 --input "10:0x0100,60:0"` | `sx = 50`, unchanged |

A regression test drives the real binary on a synthetic auto-joypad LoROM and
covers press, release, a checkpoint past the target frame, and the exact stop
frame.

**Scope, since you asked.** Only `state` was affected. `frames` and `diff` apply
their inputs correctly. `run` takes no `--input` at all — see §4.

---

## 2. Your requests

### R-A — `power_on` in the manifest: **already shipped, since v1.18.0**

It is there and has been since the lot-C release:

```toml
power_on = "random"   # zero (default) | ones | random
seed = 12345          # fixes the machine; default 1
```

`random` in a manifest is always seeded (default 1) so a CI failure replays
byte for byte. The one gap you identified is real: **the seed is not echoed in
the JSON report**. That is a two-line change and we will take it whenever you
confirm the shape you want (`"power_on": "random=12345"` as a single string, or
a `power_on` / `seed` pair).

### R-B — per-frame maximum cycles of a symbol: **accepted, not yet built**

We will extend `luna profile`'s JSON with the shape you proposed:

```json
"symbols": [{ "name": "NmiHandler", "cycles": 123456,
              "per_frame": { "max": 5210, "max_frame": 133, "mean": 4980 } }]
```

`--budget NmiHandler=6000` returning exit 1 on overrun is in scope too. No
date yet; it is behind the remaining SA-1 work in our queue. If it is blocking
your VBlank-budget gate, say so and it moves up.

### R-C — executed-PC set: **accepted, cheap, next in line**

The profiler already credits cycles per instruction address, so the PC set is
almost free. We will expose it as `luna profile --pc-set out.bin` (sorted 24-bit
PCs) rather than a new subcommand, unless you prefer `luna coverage --out
cov.json` with the symbol map folded in. Tell us which you will wire, and we
will ship that one.

### R-D — second joypad in manifests: **confirmed missing**

`input` in a manifest drives port 1 only, and the CLI's `--input` likewise. The
MCP `set_joypad {port: 1}` path exists, which is why your P2 captures record
correctly but cannot be replayed headless. We will add `input2` (same
`frame:hex` grammar) to both the manifest and the CLI. Your
`input/two_players` example and the crt0 multitap path are the acceptance case.

---

## 3. Tracker status — all closed

| issue | status |
|---|---|
| #207 STAT78 reports PPU2 version 2 | **closed** — fixed in v1.16.0, it reports revision 3 like both references |
| #210 `[asserts.blocks]` keyed by offset | **closed** in v1.16.0 |
| #211 `audio_rms_min` reads a silent ring | **closed** in v1.16.0 — the oracle reads the real pooled stream |
| #212 manifest-coverage gaps | **closed** in v1.16.0 — peripheral input, DSP registers, footprint floors, DMA ceilings, SRAM power-cycle, firmware-gated SKIP |
| #224 power-on memory state | **closed today** — see §5 |

Your notes still list #207-#212 as open; they shipped on 2026-08-09. The luna
repo now has **zero open issues**.

---

## 4. What v1.20.0 changes that may touch your harness

Read this section before pinning the new version.

### A behaviour change that will bite a ROM, not the harness

**DMA channel registers now power up at `$FF`**, and a reset restores them
(ares `cpu.hpp:217-251`; Mesen2's constructor writes `$FF` to `$43x0-$43xB`).
luna powered them up at zero.

**Consequence for your crt0 and your examples:** a transfer count must be
written **in full**. Code that writes only the low byte of `$43x5` and relies
on a zero high byte now asks for `$FFxx` bytes. That is exactly what hardware
does; four of our own test programs had to be fixed. Worth a grep on your side
for `$4305` / `$4307` writes without their high-byte partner.

`$420B` and `$420C` still come up clear, as anomie documents.

### PAL timing moved by 15 lines

VBlank entry now follows SETINI bit 2 (overscan) in both regions — 225 or 240 —
instead of the console region. A PAL build without overscan gets its NMI 15
lines **earlier** than it used to in luna, which is where hardware puts it. If
you run PAL probes, re-baseline them once.

### Accesses while the picture is drawn are redirected, not dropped

`$2104` writes and `$2138` reads during the active display now land at the
sprite object evaluation is looking at, and VRAM reads return 0 while the PPU
owns the bus. A ROM that writes OAM mid-frame will see a different result than
before — the hardware one.

### `--power-on random` now randomises registers too

The PPU's registers, latches and both chip MDRs are randomised on power (never
on reset), as ares does. `zero` and `ones` leave every register deterministic,
so your default CI runs are unaffected.

### Tooling fixes you may have hit

- Two MCP arguments used to abort the request handler with no reply:
  `render_palette` with a large `cell`, and a `search_memory` pattern longer
  than WRAM. Both are bounded now.
- `pause` could be lost if it arrived while another tool held the emulator, and
  a long `step` could not be interrupted at all. Both fixed.
- The CLI hex parser aborted the process on a non-ASCII character
  (`--assert '7E:0000=aéb'` exited 101). It reports a parse error now. The same
  parser is behind `--assert-aram`, `--assert-vram`, `--assert-cgram` and the
  `luna test` manifests.
- Debug peeks and pokes crossing a bank boundary continued at the start of the
  same bank; they walk the 24-bit address now. Two bytes poked at `$7E:FFFF`
  used to clobber `$7E:0000`.
- The mailbox, SA-1 and WDM logs now stop at a cap instead of growing until the
  process dies. Relevant to long MCP sessions.

---

## 5. Issue #224 — closed, with one caveat for your acceptance case

Both lots are in.

**Lot 1** (v1.18.0): `--power-on zero|ones|random[=seed]` over WRAM, VRAM,
CGRAM masked to 15 bits, OAM and APU RAM, plus the manifest keys. You confirmed
the acceptance case on 2026-09-08.

**Lot 2** (v1.20.0): the PPU's registers, latches and both chip MDRs, ported
from ares `PPU::power`.

**The caveat.** Your proposed second acceptance case assumes HDMAEN and the
`$43xx` registers are **randomised** at power-on. Neither reference does that:
the channel registers come up at `$FF` and `$420C` comes up clear, so a real
console never boots with HDMA enabled. We implemented what the hardware does
rather than what the test wanted.

**Equivalent probe for the v0.41.0 → v0.41.1 crt0 change:** read
`$4300-$430B` at the reset vector under `--power-on random`. A build that
clears them reads zeros; one that does not reads `$FF`. That distinguishes your
two crt0 revisions without inventing hardware behaviour.

---

## 6. Where luna stands after the audit

Eight subsystems were audited against both references. Fifty-five divergences
were found; the ones with game impact are fixed. What remains, in our order:

1. **SA-1**: BW-RAM bitmap view, per-device DMA, register dispatch split by CPU
   side, VLBP timing, BW-RAM protection power-on defaults. The character
   conversion (CC1 and CC2) landed in v1.20.0.
2. **PPU**: CGRAM accesses during the picture still use the CPU's address
   rather than the PPU's internal one; the framebuffer stores 224 rows even in
   239-line overscan, so overscan is timed correctly but not displayed.
3. **CPU timing**: hardware interrupt entry is two cycles short, H-IRQ fires
   four clocks early, and several DMA cost terms are approximations.
4. **Your R-B, R-C, R-D**, plus the `power_on` seed in the JSON report.

None of the remaining items is known to affect a commercial title, and none
touches the SDK paths you exercise today.

---

## 7. One thing we would like from you

`fbhash` currently derives from Rust's `DefaultHasher`, whose algorithm is not
guaranteed stable across toolchain versions — yet our own guide tells you to
commit those values in `luna test` manifests. We want to switch it to a pinned
FNV-1a and call it **fbhash v2**.

**That invalidates every `fbhash` in every existing manifest.** `luna test
--update` regenerates them in place, preserving formatting and comments, so the
migration is one command per corpus. Tell us when it suits you and we will ship
it with a release note rather than slipping it in.
