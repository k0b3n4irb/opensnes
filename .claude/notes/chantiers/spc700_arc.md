# Chantier: SPC700 arc (#119) — raw APU path

Status: IN PROGRESS (2026-07-15). Branch wip/spc700. Port 1: PlayBRRSample.

## Reference facts (full read of BOTH halves)

**65816 side** (PlayBRRSample.asm, 27 lines + SNES_SPC700.INC macros):
IPL boot-ROM protocol on APUIO0-3 ($2140-$2143):
- SPCWaitBoot: wait IO0==$AA, write IO0 (clear $CC), wait IO1==$BB
- SPCBeginUpload: X=spc_addr → IO2/3 (16-bit); cmd = IO0+$22 (bne guard:
  if result 0, inc — "special case fully verified"); cmd → IO1? NO —
  reread: adc #$22 → sta IO1, sta IO0; wait IO0==cmd; X=0
  (CAREFUL at port time: krom writes the CMD to BOTH IO1 and IO0.)
- SPCLoadByte: data → IO1; index low byte (TXA) → IO0; wait IO0==index;
  X++ (per-byte handshake, index echo = ack)
- SPCExecute: addr → IO2/3, 0 → IO1, cmd=IO0+$22 → IO0, wait ack
- Boot upload dest = SPCRAM ($0200); krom's .spc = raw assembled binary
  of the SPC side (NOT an .spc savestate).

**SPC700 side** (PlayBRRSample_spc.asm, 55 lines, bass `arch snes.smp`):
- SPC_INIT (from the INC's SPC700 section — read it at port time)
- DSP writes via $F2/$F3 (WDSP macro): DIR=$03 (dir at $0300),
  KOFF=0, MVOLL/R=63; echo: ESA=$88, EDL=5, EON=1, FLG=0, EFB=80,
  FIR0=127 FIR1-7=0, EVOLL/R=25; voice 0: VOL 127/127,
  PITCH=$1000 (=32kHz), SRCN=0, ADSR1=%11111010, ADSR2=%11100000,
  GAIN=127, KON=1. Sample dir at $0300: dw start, loop
  (loop = start + (2032/16)*9). BRR at $0400.

## Design

1. **make/common.mk**: two-stage SPC700 build cloned from the GSU rule:
   `SPCSRC` (%.spc700.asm) → wla-spc700 → wlalink -b → %.spc700.bin →
   .incbin. Memory map for wla-spc700: flat 64KB APU RAM, org $0200.
2. **lib `apu` module (opt-in, NEW — the anti-audio.asm)**: clean
   cc65816 ABI, lint-verified (no skip-file):
   - void apuWaitBoot(void)
   - void apuUpload(const u8 *src, u16 spcAddr, u16 size)  (far ptr)
   - void apuExecute(u16 spcAddr)
   ASM impl (timing-tight polling loops), lda N,s annotated for the
   ABI lint. NOT touching snesmod/audio.asm (parallel path).
3. **examples/audio/play_brr**: C main = apuWaitBoot → apuUpload(code)
   → apuUpload happens INSIDE code image? krom uploads ONE image
   (code+dir+sample as a single blob $0200..) then executes. Simplest:
   one blob, one upload. SPC side ported to WLA syntax.
   Own sample: python BRR encoder (filter-0 blocks: header
   [range<<4|filter<<2|loop<<1|end], 16 nibbles signed) — a decaying
   pluck (sine*exp) at 32kHz. LoROM (owner convention: default, state
   the mode in README).
4. **Validation**: luna --audio-out WAV. Oracles: (a) our ROM's WAV
   FFT peak at expected f0 (pitch $1000 = 1.0 x 32kHz sample rate ->
   f0 = generator frequency), echo tail present (~EDL*16ms spacing);
   (b) krom's ROM WAV in luna for protocol sanity (his sample, his
   spectrum — different art, same machinery); (c) SPC-side register
   table in the README. If luna's DSP diverges → Mesen2 cross-check
   (the #113 method) before filing anything.

## Risks / unknowns
- wla-spc700 syntax + memmap for a flat binary (first project use!)
- luna APU fidelity unknown territory (owner: "un vrai challenge que
  j'ai affronté avec mes ingénieurs Luna")
- BRR filter-0-only encoding = lower quality (acceptable for a pluck)

## State
- [x] Arc issue #119 filed; both halves read; design set
- [ ] common.mk SPC700 stage + first wla-spc700 assembly working
- [ ] lib apu module + ABI lint clean
- [ ] play_brr example + WAV spectral validation
- [ ] docs (audio tutorial section), counts 66, merge, close port 1

## Port 2: speech_synth (2026-07-16/17) — SHIPPED, machinery proven

Method = phoneme BANK sequencing (krom's 32 phonemes spell PETER LEMON;
ours: 5 formant-synthesized phonemes say OPEN SNES). Debugging saga
(all owner-ear-driven): (1) flat-binary layout collision — sequencer
outgrew $02xx, .ORG $0300 dir overwrote code tail (dir → $0800);
(2) timer ticks 2x short → vowels died early (4 ms grain, krom-exact);
(3) inter-phrase KOFF pulse = the audible 'toc' (krom: none, ADSR
decays). Proof chain: luna spc-dump (EXISTS — covers most of luna#122!)
diffed ARAM uploads (byte-intact), directories (entry-equivalent), DSP
regs (equal); final owner A/B in luna AND Mesen2: perceived 'defects'
present in the original = the technique's authentic sound.

Lessons pinned: budget the APU code page before placing .ORG data
sections; replicate the reference's TIMER GRAIN, not just durations;
bare-KON retriggers (KOFF pulses truncate waveforms audibly);
'own art' for speech = full phoneme-bank craft (our 5-phoneme bank is
intelligible-adjacent; krom-quality banks are recorded/curated).

## Port 1 (play_brr) REMOVED by owner decision (2026-07-17)

Three art attempts (4-bit sine 'hive', additive tone 'power line',
formant voice 'vuvuzela/ghost') then krom's tenor sample ('pipe, too
high' — the melody transposes a fixed recording up to +12 semitones).
Owner: don't sink more time. The example is deleted; every LESSON it
produced survives: the apu module, the SPCSRC stage + .incbin deps,
the BRR filter-search encoder knowledge (gen_phonemes.py uses it), the
flat-binary layout rule, the timer-grain rule — all shipped via
speech_synth, which remains the module's worked example. If a
sample-playback demo returns someday: an INSTRUMENT (bell/e-piano)
synthesizes convincingly where voices don't, and melodies need
samples ROOTED near the melody's register.
