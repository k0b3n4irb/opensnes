# apu_switch — hot-swapping APU programs at runtime


The `apu` module's upload path runs once, at boot. This example adds
the missing capability: **replacing the running APU program at any
time**. Press A for the play_noise drum kit, B for the pitch_mod
vibrato cello — the kit cuts, the cello blooms, and back, forever.
The backdrop tracks the active program (red = drums, amber = cello).

The method is krom (Peter Lemon)'s PlayTwoSong protocol, productized:
`apuReset()` writes `APU_RESET_MAGIC` to I/O port 0; the SPC700
program polls for it inside its wait loops (the `APU_CHECK_RESET`
idiom from `templates/memmap_spc700.inc`), silences the DSP, acks on
its output ports, and jumps back to the IPL boot ROM at `$FFC0` —
which is re-entrant, so the full boot handshake works again and the
next `apuUpload()`/`apuExecute()` proceeds exactly like the first.

krom's original demo swaps between two songs whose samples are ripped
from commercial games (Axelay, Final Fantasy VI, the Halken logo), so
this port swaps between two OpenSNES-native programs instead — same
protocol, zero rights issues.

ROM mode: LoROM (project default).

## SNES Concepts

- Cooperative APU reset: CPU→APU messaging over `$2140` *after* boot
- The IPL ROM is re-entrant — re-enable it (`CONTROL` bit 7), jump to
  `$FFC0`, and the `$AA/$BB` + `$CC` upload protocol runs again
- **Hot-swap hygiene**: the DSP arrives *dirty* in the second program
  (this example's cello inherited the hi-hat's ADSR on its LFO voice
  and lost its vibrato until the program zeroed `V0ADSR` explicitly).
  Every register a voice depends on must be written — never rely on
  reset defaults after a swap.
- Two independent APU programs in one ROM, uploaded on demand

## Validation

Functional probe (`tools/luna-test/probes/apu_switch.py`): boot state,
non-silent drums, and the B-press swap asserted on `current_song` via
WRAM. Audio parity of each program vs its source example verified by
capture (drums RMS/onsets; cello 174.6 Hz, vibrato ±1.1 %, LFO 5 Hz —
identical to pitch_mod).

## How to Build

```bash
make
```

## Modules Used

console, apu, input
