# opensnes-save — battery save files {#tools_opensnes_save}

The save tool of the 1.x family (@ref tools_conventions). A `.srm` is the
raw image of a cartridge's save RAM: what an emulator writes beside the
ROM, and what luna reads and writes with `srm_in` / `srm_out` in a test
manifest. `opensnes-save` makes one,
reads one, patches one and compares two — so a test can start from a
prepared save instead of playing up to it, and a run's save can be read
without a hex editor.

The file has no structure of its own: offsets are the ones your game hands
to `sramSaveOffset()` / `sramLoadOffset()` (@ref tutorial_sram).

## New

```sh
opensnes-save new game.sfc              # -> game.srm, the size the ROM declares
opensnes-save new game.sfc --fill 255   # a save RAM that is not zeros: does the game tell a blank save from a real one?
```

The size is read from the ROM header: `$FFD8` (1 KB << n: 8 KB with the
default `SRAM_SIZE := 3`, 32 KB of BW-RAM on SA-1), or, for a Super FX
cartridge, the expansion RAM declared at `$FFBD`, which the battery keeps
(64 KB by default) — the snesdev wiki's *ROM header* page is the reference
for both bytes. A ROM that declares no save RAM is refused, with the
Makefile line that adds it (`USE_SRAM := 1`).

## Inspect, get, set, diff

```sh
opensnes-save inspect game.srm --rom game.sfc
# game.srm: 8192 bytes, 6 written (offsets 0 to 7), the rest $00

opensnes-save get game.srm --at 0 --count 8
# 09 00 0A 00 34 12 78 56

opensnes-save set game.srm --at 0 --hex "09 00 0A 00 34 12 78 56"
opensnes-save set game.srm --at 256 --from slot2.bin

opensnes-save diff before.srm after.srm
# offset 4, 2 bytes: 34 12 -> 35 12
```

- **`inspect`** says how much of the file is written and where; with
  `--rom` it refuses a save whose size is not the one that ROM declares (a
  save of another build, or of another game).
- **`get`** prints bytes in hex, or writes them to a file with `--to`.
- **`set`** patches bytes in place, from `--hex` or from a file; a write
  that would pass the end of the file writes nothing.
- **`diff`** lists the byte ranges in which two saves differ, and exits 1
  when they do — a script can use it as an assertion.

Every subcommand takes `--json`.

## A prepared save in a test

```toml
# test/load_slot1.toml — the game boots with a save already on the cartridge
rom = "../game.sfc"
frames = 120
input = "30:0x8000"          # B: load slot 1
srm_in = "slot1.srm"         # made once: opensnes-save new + set, committed with the test

[asserts.blocks]
loaded = "09000A0034127856"
```

The save is a file of the project, under version control like the manifest
that reads it. When the save's layout changes, `opensnes-save get` on the
old file and `set` on a new one is the migration, in a script.

## What it does not do

It does not know your save's layout: no checksum, no version field, no
slot table. Those belong to a save format, which the SDK does not define
yet — the `sram` module copies bytes. When it does, this tool will verify
and migrate it.

## See also

- @ref tutorial_sram — the `sram` module: sizes, mappings, return codes.
- @ref tools_luna — `--srm-in` / `--srm-out`, and the manifest keys.
