# Enhancement chips

**Family 11 — "more horsepower, when the base hardware isn't enough."** Some
SNES cartridges carry a coprocessor. These examples boot one, prove it runs,
then put it to real work — the branch you reach for late in a project, not on
day one.

Two chips, each a boot-then-work pair:

## The ladder

| Rung | Example | Developer question | Chip |
|------|---------|--------------------|------|
| 11.1 | [sa1_hello](sa1_hello/) | How do I boot the SA-1 and hand data back via I-RAM? | SA-1 |
| 11.2 | [sa1_starfield](sa1_starfield/) | How do I offload real math to the SA-1? | SA-1 |
| 11.3 | [superfx_hello](superfx_hello/) | How do I boot the GSU and run a hardware test? | SuperFX |
| 11.4 | [superfx_3d](superfx_3d/) | How do I render 3D with the SuperFX? | SuperFX |

Boot first (11.1 / 11.3 — prove the chip is alive and talking), then a real
workload (11.2 / 11.4).

## The two chips, in one screen

- **SA-1** — the *same* 65816 ISA as the main CPU, but at **10.74 MHz** and
  with its own I-RAM (`$3000–$37FF`) shared with the main CPU. So your SA-1
  code is ordinary 65816 assembly; the art is the handshake (who writes what,
  when) through shared RAM. See the [SA-1 tutorial](../../docs/tutorials/sa1.md).
- **SuperFX / GSU** — a *custom RISC* processor for bitmap and 3D work. It has
  its own ISA (no C compiler — the GSU code is assembly, `.sfx` sources
  assembled by `wla-superfx`), renders into cartridge RAM, and DMAs the result
  to VRAM. See the [SuperFX tutorial](../../docs/tutorials/superfx.md).

Both are validated natively by luna, which detects and runs the SA-1 and the
GSU in the headless test harness — no chip-ROM side channel needed.
