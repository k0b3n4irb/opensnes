# Audio Examples

The SNES has a dedicated audio processor — the SPC700 — with its own 64KB of RAM,
its own DSP chip, and its own instruction set. It runs independently from the main CPU.
The two communicate through just 4 shared bytes.

These examples cover two approaches:

**Bare-metal** — upload your own driver to the SPC700, trigger sounds manually.
Maximum control, minimum abstraction.

**SNESMOD** — a full tracker audio engine that handles music and sound effects from C.
You write `.it` files in a tracker, `smconv` converts them, and the library plays them.

| Example | Approach | What you'll learn |
|---------|----------|------------------|
| [SFX Demo](sfx_demo/) | Bare-metal | SPC700 upload protocol, BRR format, port handshaking |
| [SNESMOD Music](snesmod_music/) | SNESMOD | Tracker music playback, smconv tool, the process loop |
| [SNESMOD SFX](snesmod_sfx/) | SNESMOD | Sound effects alongside music |
| [SNESMOD HiROM](snesmod_hirom/) | SNESMOD | Audio in HiROM mode (64KB banks) |

Start with **SFX Demo** if you want to understand how audio works at the hardware level.
Start with **SNESMOD Music** if you just want music in your game.
