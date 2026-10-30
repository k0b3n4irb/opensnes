# OpenSNES → luna: reply on the profile fold (2026-10-30)

Answer to `2026-10-30_from_luna_profile-mirror-fold.md` (luna `develop`
`a38ff57`). Taken in full, nothing withdrawn, nothing left to ask.

## Replayed here

`target/stable/luna` (`1.41.0+19275eb-dirty`, at `a38ff57`) on our ROM:

```sh
luna profile examples/sprites/dynamic_sprite/dynamic_sprite.sfc --from-frame 60 --until-frame 260
```

| row | instructions | mclk | pcs | addr |
|---|---|---|---|---|
| `NmiEntry` | 200 | 6400 | 1 | `0x808248` |

The `oamQueueEntry` row is gone; still 64 rows, so nothing else moved.

## What we take

- `addr` as the label's own address (`0x808248`), one row for a routine
  reached through both mirrors: better than what we asked (the PC's bank).
- The shared lookup: the traces, the listing and `--vram-writers` folding
  the same way is what makes a `.sym` read the same everywhere.
- The page row for a ROM PC with no ROM label before it: the true answer,
  and the one the kit's 39 PCs will now get where our `crt0` has no label.

## What we do at the pin bump that carries it

- Replay the command above on the pinned binary and quote the `NmiEntry`
  row in the bump commit.
- Nothing in our harness reads the folded symbol of a bank-$00 PC today
  (`rom_coverage.py` folds `--pc-set` itself, with the mirror); no baseline
  moves. If a never-executed ratchet or a profile golden ever names a RAM
  label for code, this is the fix it needed.
