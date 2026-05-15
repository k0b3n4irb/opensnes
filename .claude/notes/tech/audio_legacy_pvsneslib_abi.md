---
name: audio.asm — legacy PVSnesLib ABI, broken under cc65816
description: The 26 functions in lib/source/audio.asm use PVSnesLib's R-to-L push + 1-byte-per-u8 packed arg convention, not cc65816's L-to-R + 2-byte-slot. Calling them from C wouldn't reliably work. They are advertised in lib/include/snes/audio.h as if they did. The lint correctly skips them as custom CC. This is structural API debt to resolve in a dedicated chantier.
type: tech
---

## What

`lib/source/audio.asm` (26 functions) and `lib/include/snes/audio.h`
(~25 prototypes) form what the SDK presents as the "OpenSNES Audio
System": `audioInit`, `audioLoadSample`, `audioPlaySample`,
`audioSetVoiceVolume`, etc. The header includes a quick-start
@code block showing how to call these from a normal C `int main()`.

**The header lies.** The ASM functions use the PVSnesLib calling
convention (right-to-left push order, each u8 arg packed into one
stack byte instead of a 2-byte slot, 24-bit pointer instead of
post-A6 4-byte pointer). cc65816 emits the opposite of each of those
choices. The result: multi-arg audio functions called from C read
their args from the wrong stack slots and operate on garbage.

## Discovery path

Found while extending the static lint `devtools/check_asm_abi.py`
during the chantier elargissement-du-filet session of 2026-05-15.
The lint flagged 20+ "ABI mismatch" entries across `audio.asm`. On
investigation:

1. `audioLoadSample` prologue pushes `php; phb; phd` AND uses a
   custom packed-arg comment block (`id(1), pad(1), brrData(3), size(2),
   loopPoint(2)` — total 9 bytes, NOT the 10 bytes cc65816 would push).

2. `audioSetVoiceVolume(u8 voice, u8 volumeL, u8 volumeR)` reads at
   consecutive 1-byte offsets `lda 6,s ; voice`, `lda 7,s ; volumeL`,
   `lda 8,s ; volumeR`. With cc65816's 2-byte u8 slots, this would
   read `volumeR.low`, `volumeR.pad`, `volumeL.low` and label them
   wrong — total swap of values.

3. ZERO examples in `examples/` call any of these audio functions.
   The only audio used in practice is SNESMOD (`examples/audio/snesmod_*`).
   The legacy API is documented but unused, which is why the bug class
   has been latent: no caller, no failure.

The lint skips `audio.asm` correctly under its `phb/phd/phx/phy →
custom CC` rule. That's the right call — the offsets the ASM uses
ARE consistent with PVSnesLib's convention; it's just not what
cc65816 emits.

## What works by accident

Single-u8-arg functions like `audioSetVolume(u8 volume)` happen to
work. With cc65816's 2-byte slot at offset 5..6, the low byte of
`volume` lands at 6,s — which IS where the ASM reads. So the value
arrives correctly. This is structurally fragile (a future refactor
that changes 1-arg slot positioning would break it silently), but
today it works.

The same is true for any zero-arg functions: `audioInit`,
`audioStopAll`, `audioUpdate`, `audioIsReady`, `audioGetVolume`,
`audioGetFreeMemory`. Anything multi-arg, takes a pointer, or
returns a struct → broken.

## Why the lint can't auto-fix this

Even if the lint detected the PVSnesLib convention and tried to
check it on those terms, it would have to:
- Track 8/16-bit mode at every push (REP/SEP #$10, #$20 toggles)
- Know the 1-byte-per-u8 packing rule
- Recognize the `pad(1)` byte that some functions insert
- Discriminate "1 arg works → leave alone" from "N args broken → flag"

That's a separate lint with its own rules. Not worth building unless
the audio API gets a real caller surface.

## Resolution paths (none done yet)

In rough order of effort:

### Path A — Deprecate the API publicly
`@deprecated` annotations on every function in `audio.h`. Add a
`@warning` paragraph in the file header pointing to SNESMOD. Keeps
the source intact, sets correct expectations for users. Cost: ~30 min.

### Path B — Add a cc65816 shim layer
Write `lib/source/audio_shim.asm` (or `.c` with inline asm) that
exposes cc65816-compatible entry points and translates the args
into the PVSnesLib-packed layout before calling the legacy ASM.
Cost: ~3-4h for the full surface, plus probe tests.

### Path C — Rewrite audio.asm for cc65816
Refactor each of the 26 functions to use cc65816's stack layout.
Easier than path B (single source of truth), but breaks any caller
that was already using the PVSnesLib convention (there are none in
the SDK, but external user code might exist). Cost: ~4-6h plus
extensive probe testing.

### Path D — Replace audio.asm entirely with SNESMOD wrappers
SNESMOD already provides a working audio engine. Expose just a few
convenience entry points (audioInit, audioPlaySample for sample id N)
that delegate to SNESMOD internally. Drop the legacy ASM. Cost:
similar to C, but cleaner end state.

## Recommendation

**Path A** is the right next step. It costs nothing and stops the
documentation from misleading users. The deeper fix (B/C/D) can wait
until someone genuinely needs the legacy API working — historically
zero caller surface argues for D (replace it).

## Cross-references

- `devtools/check_asm_abi.py` — the lint that surfaces this
  (correctly skips audio.asm via `phb/phd/phx/phy` heuristic).
- `lib/source/audio.asm` — the 26 affected functions.
- `lib/include/snes/audio.h` — the header that currently
  misrepresents the API as cc65816-callable.
- `tools/opensnes-emu/test/functional/` — where audio probes
  WOULD live, but none exist because no example exercises this API.
- Chantier note: any future "audio resurrection" chantier reads this
  doc first.

## When this rule does NOT apply

- The SNESMOD subsystem (`lib/source/snesmod.asm`, `examples/audio/snesmod_*`)
  is a different audio engine and is NOT affected. SNESMOD has its
  own ABI which has been validated by working examples.
- The single-arg audio functions (`audioSetVolume`, etc.) work by
  accident; don't deliberately depend on them, but they're not the
  failure mode this doc is about.
