# tmx2snes — library, not a tool

The converter of the SDK's first Tiled converter. Its binary is no longer built or shipped: the
code is linked into `opensnes-level`, which write the same bytes.

What is here: `level.c`, the converter; `tests/golden` is what opensnes-level's suite compares against.

A project that used the `tmx2snes` command: `docs/tools/legacy.md` maps its
options to the tool that replaced it.
