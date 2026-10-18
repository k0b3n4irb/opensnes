# Enhancement chips

**"more horsepower, when the base hardware isn't enough."** Some
SNES cartridges carry a coprocessor. These examples boot one, prove it runs,
then put it to real work — the branch you reach for late in a project, not on
day one.

Three chips — SA-1 and Super FX as boot-then-work pairs, the DSP-1 as a single worked example (it needs no boot: the firmware is already on the chip):

## The ladder

| Example | Developer question | Chip |
|---------|--------------------|------|
| [sa1_hello](sa1_hello/) | How do I boot the SA-1 and hand data back via I-RAM? | SA-1 |
| [sa1_starfield](sa1_starfield/) | How do I offload real math to the SA-1? | SA-1 |
| [sa1_save](sa1_save/) | How do I save to SA-1 BW-RAM so it survives the power switch? | SA-1 |
| [superfx_hello](superfx_hello/) | How do I boot the GSU and run a hardware test? | SuperFX |
| [superfx_3d](superfx_3d/) | How do I render 3D with the SuperFX? | SuperFX |
| [superfx_game_skeleton](superfx_game_skeleton/) | How do I run a 60 fps game loop while the GSU renders? | SuperFX |
| [superfx_save](superfx_save/) | How do I save to a Super FX cartridge's RAM? | SuperFX |
| [dsp1_cube](dsp1_cube/) | How do I offload 3D rotation + perspective to the DSP-1? | DSP-1 |

Boot first (`sa1_hello`, `superfx_hello` — prove the chip is alive and
talking), then a real workload (`sa1_starfield`, `superfx_3d`), then a game
loop and a save (`superfx_game_skeleton`, `sa1_save`, `superfx_save`).

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
fails — see [dsp1_cube](dsp1_cube/).
