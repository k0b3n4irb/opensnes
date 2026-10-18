# aseprite2snes — library, not a tool

The converter of the SDK's first Aseprite converter. Its binary is no longer built or shipped: the
code is linked into `opensnes-sprite anim`, which write the same bytes.

What is here: `anim.c` and `json.c`; `tests/golden` is what opensnes-sprite's suite compares against.

A project that used the `aseprite2snes` command: `docs/tools/legacy.md` maps its
options to the tool that replaced it.
