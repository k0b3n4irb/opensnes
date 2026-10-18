# SNESMOD: an effect that does not fit overwrites the echo buffer — open

**Status: open, deferred by the owner** (2026-10-18: "garde une note précise
pour le problème SNESMOD. On s'en occupe plus tard"). SNESMOD is frozen
after 03-4 and 03-6 (owner, lot 0 (d), 1.0 audit): any fix below that
touches the driver or the soundbank format is the owner's call.

Already written down for users: `KNOWN_LIMITATIONS.md` (🔴 "A SNESMOD sound
effect larger than the free SPC RAM overwrites the echo buffer"),
`CLAUDE.md` (critical constraints), `docs/tutorials/audio.md` ("An effect
has to fit"), `examples/audio/snesmod_music/README.md` ("Room for the
Effects"), and the row of 2026-10-18 in
`.claude/notes/status/silent_defects_log.md`.

## The defect

`snesmodLoadEffect(id)` sends `CMD_LOADE` and streams the source's BRR to
the SPC700. The driver stores the effect at `xfer_address` — the first byte
after the module and the effects already loaded — and copies it there
without comparing anything with the echo buffer, which takes the top of the
RAM (`ESA` page = `$FF - 8 x EDL`, `EDL x 2 KB`). An effect that does not
fit is written over the echo ring; the DSP's echo writes then overwrite the
effect, and both sound wrong. No error on the CPU side (the call returns a
valid slot) or on the SPC700's.

Code (lib/source/sm_spc.spc700.asm, Mukunda Johnson's driver):

- `CMD_LOADE` (about line 713): `mov a, xfer_address` / `mov y, next_sample`
  / stores the start and `start + LLLL` in `EffectDirectory` ($0300, 4 bytes
  an effect), `call !StartTransfer`. Nothing reads `ESA` or `EDL`.
- `StartTransfer` / `DoTransfer` (about line 677): `mov [xfer_address]+y, a`
  word by word, `_inc_address` on a page wrap. No bound.
- CPU side, `snesmodLoadEffect` (lib/source/snesmod.asm, about line 590):
  refuses only a 17th effect (returns `$FF` since 2026-10-03), reads the
  source's address and length from the soundbank (`SB_SRCTABLE`), never a
  size budget.

**Hypothesis, not measured:** an effect long enough to pass `$FFFF` would
wrap `xfer_address` to page 0 and write over the driver's direct page or
code — a crash rather than noise. Measure before writing it as a fact.

## The measurement (luna, 2026-10-18)

Module `pollen8.it` (echo `EDL 14`, `ESA $8F`: ring at `$8F00-$FEFF`), then
the five samples of `effectssfx.it` loaded as effects. `opensnes-music
inspect` gave pollen8 "16446 bytes of SPC RAM (13506 free of 29952)"; the
five effects take 20 524 bytes. `luna state music.sfc --until-frame 200
--peek APU:0300:18` (the effect directory):

| Effect | Start | In the echo ring? |
|---|---|---|
| 0 tada (8 740 B) | `$5A3E` | no |
| 1 hall strings (4 510 B) | `$7C62` | no |
| 2 piano (4 734 B) | `$8E00` | from `$8F00`, 256 bytes in |
| 3 marimba (1 414 B) | `$A07E` | yes |
| 4 cowbell (1 126 B) | `$A604` | yes |

With the four that fit (strings, piano, marimba, cowbell: `$5A3E-$8845`)
the example played correctly; the merged `snesmod_music` shipped that way
(commit `5a5826ff`), then moved to its own sounds, which all fit (`e6b087bc`:
nocturne leaves 30 008 bytes, four effects take 18 460).

To reproduce, the two modules are in the history (removed by `e6b087bc`):

```
git show ff8d643b:examples/audio/snesmod_music/music/pollen8.it   > pollen8.it
git show ff8d643b:examples/audio/snesmod_music/sfx/effectssfx.it  > effectssfx.it
# a ROM: SOUNDBANK_SRC := pollen8.it effectssfx.it; snesmodLoadModule(MOD_POLLEN8);
# then snesmodLoadEffect(SFX_TADA + i) for i = 0..4; then:
luna state rom.sfc --until-frame 200 --peek APU:0300:18 --out - | jq .apu.dsp
```

## Who knows what

- **The converter** knows everything: each module's SPC RAM use and the
  room left beside its echo (`spc_ram_bytes`, `spc_ram_free`, the `aram_size`
  / `aram_limit` of `tools/lib/smconv/src/it2spc.c`, since 2026-10-17), and
  each source's BRR length.
- **The CPU side** knows each source's length (soundbank table) but not the
  module's end or the echo's start.
- **The driver** knows `xfer_address` and `ESA`, and checks neither.

## Ways to fix it, cheapest first

1. **Tool only, nothing frozen touched.** `opensnes-music bank` writes, in
   `soundbank.h`, the room each module leaves (`MOD_<NAME>_ROOM`) and each
   source's BRR size (`SFX_<NAME>_BRR`), so a game can check with constants;
   the docs show the sum. Informs, does not prevent.
2. **Library check, no driver change (recommended).** `snesmodLoadModule`
   records the module's room (from a table the converter adds to the
   soundbank, or from the constants of 1 passed in by the caller), and
   `snesmodLoadEffect` keeps a running total and returns `$FF` — the value it
   already returns when the 16 slots are taken — for an effect that does
   not fit, loading nothing. The contract of the return value grows by one
   case, the driver stays as it is. Needs a soundbank-format addition if the
   room comes from the bank (frozen: the owner's call).
3. **Driver check.** `CMD_LOADE` compares `xfer_address + LLLL` with
   `ESA x 256` and answers a refusal the CPU side reads. Touches the frozen
   driver and the command protocol; the most complete, the most expensive.

Whichever lands: report it to Mukunda Johnson as an issue (the driver is his
and PVSnesLib ships it too) only with the owner's reading of the final text
(`.claude/notes/partners/snesmod-upstream/README.md`).

## Test plan for the fix

- A fixture ROM: a module with echo, then effects whose total exceeds the
  room by one source. Assert (luna manifest) that the last
  `snesmodLoadEffect` returns `$FF`, that `EffectDirectory` holds only the
  ones that fit, and that nothing was written at or above `ESA x 256`
  (`--peek APU:<ESA>00:...` against a capture of the ring before the load).
- Negative control: the same ROM against the library before the fix must
  fail the manifest (the effect lands in the ring).
- `make tests`, the audio oracle and the phase sweep: the examples' effects
  all fit today, so no example's sound may move.
