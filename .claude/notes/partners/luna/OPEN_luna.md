# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.30.1 (2026-09-30).

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| — | (empty: everything sent up to `2026-09-27_to_luna_v1.30_reply.md`) | | |
| 2026-09-29 | **`[asserts.dma]` stops at 1 000 000 trace events.** `luna test` on a manifest with `frames = 200` and `[asserts.dma] unsafe_writes = 0` for `examples/chips/superfx_game_skeleton` (16 KB of VRAM DMA a frame) → `dma: trace hit its 1000000-event cap — counts would under-report; shorten the run`. Refusing is right; we run such ROMs over 55 frames (`tools/luna-test/vram_dma_blank.py`, `CAP_FRAMES`). Asked: a manifest key for the cap, or counting unsafe writes / per-VBlank bytes without storing events. Low priority. Also a thank-you for the report: `--dma-trace` + `--trace-writes` made every claim of phase D checkable (superfx_runtime.md §5). | `luna test`, v1.30.0 | a cap key or storage-free counters |
