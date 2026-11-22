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

## Measured (filled in after the code, same day)

`make bench-place` on the worktree, before the change (the references came
back exactly: 939 / 588 / 3 181) and after. SlowROM, master cycles.

| Figure | Predicted | Measured | Verdict |
|---|---|---|---|
| single size, per sprite, four shapes (placed / hidden) | unchanged ± 5 | 939/588, 1 169/844, 1 004/776, 1 235/991: identical (order placed 1 169 both times; FastROM order placed 1 018 -> 1 019) | in |
| single size, per call | + 90 ± 25 | + 108 (one sprite 3 181 -> 3 289; the 32-sprite rows + 108 to + 110; the consumer's two large calls 47 197 -> 47 415) | in (upper half) |
| mixed, no order: placed small | 1 150 ± 100 | 1 086 | in |
| mixed, no order: placed large | 1 270 ± 100 | 1 195 | in |
| mixed, no order: hidden | 850 ± 100 | 768 | in (low edge) |
| mixed with order, delta to single size with order (small / large / hidden) | + 170 / + 290 / + 170 ± 100 | + 103 / + 212 / + 109 (1 272 / 1 381 / 953) | in (low half) |
| mixed: a one-sprite call | 4 600 ± 600 | 6 838 | OUT: + 2 238 |
| ROM added | + 5.0 KB ± 0.8 | + 5 219 bytes (section 7 751 -> 12 970) | in |

The one-sprite call: the prediction counted the mixed setup against the
single-size one-sprite path, but a batch with `big` runs the group setup
(GSETUP, about 230 CPU cycles with its two table reads) on top of the entry,
plus the large constants, the two masks and the shape dispatch (about 50
instructions): roughly 5 700 fixed. Not worth a straight path of its own for
the consumer (its batches are large); the docs tell a lone sprite to use a
one-size batch. The per-sprite figures sit in the low half of their bands:
the CHK test and the slot counter cost less than counted (the slot load also
serves the end test).

Against the consumer's own C (about 2 500 to 2 700 a placed entry, its
measure): 1 086 to 1 447 a placed entry depending on the shape, i.e. 45 to
60 % less on that post, before its own test (hypothesis until it measures).

Faults put in the new path, each caught by both fixtures (world_place bad
bytes; libtests r_worldm red): the size bit never set 9 715; a large
sprite's x tested at the small size 27 598; a hidden slot keeping its size
bit 24 657.

Line bounds (ask 2): not done, as planned. tcc__lf is now full (40 bytes)
and r10 holds the big pointer; two bounds need two more y constants per
size (the stored y and the test no longer share NSZ), i.e. four more words,
or a slower path. The consumer said it can live without; the docs give the
workaround (oamHide() after the call).
