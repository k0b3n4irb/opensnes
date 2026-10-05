# opensnes — the project tool {#tools_opensnes}

The one command of the SDK that is about your *project* rather than about
an asset: it creates a project, builds it, runs it, tests it, and checks
your installation. It is a compiled program like the rest of the family
(@ref tools_conventions) — nothing to install beside the zip, on Linux,
macOS or Windows.

`opensnes` lives in the SDK's `bin/`; put that folder on your `PATH`, or
call it by its path. It finds the SDK from `$OPENSNES_HOME` if you set it,
then from its own place (`<sdk>/bin/opensnes`), then by walking up from the
current folder.

## Commands

```sh
opensnes init my-game --template game   # a new project folder
cd my-game
opensnes build                          # make, in the project's folder (or any folder below it)
opensnes run                            # build, then open the ROM in an emulator
opensnes test --update                  # first time: record the visual baselines
opensnes test                           # run test/*.toml in luna
opensnes clean
opensnes doctor                         # is everything there?
```

| Command | What it does |
|---|---|
| `init <name> [--template blank\|game]` | writes `main.c`, a `Makefile` and `res/`; `blank` is a text screen, `game` a sprite you steer with two luna tests in `test/`. The name is the ROM's (`<name>.sfc`): letters, digits, `-`, `_`, `.`. A path works (`init ~/projects/my-game`). |
| `build [--clean]` | runs `make` in the project — the build itself is `make/common.mk` (@ref tools_build). `--clean` rebuilds from nothing. |
| `clean` | removes what the build made |
| `run [--emulator NAME]` | builds, then opens the ROM (the `TARGET` of your Makefile) in the luna GUI that `scripts/install-luna.sh` installed, or the first of Mesen, bsnes, snes9x on your `PATH`; `--emulator` names another |
| `test [--update]` | `luna test` on the project's `test/*.toml`; `--update` rewrites the `asserts.fbhash` baselines |
| `doctor [--json]` | checks the SDK, the host C compiler (`cc`, which the compiler's preprocessing stage uses), `make`, the compiler and the tools of `bin/`, the built library, luna and an emulator. Exit 1 if something a build needs is missing, so a script can gate on it. |
| `upgrade [-q] [--removed-only] <folder-or-file>...` | lists, in your sources, the names OpenSNES 1.0 removed, with what to use instead, and the calls that kept their name and changed meaning (@ref upgrading). Exit 1 on a hit. The build runs it for you on a source that fails to compile. |

`build`, `clean`, `run` and `test` work from any folder of the project: the
project is the nearest folder above whose `Makefile` includes
`make/common.mk`.

## It is still make underneath

`opensnes build` is `make` with `OPENSNES_HOME` set; a project is a
`Makefile` of six lines and you can call `make`, `make test` and
`make clean` yourself, from an editor or a CI job. The tool is the short
way in, not a layer you depend on.

## See also

- @ref getting_started — the first project, step by step.
- @ref tools_build — every knob of a project's Makefile.
- @ref tools_luna — the emulator, the debugger and the test runner.
