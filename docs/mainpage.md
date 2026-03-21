# OpenSNES Documentation {#mainpage}

A modern, open-source SDK for Super Nintendo development.
Write game logic in C, produce .sfc ROMs that run on emulators or real hardware.

## Start Here

@subpage getting_started -- Install the SDK and build your first ROM

@subpage learning_path -- 52 examples from "Hello World" to complete games

@subpage examples_by_category -- Browse examples by topic

## Guides

@subpage snes_graphics_guide -- Backgrounds, sprites, modes, VRAM

@subpage snes_sound_guide -- SPC700, SNESMOD, music and SFX

@subpage troubleshooting -- Common problems and solutions

## Tutorials

@subpage tutorial_graphics -- Graphics & Backgrounds

@subpage tutorial_sprites -- Sprites & Animation

@subpage tutorial_animation -- Animation Techniques

@subpage tutorial_scrolling -- Scrolling & Parallax

@subpage tutorial_input -- Controller Input

@subpage tutorial_collision -- Collision Detection

@subpage tutorial_audio -- Audio & Music

@subpage tutorial_game_states -- Game States & Transitions

@subpage tutorial_sa1 -- SA-1 Coprocessor (10.74 MHz second CPU)

## Hardware Reference

@subpage hardware_overview -- CPU, PPU, APU architecture

@subpage memory_map -- Address space layout

@subpage registers -- PPU, DMA, I/O registers

@subpage oam -- Sprite attribute memory

## Examples (Annotated Source Code)

See the @ref example_sources "Example Source Code" page for all 52 examples organized by topic, with links to fully documented source files.

## API Reference

- @ref snes.h "Main Header (snes.h)"
- @ref console.h "Console Functions"
- @ref sprite.h "Sprite/OAM Functions"
- @ref input.h "Input Handling"
- @ref dma.h "DMA Transfers"
- @ref mode7.h "Mode 7 Graphics"
- @ref snesmod.h "SNESMOD Audio"
- @ref registers.h "Hardware Registers"

## Contributing

@subpage code_style -- Code style conventions

@subpage third_party -- Third-party components and licenses

## Acknowledgments

OpenSNES is built upon [PVSnesLib](https://github.com/alekmaul/pvsneslib) by **Alekmaul** and contributors.
We owe them an enormous debt of gratitude for the hardware library, SNESMOD audio engine, asset tools, and decades of SNES development knowledge.

For the full list of dependencies, licenses, and contributors, see [ATTRIBUTION.md](https://github.com/k0b3n4irb/opensnes/blob/develop/ATTRIBUTION.md).

## Links

- [GitHub Repository](https://github.com/k0b3n4irb/opensnes)
- [Issue Tracker](https://github.com/k0b3n4irb/opensnes/issues)
- [SNES Development Wiki](https://snes.nesdev.org/)

## License

OpenSNES is released under the MIT License.
