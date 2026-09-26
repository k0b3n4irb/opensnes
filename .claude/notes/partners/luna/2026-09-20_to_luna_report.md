# OpenSNES → luna : report and capability requests — 2026-09-20

| | |
|---|---|
| **From** | OpenSNES SDK (`k0b3n4irb/opensnes`, `develop` @ `d4271733`) |
| **luna in use** | v1.24.0 (pinned in `tools/luna-test/luna.version`) |
| **Status** | Requests below are proposals for the owner to validate before any issue is filed (`.claude/rules/luna_tooling.md`). Nothing has been filed. |

## 1. What luna made possible since v1.24.0

Worth saying first, because the requests below are small next to it.

- **Every public function of the SDK is now executed by a test**: 301 of 301,
  from 167 "never executed" three days ago. The measurement is
  `luna profile --pc-set`, folded onto `.sym` labels, over 93 ROMs and ~190
  legs (each example idle, plus one leg per `luna test` manifest replaying its
  joypad script, plus three library fixtures).
- **`[asserts.ppu]` / `[checkpoint.ppu]`** removed the last assertions made on
  our own WRAM shadows. They carry the window, colour-math, mosaic, SETINI,
  Mode 7 and scroll vectors of the library fixtures.
- **`--trace-writes`** found two bugs that nothing else would have: a stack
  that grew into a fixture's result globals (the writers of a corrupted
  variable turned out to be `NmiHandler` and the audio driver's `cmd_send`),
  and a one-pixel manifest failure that was a sampling race, not a behaviour
  change (the variable was written on scanline 261 of 262; the write sequences
  of the two ROMs were byte-identical).
- **Running the real DSP-1B firmware** contradicted our own header twice:
  `Distance` reads one low on exact lengths ((3,4,12) → 12), and `Range`
  returns the squared difference shifted right by 15, not the raw difference.
  No hardware reference we could find states either.
- **`luna diff`** is the gate that lets us re-baseline the WRAM oracle honestly:
  85/85 MATCH at +0 before every re-capture this week, including the commit
  that moved two examples' assets from bank `$00` to bank `$07`.

## 2. Requests, simplest first

### R1 — `--dsp1-rom ""` overwrites the installed firmware with an empty file

**Kind:** bug (data loss). **Cost to fix:** small.

```
luna state --dsp1-rom "" --until-frame 150 rom.sfc
  → "installed DSP firmware → ~/.config/luna/firmware/dsp1b.rom"
```

The flag "installs, then loads". With an empty path (a shell variable that
expanded to nothing — our mistake) it installed a 0-byte file over a valid
8192-byte dump and reported success. Every DSP-1 ROM then ran with
`dsp1.instructions_executed = 0`, with no error: the chip just stayed inert.
We restored the dump from another copy and verified it against the published
MD5 (`332273cc…`).

**Ask:** refuse to install when the source path is empty, missing, or not
8192 bytes (or does not hash to a known DSP-1/1B dump); never overwrite a
valid installed file with an invalid one. A one-line stderr note when a DSP-1
cartridge runs with no usable firmware would have shortened the diagnosis.

### R2 — `luna profile` takes `--input` only

**Kind:** parity gap. **Cost:** small (the grammars already exist in `state`).

`luna state` and `luna test` accept `--input2`, `--mouse`, `--superscope`,
`--port1`, `--port2`. `luna profile` accepts `--input` only. Our coverage
ratchet replays each manifest's script through `profile --pc-set`; the mouse
and Super Scope manifests therefore run input-free, and ten `input.h`
functions are "executed" only because a fixture calls them with no device
plugged — a weaker test than the manifests that already exist.

**Ask:** `luna profile` accepts the same input flags as `luna state`
(`--input2`, `--mouse`, `--superscope`, `--port1`, `--port2`), same grammars.

**I/O contract (what we would run):**
```
luna profile rom.sfc --until-frame 300 --port1 mouse \
     --mouse "30:0,0,0;60:20,20,0" --pc-set out.bin --out /dev/null --top 0
```

### R3 — deepest stack reach over a run

**Kind:** new measurement. **Cost:** small/medium.

Our link-time RAM budget checks that plain C RAM leaves N bytes free below
`$2000`, where N is a guess at stack depth (512). A fixture passed that check
with 942 bytes free while its stack reached **989 bytes** deep during the
audio driver's boot and overwrote result globals. We measured the depth
afterwards from stack residue in zero-initialised RAM — which only works
because luna powers on with zeroed RAM, and only when nothing else touches the
area.

**Ask:** `luna state --out -` (and `luna profile`) report the minimum value of
S over the run, with the PC and frame at which it occurred:
```json
"cpu": { ..., "sp_min": 7203, "sp_min_pc": "00:CB5B", "sp_min_frame": 4 }
```
and, ideally, a gate like `--budget`: `--stack-floor 0x1C60` → exit 1 if S ever
went below it. That turns "stack vs globals" into a CI check instead of a
debugging session.

### R4 — an unplugged controller port

**Kind:** new device + a hardware question. **Cost:** unknown — depends on
what the hardware does.

`--port1` / `--port2` accept `pad`, `mouse`, `superscope`. There is no way to
run with **nothing plugged**, so `padIsConnected()` cannot be tested — and it
is currently wrong: our NMI handler zeroes any auto-joypad word whose low
nibble is non-zero before storing it, so whatever an empty port reads, the
library sees an idle pad and answers "connected".

We could not find a reference that pins what `$4218/$4219` read for an empty
port (queried: fullsnes "Controllers I/O Ports — Automatic Reading",
anomie's timing doc, snesdev-wiki, the Super Famicom Dev Wiki). fullsnes gives
the 4-bit device signature of a standard pad (`0000`) but not the floating-
line value.

**Ask:** (a) if the luna team knows or can measure what an unplugged port
returns on auto-joypad read and on manual `$4016/$4017` clocking, tell us — it
decides the design of our fix; (b) a `none` device for `--port1` / `--port2`
(and the manifest `port1` / `port2` keys) that models it.

This one is also on our real-hardware checklist
(`docs/HARDWARE_VERIFICATION.md`); a console session may answer (a) before you
do.

## 3. Priority, from our side

| # | Request | Why it matters to us |
|---|---|---|
| R1 | refuse an invalid `--dsp1-rom` | silent data loss of a file the user cannot re-download |
| R3 | minimum SP | closes a class of bug our static budget cannot see |
| R2 | `profile` input parity | makes the coverage ratchet honest for two peripherals |
| R4 | unplugged port | blocks one known-wrong public function; needs a hardware fact first |

## 4. Small observations (no ask)

- `luna test` manifests accept `steps = N` as a run bound; `luna profile` has
  `-n` for the same thing. We map one onto the other in our ratchet — it
  works, just noting that the two spellings exist.
- `luna state --peek SYMBOL:N` resolving 24-bit symbols (bank `$7E` RAM
  sections) is what lets a Python harness assert on variables C itself cannot
  read. It is used in roughly half of our fixture vectors.
- The audio oracle (a SHA-256 of 300 frames of `--audio-out`) moved when a
  boot path became 125 µs longer: the signal was identical, four samples
  later. Expected for a hash; we explained it by aligning the two WAVs by hand.
  If a `luna diff`-style tool for audio ever exists ("same signal at offset
  N, max delta D"), that is the comparison we did.
