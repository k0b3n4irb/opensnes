# luna → OpenSNES: reply to the 2026-09-20 report

| | |
|---|---|
| **From** | luna (`k0b3n4irb/luna`, `develop` @ `4808f6e`, CI green) |
| **Re** | `opensnes_report_for_luna_2026-09-20.md` (against luna v1.24.0) |
| **Status** | **R1, R2, R3 and R4 are all implemented and pushed to `develop`.** No tag yet — see *Getting a build*. |

Thank you for §1. That 301-of-301 figure, the stack that grew into a
fixture's globals, and the two places where the real DSP-1B firmware
contradicted your own header are worth more than the four requests put
together. That is what the introspection surface is for, and knowing it
landed changes what we prioritise next.

Two of your four requests turned out to be something other than what the
report describes. Both differences change what you should conclude on your
side, so they are spelled out rather than quietly fixed.

---

## R1 — the trigger is not the one in the report

`--dsp1-rom ""` never reaches luna. clap rejects it first:

```
error: a value is required for '--dsp1-rom <DSP1_ROM>' but none was supplied
```

What destroyed your dump was any source file that is **readable but not a
firmware image**. Your variable did not expand to nothing — it pointed at a
file that existed and was empty. `install_firmware` did a bare `fs::copy`:
it copied zero bytes, succeeded, and reported success.

This matters to you because guarding against the empty string would have
fixed nothing. The validation is on content.

**What is in place now:**

- the source is read and vetted **before the destination is touched at
  all** — exactly 8192 bytes for `dsp1b.rom` / `dsp1.rom`;
- the install goes through a staging file and a `rename`, so an interrupted
  or failing write cannot truncate an install that was already good;
- the message names the size found, the size required, and states that what
  was already installed is untouched.

```
$ luna state --dsp1-rom /path/empty.rom game.sfc
warning: could not install <dir>/empty.rom: firmware: <dir>/empty.rom is 0 bytes;
'dsp1b.rom' must be exactly 8192 — refusing to install, so the file already
installed (if any) is untouched
```

**On hashing.** You suggested "or does not hash to a known DSP-1/1B dump".
We deliberately stopped at the length. DSP-1 and DSP-1B are both legitimate
at 8192 bytes and other regional variants may exist, so pinning a hash list
would lock out valid files in order to catch a case the length already
catches. If you would still like it, we can warn (not refuse) on a dump
whose hash we do not recognise — say the word.

### The half that cost you the diagnosis

You do not mention it, and it is probably why the hunt took as long as it
did: **after the overwrite, `missing_firmware` was `null`.** luna was not
merely silent about the broken dump — it asserted the firmware was fine.

And the one-line stderr note you ask for already existed, in full:

```
warning: 'SUPER MARIO KART' needs coprocessor firmware 'dsp1b.rom' which was not
found — the coprocessor stays inert (e.g. Mode 7 graphics will be wrong). Supply
it with `--dsp1-rom <path>` or place 'dsp1b.rom' in <config>/luna/firmware.
```

It was never reached, because a 0-byte blob was accepted as firmware, so
nothing was missing any more. `Cartridge::set_coprocessor_firmware` now
refuses a blob of the wrong size outright, which makes `missing_firmware`
honest again — and that alone brings the warning back. You asked for a
message; the fix was to stop the lie, and the message was already written.

**On your side:** if a corrupted `dsp1b.rom` is still sitting in a CI
runner's `~/.config/luna/firmware`, it is now *reported* instead of
silently inert. Your restored dump's MD5 `332273cc…` is the correct one.

---

## R2 — done, and not by copying

`luna profile` now accepts `--input2` … `--input5`, `--port1`, `--port2`,
`--mouse`, `--superscope`, with the same grammars as `state`. Your I/O
contract runs unchanged:

```bash
luna profile rom.sfc --until-frame 300 --port1 mouse \
     --mouse "30:0,0,0;60:20,20,0" --pc-set out.bin --out /dev/null --top 0
```

Exit codes match `state`: **1** when the emulator refuses a device, **2**
for a malformed flag.

```
$ luna profile game.sfc --port2 bogus
error: --port2: unknown device `bogus` (pad, mouse, superscope, multitap, none)   → exit 1
```

The flags were not copied into a third place. `state` and `profile` now
share one implementation (`parsers::apply_input_flags`), because copying is
precisely how they drifted apart in the first place, and a test pins the
surface: every flag must parse under both verbs or the test fails.

---

## R3 — done, with one rule you need to know about

`cpu.sp_min` is in `luna state`'s JSON, a `stack` block is in `luna
profile`'s, and `--stack-floor` closes the gate in CI.

```json
"cpu": { …, "sp_min": { "sp": 7203, "pc": 4967259, "frame": 4,
                        "symbol": "AudioDriverBoot" } }
```

```bash
luna profile --from-frame 120 --until-frame 600 --stack-floor 0x1C60 game.sfc
# stack: deepest S $1C23 at $00CB5B (AudioDriverBoot, frame 4) < $1C60 — UNDER
#   → exit 1
```

`--stack-floor` takes `0x`-hex, `$`-hex or decimal, so an address pasted out
of a linker script works as-is.

Two deliberate differences from your sketch:

- **`pc` is a 24-bit integer**, not the string `"00:CB5B"` — that is the
  convention everywhere else in luna's JSON (`ProfileEntry.addr`). To make
  up for it there is a `symbol` field, resolved against the loaded `.sym`:
  *which routine went deepest* is the question you will ask next.
- **One object** instead of three flat `sp_min` / `sp_min_pc` /
  `sp_min_frame` fields: the three only mean anything together, and `null`
  then says "nothing to report" cleanly.

### The rule, and why it exists

**Only an instruction that *lowers* `S` moves the mark — and only in native
mode.**

This is not a refinement; the first test written against the feature
demolished the first implementation. Every ROM leaves reset with
`S = $01FF` and **enters native mode still carrying it**, before `TXS`
installs the real stack. A naive "smallest `S` ever seen" therefore pinned
the mark at `$01FF` for the whole run — below any floor worth checking, and
at a depth no push ever reached. The number would have been useless and
your gate would have failed permanently.

Hence both rules. A value *inherited*, or loaded by `TXS`, is not a reach.
And emulation mode is excluded because the hardware confines `S` to page 1
there: its absolute value belongs to another régime and is not comparable
with a native stack.

**What this means for your bookkeeping:** if your audio driver reaches 989
bytes deep in native mode, luna sees it. If a phase of your boot runs in
emulation mode and goes deep there, luna will not. Tell us if that is a real
case for you and the rule can be adjusted — it is a deliberate choice, not a
limitation of the measurement.

`profile` starts the measurement at `--from-frame`, so the figure describes
the profiled window rather than the boot; `state` measures from reset.

---

## R4 — the hardware question, settled by both references

You asked (a) what an unplugged port reads. **Both references agree**, which
is the strongest answer available short of an oscilloscope:

| | unplugged port | standard gamepad |
|---|---|---|
| **ares** | `ControllerPort::data()` returns `0` when no device is allocated (`controller/port.cpp:12`) | 12 button bits, `0000` signature, then **`return 1`** on every further clock (`controller/gamepad/gamepad.cpp:39-46`) |
| **Mesen2** | `GetOpenBus() & 0xFC` for `$4016` with no device in the loop to OR anything back → data bits read `0` (`SnesControlManager::Read`) | same: the serial line idles high |

**So, and this is the heart of your problem:**

- **Auto-read cannot tell them apart.** An empty port shifts in zeros, so
  `$4218`/`$4219` read `$0000` — which is also exactly what an idle
  connected pad reports. Your NMI handler zeroing any word with a non-zero
  low nibble is not what is hiding the difference; the difference is not in
  that register at all.
- **It only appears past bit 15**, in a manual serial read: a connected pad
  keeps returning **1** (its data line idles high), an unplugged port keeps
  returning **0**, for ever. That is the detection, and it is what luna
  models. luna already implements the pad side (the `$4016`/`$4017`
  idle-high fix), so the asymmetry is real in the emulator today.

For (b), `--port1 none` / `--port2 none` exists (aliases `empty`,
`unplugged`), as does **Devices → Nothing (unplugged)** in the GUI, and the
manifest `port1` / `port2` keys take the same names.

**A caveat worth keeping for your console session.** Both emulators
*model* 0; neither documents a measurement, and a genuinely floating line on
real silicon would more likely read high. Two references agreeing is not a
hardware fact. `docs/HARDWARE_VERIFICATION.md` keeps its value here, and if
the console disagrees, luna is what changes — send us the trace.

**One neighbouring gap, since you will be probing `$4017`:** its bits 2-4
are forced to 1 in both references (ares `0b111`, Mesen2 `|= 0x1C`) and luna
leaves them at 0. It is a known deviation on our accuracy scorecard and it
affects anything that reads `$4017` as a whole byte. Say the word and we
close it.

---

## Your §4

- **`steps` vs `-n`.** Noted; both spellings stay.
- **`--peek SYMBOL:N` on 24-bit symbols.** Glad it carries half your fixture
  vectors. It is not going anywhere.
- **The audio oracle.** "Same signal at offset N, max delta D" is a feature
  request wearing an observation's clothes, and a fair one: a SHA-256 cannot
  distinguish "four samples later" from "wrong". We have not built it,
  because it deserves to be scoped with you rather than guessed at:
  - alignment by cross-correlation, or a bounded offset search?
  - tolerance in LSBs, or in dB?
  - on the mixed output, or per DSP voice?

  Answer those three and we will size it.

---

## Verified

91/91 golden ROM tests against the pinned corpus — including
`input_controller_latency`, the controller-sensitive one — plus 894
workspace tests, `cargo fmt` and `cargo clippy --all-features -D warnings`
clean, debug and release.

## Getting a build

Everything above is on `develop` (`4808f6e`). Nothing is tagged yet: v1.25.0
shipped on 2026-09-19 and `develop` already carries a save-state format
break (v7 — unrelated to your requests; it comes from a CPU interrupt-timing
fix) and we would rather group than ship two breaking releases two days
apart.

If you need a tagged binary to pin in `tools/luna-test/luna.version`, say so
and we will cut the release now. Otherwise `develop` is authoritative from
today.

Nothing needs filing as an issue on your side — the lot is done.
