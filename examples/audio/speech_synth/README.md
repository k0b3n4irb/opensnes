# speech_synth — the SNES says "OPEN SNES"


Port of krom (Peter Lemon)'s **SpeechSynth**
([PeterLemon/SNES](https://github.com/PeterLemon/SNES), `SPC700/SpeechSynth`).
Speech on the SNES = sequencing a **phoneme bank** on one DSP voice: per
phoneme, the APU-side program selects the sample (SRCN), shapes it
(ADSR: plosives one-shot, vowels/nasals looped and sustained), inflects
pitch for prosody, and waits (4 ms timer grain). krom's 32-phoneme bank
spells "PETER LEMON"; our original 5-phoneme bank (formant synthesis,
`gen_phonemes.py`) says **"OPEN SNES"** on loop.

ROM mode: **LoROM** (project default).

## Register fidelity vs the original

| Element | krom | this port |
|---|---|---|
| Master | MVOL 127/127, echo fully off (FLG $20) | same |
| Voice 0 | VOL 127/127, GAIN 127 | same |
| Vowel ADSR | `%11110111`/`%11111100` | same values |
| Plosive ADSR | `%11111111`/`%11100000` | same values |
| Pacing | 32 ms consonants / 256 ms vowels | same (4 ms tick × 8/64) |
| Retrigger | bare KON per phoneme, no KOFF | same |
| Phrase end | plain wait, no KOFF (ADSR decays) | same |

## The debugging archaeology (all owner-listening-test driven)

1. **Flat-binary layout collision**: the sequencer outgrew $0200-$02FF; directory moved to
   $0800.
2. **Timer 2× too fast**: T0DIV=255 gave ~16 ms effective ticks in
   practice; krom's 1 ms grain replicated at 4 ms — vowels finally last
   their 256 ms (vowel-segment count now matches the reference 9 vs 8).
3. **The "toc"**: an inter-phrase KOFF pulse we added truncated the last
   phoneme mid-waveform; krom lets the ADSR decay naturally. Removed.
4. **The perceptual verdict**: with all fixes, the owner A/B'd our
   machinery against krom's demo in BOTH luna and Mesen2 — the
   "noises/defects" heard are present in the original too: that IS what
   robotic phoneme spelling sounds like. Port machinery verified by
   `luna spc-dump` end-to-end: uploads byte-intact, directories
   entry-equivalent, DSP registers equal, phoneme schedule correct.

## Debug tooling that cracked it

`luna spc-dump` exports the full APU state (64 KB ARAM + all 128 DSP
registers) as a playable .spc — diffing two dumps localized every one
of the bugs above. See `docs/tutorials/audio.md`.

## Modules Used

`console`, `apu`
