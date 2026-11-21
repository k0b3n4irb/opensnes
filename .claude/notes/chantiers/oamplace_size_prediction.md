# oamPlaceWorld: per-sprite size, prediction written before the code (2026-11-21)

Lot decided 2026-11-21 (resume note, "Next lot"): a consumer sorts sprites of
two sizes (16 and 32) into ONE depth order; one size per batch breaks it. This
note is committed before any assembly; the measured figures are added at the
end, against these.

## Design (chosen before the code)

- Two fields appended to `OamWorldBatch`, offsets 0-27 unchanged:
  - `const u8 *big` at 28: optional, one byte per sprite (indexed by the
    sprite, as tile / attr / visible): 0 = small, any other value = large.
    0 (absent) = today's path, exactly.
  - `u8 size_big` at 32: the large size in pixels (width and height), used
    only with `big`. `size` stays the small one.
  - A static batch written with 8 or 9 initialisers leaves both at 0.
- With `big`, the call writes the size bit of every slot of the batch in the
  high table: 1 for a placed sprite whose `big` byte is not 0, 0 for a placed
  small one AND for every hidden slot (a hidden entry is small: x 257, y 240,
  the smallest footprint). Without `big`, size bits untouched (as today).
- Dispatch: one test of the 24-bit `big` pointer at entry, before the data
  bank is given back; non-zero jumps to a separate path. The single-size path
  is otherwise not touched (same instructions, same order).
- The mixed path: always the group path (groups of eight OAM entries, ninth
  bits and size bits gathered in ACC, one word written a group); per sprite
  `big[spr]` is read first and selects one of two copies of the screen test
  (two sets of size-1 constants in the direct page: the small set where it is
  today, the large set in the last 10 bytes of `tcc__lf`, the `big` pointer in
  `tcc__r10` once the setup has read the size). Only the checked copy (end
  test before each sprite) is emitted, not the unchecked copy of whole groups:
  half the code for about 7 CPU cycles a sprite. Four shapes (order or not,
  visible or not), as the single-size path.
- Line bounds (optional ask 2): NOT planned. The direct page has no room left
  for the two extra y constants per size without moving the ABI's scratch, and
  reaching them through the single-size path would cost it a second test. To be
  confirmed or refuted by the code, noted at the end.

## Predictions (master cycles, SlowROM, devtools/placebench)

References today: single size, large batch, 939 a placed sprite, 588 a hidden
one; a one-sprite call 3 181 placed.

| Figure | Predicted |
|---|---|
| single-size path, per sprite (placed, hidden, all four shapes) | unchanged, ± 5 |
| single-size path, per call (the dispatch: lda / ora of 3 bytes and a branch) | + 90 ± 25 |
| mixed, no order, no visible: a placed small sprite | 1 150 ± 100 |
| mixed, no order, no visible: a placed large sprite | 1 270 ± 100 |
| mixed, no order, no visible: a hidden sprite | 850 ± 100 |
| mixed with order: delta to single size with order, placed small / large / hidden | + 170 / + 290 / + 170 (± 100) |
| mixed: a one-sprite call | 4 600 ± 600 |
| ROM added (four shapes of the mixed path, its setup) | + 5.0 KB ± 0.8 KB (section 7.7 KB today) |

What decides the per-sprite delta (no order): the `big` read and its branch
(about 12 CPU cycles), Y rebuilt from the slot (6), the slot counter in the
direct page instead of Y (about 8), the end test before each sprite (7); for a
large placed sprite the size bit (`lda #` / `tsb`, 10). Against the consumer's
own C, about 2 500 to 2 700 a placed entry (its measure, not ours).

## Measured (filled in after the code)

(pending)
