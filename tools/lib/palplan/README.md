# palplan — library, not a tool

The converter of the SDK's first palette planner. Its binary is no longer built or shipped: the
code is linked into `opensnes-palette plan`, which write the same bytes.

What is here: no source (the planner is opensnes-palette's own); `tests/golden` is what opensnes-palette's suite compares against.

A project that used the `palplan` command: `docs/tools/legacy.md` maps its
options to the tool that replaced it.
