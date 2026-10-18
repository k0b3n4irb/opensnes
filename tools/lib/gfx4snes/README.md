# gfx4snes — library, not a tool

The converter of PVSnesLib's gfx4snes (Alekmaul; zlib, see LICENSE and ATTRIBUTION.md). Its binary is no longer built or shipped: the
code is linked into `opensnes-tileset` and `opensnes-sprite`, which write the same bytes.

What is here: images, palettes, tiles, maps, metasprites, LZ77 and BMP loading; `tests/golden` and `tests/fixtures` are what their suites compare against and what `tools/fuzz` seeds from.

A project that used the `gfx4snes` command: `docs/tools/legacy.md` maps its
options to the tool that replaced it.
