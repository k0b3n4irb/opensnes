# Enhancement chips

**"more horsepower, when the base hardware isn't enough."** Some
SNES cartridges carry a coprocessor. These examples boot one, prove it runs,
then put it to real work — the branch you reach for late in a project, not on
day one.

Two chips here — SA-1 and Super FX. The third, the DSP-1, needs no boot (its firmware is on the chip) and has one worked example, in Mode 7: [mode7/dsp1_ground](../mode7/dsp1_ground/), the floor and the objects on it.

## The ladder

| Example | Developer question | Chip |
|---------|--------------------|------|
| [sa1_starfield](sa1_starfield/) | How do I offload real math to the SA-1? | SA-1 |
| [superfx_3d](superfx_3d/) | How do I render 3D with the SuperFX? | SuperFX |
| [superfx_game_skeleton](superfx_game_skeleton/) | How do I run a 60 fps game loop while the GSU renders? | SuperFX |

A real workload on each chip (`sa1_starfield`, `superfx_3d`), then a game
loop the GSU renders under (`superfx_game_skeleton`). Saving on these
cartridges uses the same `sram` calls as any other: the
[SA-1 tutorial](../../docs/tutorials/sa1.md) and the
[advanced Super FX tutorial](../../docs/tutorials/superfx_advanced.md) say
where the battery-backed RAM is.

## The two chips, in one screen

- **SA-1** — the *same* 65816 ISA as the main CPU, but at **10.74 MHz** and
  with its own I-RAM (`$3000–$37FF`) shared with the main CPU. So your SA-1
  code is ordinary 65816 assembly; the art is the handshake (who writes what,
  when) through shared RAM. See the [SA-1 tutorial](../../docs/tutorials/sa1.md).
- **SuperFX / GSU** — a *custom RISC* processor for bitmap and 3D work. It has
  its own ISA (no C compiler — the GSU code is assembly, `.sfx` sources
  assembled by `wla-superfx`), renders into cartridge RAM, and DMAs the result
  to VRAM. See the [SuperFX tutorial](../../docs/tutorials/superfx.md).

All three are validated natively by luna, which detects and runs the SA-1,
the GSU and the DSP-1 in the headless test harness — no chip-ROM side channel
needed. The DSP-1 leg additionally needs Sony's `dsp1b.rom` firmware installed
into luna (copyrighted, not shipped); without it the test SKIPs rather than
fails — see [mode7/dsp1_ground](../mode7/dsp1_ground/), the DSP-1 example.
