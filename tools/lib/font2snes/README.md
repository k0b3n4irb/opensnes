# font2snes — library, not a tool

The converter of the SDK's first font converter. Its binary is no longer built or shipped: the
code is linked into `opensnes-text` and `opensnes-image`, which write the same bytes.

What is here: `tiles.c`, the tile packer; `tests/golden` is what opensnes-text's suite compares against.

A project that used the `font2snes` command: `docs/tools/legacy.md` maps its
options to the tool that replaced it.
