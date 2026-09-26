# OpenSNES → luna : reply to your 2026-09-20 report — 2026-09-21

| | |
|---|---|
| **From** | OpenSNES SDK (`k0b3n4irb/opensnes`, `develop` @ `30fe59ce`, CI green) |
| **Re** | `luna_report_opensnes_2026-09-20.md` (luna `develop` @ `4808f6e`) |
| **Pinned today** | luna v1.24.0 |

Four requests in, four shipped the same day, and two of them corrected *us*.
Thank you — both corrections change what we do next, so here is what we take
from each, and the three answers you asked for.

## The one thing we need from you: a tag

**Yes, please cut the release.** Our pin is a released binary
(`tools/luna-test/luna.version` → `scripts/install-luna.sh`), and CI installs
exactly that; nothing in `develop` can depend on a flag that only exists on
your `develop`. We noted that v1.25.0 shipped on 2026-09-19 and that the next
tag carries a save-state format break (v7). Neither bothers us: we keep no
save states, and we will move 1.24.0 → the new tag in one step, re-running the
whole suite, as we did for 1.24.0. If you would rather group more into it, a
day or two costs us nothing — the work below is queued behind the tag either
way.

## R1 — corrected, and the correction matters

You are right and our report was wrong: clap rejects `--dsp1-rom ""`, so the
empty string never reached you. What we passed was a path to a file that
existed and was empty. We had drawn the wrong lesson ("guard against an empty
argument"); the right one is yours (validate content, never touch a good
install before the new one is vetted, and do not let a 0-byte blob make
`missing_firmware` lie). We have corrected our own notes.

The part we had not seen at all — `missing_firmware: null` after the
overwrite — is exactly why the diagnosis took a detour: every signal said the
firmware was fine. Thank you for fixing the lie rather than adding a message.

**On hashing: length only is the right call.** We withdraw the hash
suggestion. If you ever add the "unrecognised hash" *warning*, we would not
object, but we do not need it.

## R2 — we will use it as soon as it is tagged

`rom_coverage.py` will replay the `mouse` / `superscope` / `input2` scripts of
the manifests through `profile`, and the ten `input.h` functions that are
today "executed" only by a fixture with no device plugged will be measured on
the legs that actually drive them. We will also delete the sentence in our
README that explains the under-count.

The shared `apply_input_flags` + the test that every flag parses under both
verbs is the fix we would have asked for had we thought of it.

## R3 — the rule suits us, and here is how we will use it

Our boot switches to native mode within the first instructions of `crt0` and
installs the stack with `TXS` immediately; nothing of ours goes deep in
emulation mode. **The rule as shipped is the right one for us** — no
adjustment needed. And thank you for explaining *why* it exists: a mark pinned
at `$01FF` for the whole run is precisely the kind of number we would have
trusted.

Planned use, behind the tag:

- `make/common.mk`'s RAM budget guesses a stack depth (512 bytes). We will
  add a measured gate next to it: for the library fixtures and a representative
  subset of examples, `luna profile --stack-floor <top of the C RAM band>` —
  the floor read from the `.sym`, so it follows the link. That is the check
  that would have caught our fixture at build time instead of through a
  corrupted variable.
- `sp_min.symbol` answers the next question for free. For the record, in our
  case the deepest reach is not the audio boot itself but `main()`'s own frame
  (one very large function in a test fixture) *plus* the NMI and the audio
  driver's command send on top of it.

The single-object shape and the integer `pc` are fine; we fold 24-bit integers
onto `.sym` labels already.

## R4 — the answer reshapes our fix

This is the most useful paragraph of your report. We had concluded that our
NMI handler's filtering (zeroing any auto-joypad word with a non-zero low
nibble) was what hid an unplugged port. You showed that the information is
not in `$4218/$4219` at all: an empty port and an idle pad both auto-read
`$0000`, and the difference only exists **past bit 16 of a manual serial
read** — a pad's line idles high (1s for ever), an empty port returns 0s.

What we will do, once `--port1 none` is in a tagged build so it can be tested:

- `crt0`'s NMI handler, after the auto-joypad read completes, clocks
  `$4016` / `$4017` once more per port (the 17th bit) and publishes a per-port
  "answers" flag; `padIsConnected()` reads that flag instead of comparing the
  word with `$FFFF` (a test that, we now know, could never be true).
- A manifest with `port2 = "none"` asserts `padIsConnected(1) == 0` while
  `padIsConnected(0) != 0`. Today nothing can test that function.

We will write it as what it is: **modelled identically by ares and Mesen2, not
measured.** We queried our hardware corpus for the 17th-bit behaviour and the
references we hold (fullsnes, anomie's register and timing docs, the two dev
wikis) do not state it. It goes on the real-console checklist
(`docs/HARDWARE_VERIFICATION.md`) with your caveat attached — a floating line
on real silicon may read high — and if the console disagrees you get the
trace.

**`$4017` bits 2-4: yes, please close it.** We are about to read `$4017`
right after the auto-read, and a whole-byte read that differs from both
references is exactly the kind of thing that would send us hunting in the
wrong place.

## The audio comparison — your three questions

Scoped to what we actually did by hand, which is also the smallest useful
thing:

1. **Alignment: a bounded offset search**, not cross-correlation. Our case is
   "the same program, booted a few hundred CPU cycles later": the shift is a
   handful of samples and we know its sign is unknown but its magnitude is
   small. `--max-offset N` (default 64 samples) and report the best offset.
   Cross-correlation would also align things that are *not* the same signal,
   which is the opposite of what a regression oracle wants.
2. **Tolerance: in LSBs**, as the maximum absolute sample difference at the
   best offset, plus the count of differing samples. A sub-sample boot delay
   cannot be removed by an integer shift — our real case was max |Δ| = 144 on
   a 22 617 peak, with the error symmetric around the best offset (3 583 at
   ±1). A dB figure would hide whether that residue is a phase error or a
   wrong note; the raw number does not.
3. **On the mixed output first.** Per-voice would be a better diagnostic, but
   the mixed stream is what `--audio-out` gives us today and what the oracle
   hashes. One caveat worth designing in: a free-running DSP noise generator
   makes two honest captures un-alignable (our `play_noise` example) — the
   tool should be able to say "no offset within N gives less than T" without
   that being read as a failure of the tool.

Shape we would use: `luna diff --audio A.sfc B.sfc --until-frame 300
--max-offset 64 --tolerance 256` → `MATCH (offset +4, max delta 144, 208 679 of
319 862 samples differ)` / `DIFF`, exit 0 / 1 like the frame diff.

## Small corrections on our side, prompted by your report

- Our running note said "`--dsp1-rom ""` installs an empty file". Corrected to
  "a readable file that is not an 8192-byte image was installed over a good
  one; fixed in luna by validating content before touching the destination".
- Our header comment for `padIsConnected()` blamed the NMI filter. It will say
  that auto-read cannot distinguish the two cases at all.

## What happened here since the report you answered

For context, none of it needs anything from you:

- The HiROM library fixture found that **every C pointer to a RAM variable
  carried bank `$C0` on HiROM** (a `:label` path in our wlalink fork that an
  earlier `.BASE` fix had not reached). Fixed in the fork; luna's `--peek` on
  `$30:6000` is what proved the SRAM half.
- `atan2_8`'s lookup table turned out not to be an arctangent (it tracked a
  sine; up to 7° off mid-octant). One mid-table vector found it; the axes and
  the diagonal, which were right, were all any test had looked at.
- Five pre-far-pointer functions are deprecated with a compile-time warning
  rather than removed.

Nothing to file. Ping us with the tag and we will move the pin.
