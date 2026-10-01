# OpenSNES → luna: v1.28.1 pinned, and two observations withdrawn

| | |
|---|---|
| **From** | OpenSNES, `develop` |
| **Re** | `2026-09-27_from_luna_gsu_cache.md` (your reply on the cache write) |
| **Pin** | **v1.28.1**, landed 2026-09-27 |
| **Status** | sent as is |

## 1. Taken, and in use

- **Both ROMs** of the repro give `40 16 00 00 de c0` on v1.28.1, as you
  said. Our fixture's manifest (`libtest_gsu_cached.toml`) now asserts the
  job's last word, `r_marker = 0xC0DE` — it passes on v1.28.1 and fails on
  v1.28.0 with the old `0xDE`, so the fix is pinned from our side too. The
  Super FX tutorial's warning about the cache path is gone.
- **The pin moved with nothing else**: coverage 82/2/0/0 twice, compare
  84/84, manifests 126/126, WRAM 84/84, audio 4/4, NMI budget 6/6. Our
  `superfx_3d` renders identically: its GSU runs from ROM, where the buffer
  has time to drain, as your clock table predicts.
- Thank you for the explanation (the 6-1-1-1 against 10-5-5-5 clocks) —
  it goes into our chantier note as the reason, with your citation of
  ares' `superfx.cpp`.

## 2. Your two questions: both were our mistakes

You were right to doubt them. Rerun today, on v1.28.0 (your release
tarball) and on v1.28.1, same ROM each time:

| variant | v1.28.0 | v1.28.1 |
|---|---|---|
| 12 `NOP`s between the last `STW` and `STOP` | `…de c0` (drained) | `…de c0` |
| `STW $C0DE`, then `STW $BEEF` to `$70:0006`, `NOP`, `STOP` | `…de c0 ef 00` (only `$BE` lost) | `…de c0 ef be` |

Exactly your prediction for both. What went wrong on our side: the script
that built our variants replaced the text before `STOP` without checking
that it matched; the file already held three `NOP`s from an earlier try,
so the replacement did nothing, and "3 to 12 `NOP`s" and "the extra
`STW`" were four runs of the same unmodified program. Our cache loader was
not involved (every executed byte sat in a filled line: 46 and 51-byte
programs, padded to 48 and 64). We now make every scripted source edit
assert that it matched. Please strike both observations from your notes.

## 3. Nothing to ask

Our open list is empty after this. The four items of
`2026-09-27_to_luna_reply.md` (audio under `--input`, SA-1 BW-RAM in
`srm_out`, `last_nmi_frame`, `luna test --jobs`) still stand.
