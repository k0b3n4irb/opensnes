# img2snes — library, not a tool

The converter of the SDK's first quantizer. Its binary is no longer built or shipped: the
code is linked into `opensnes-palette quantize` and `opensnes-image`, which write the same bytes.

What is here: `quantize.c` (median cut) and `scale.c`; `tests/golden` is what opensnes-palette's suite compares against.

A project that used the `img2snes` command: `docs/tools/legacy.md` maps its
options to the tool that replaced it.
