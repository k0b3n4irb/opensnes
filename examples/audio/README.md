# Audio Examples

The SNES has a dedicated audio processor -- the SPC700 -- with its own 64KB of RAM,
its own DSP chip, and its own instruction set. It runs independently from the main CPU.
The two communicate through just 4 shared I/O bytes.

These examples use **SNESMOD**, a tracker-based audio engine. You write `.it` files
in a tracker (OpenMPT, Schism Tracker, etc.), the `smconv` tool converts them to
SPC700 format, and the library handles playback from C.

## Examples

| Example | Difficulty | Description |
|---------|------------|-------------|
| [snesmod_music](snesmod_music/) | Intermediate | Tracker music playback with smconv and the SNESMOD process loop |
| [snesmod_sfx](snesmod_sfx/) | Intermediate | Playing sound effects alongside music |

## Prerequisites

- Build the `smconv` tool: `make tools`
- A tracker module in `.it` format (Impulse Tracker)

## Key Concepts

- The SPC700 runs its own program independently of the 65816 CPU
- SNESMOD uploads its driver + song data to SPC700 RAM at init
- Call `spcProcess()` every frame to keep audio synchronized
- Sound effects use separate channels and can play over music

---

Start with **snesmod_music** to get music playing, then move to **snesmod_sfx**
to layer sound effects on top.
