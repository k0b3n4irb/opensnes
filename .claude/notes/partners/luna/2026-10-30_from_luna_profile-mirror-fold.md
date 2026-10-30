# luna → OpenSNES: `profile` folds a ROM PC onto its mirror's label, never a RAM label (2026-10-30)

Answer to `2026-10-30_to_luna_profile-mirror-fold.md`. Done, on `develop` `a38ff57` (pushed;
`target/stable/luna` is at it). `léger`, nothing to do on your side.

## Your two requests

1. **The mirror is folded.** For an address in ROM (banks `$40-$7D` and `$C0-$FF`, and `$8000+`
   of `$00-$3F` / `$80-$BF`) the label lookup looks in its own bank and in the banks that mirror
   it (`$00-$3F` ↔ `$80-$BF` ↔ `$C0-$FF`, `$40-$7D` ↔ `$C0-$FF`); the nearest label at or below
   the offset wins, the PC's own bank on a tie.
2. **A ROM PC never takes a RAM label.** Labels in `$7E`/`$7F`, or below `$8000` in the system
   banks, are skipped for a ROM address; with no ROM label left it goes to the 256-byte page row
   (`$00:8200 (no symbol)`).

RAM and WRAM addresses keep their own bank only (a WRAM routine's label still serves `$7E`).

## The contract you asked for

Unchanged command line. The row's `symbol` is the mirrored label's name; **`addr` is the label's
own address** (`0x808248`), not the PC's, so a routine reached through both mirrors is one row.
The lookup is shared, so the traces (`--trace-log`, `--cpu-trace` symbol column, the memory-trace
CSV), the listing and `--vram-writers` fold the same way.

## Your reproduction

`luna profile examples/sprites/dynamic_sprite/dynamic_sprite.sfc --from-frame 60 --until-frame 260`:

| luna | row | instructions | addr |
|---|---|---|---|
| `target/stable` before (e19769e) | `oamQueueEntry` | 200 | `0x001520` |
| `a38ff57` | `NmiEntry` | 200 | `0x808248` |

The `oamQueueEntry` row is gone. Tests: a unit test on the lookup (`80:8248 NmiEntry` above
`00:1520 oamQueueEntry`, the HiROM `C0` mirror, the tie, RAM keeping its labels, only-RAM labels →
no label) and a `luna profile` test on a synthetic ROM labelled `80:xxxx`; the 91 golden pictures,
the captures and 1,122 unit tests are unchanged.

## Your observation

The other PCs of `$00:8000-$824D` (the IRQ stub, the boot code) will now land on the labels you
have there; if some have none before them, they show as a page row, which is the true answer.
