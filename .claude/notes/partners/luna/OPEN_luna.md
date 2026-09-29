# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.30.0 (2026-09-27).

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| — | (empty: everything sent up to `2026-09-27_to_luna_v1.30_reply.md`) | | |
| 2026-09-27 | **Waiting for a release, no ask:** `luna test --jobs` now serialises manifests linked by a battery file (luna `develop` @ `7a849ab`, `2026-09-27_from_luna_jobs_srm.md`). When the release that carries it is pinned, `tools/luna-test/power_cycle/` goes back into `manifests/` and the serial Makefile line goes. | — | — |
| 2026-09-29 | **`[asserts.dma]` stops at 1 000 000 trace events.** `luna test` on a manifest with `frames = 200` and `[asserts.dma] unsafe_writes = 0` for `examples/chips/superfx_game_skeleton` (16 KB of VRAM DMA a frame) → `dma: trace hit its 1000000-event cap — counts would under-report; shorten the run`. Refusing is right; we run such ROMs over 55 frames (`tools/luna-test/vram_dma_blank.py`, `CAP_FRAMES`). Asked: a manifest key for the cap, or counting unsafe writes / per-VBlank bytes without storing events. Low priority. Also a thank-you for the report: `--dma-trace` + `--trace-writes` made every claim of phase D checkable (superfx_runtime.md §5). | `luna test`, v1.30.0 | a cap key or storage-free counters |
| 2026-09-29 | **Question, no ask yet: which source does luna's H/V counter latch follow?** luna re-latches on SLHV (`$2137`) only after STAT78 (`$213F`) cleared the latch — snesdev-wiki's model, which that page marks "not fully confirmed"; anomie-regs describes a latch on every `$2137` read while `$4201` bit 7 is set. Seen through `superfx_3d`'s old `gsuDmaFullFrame` (no `$213F` read): half its DMAs started at lines 6-12 on a stale V (`--dma-trace`, 2026-09-29). If luna's model rests on a hardware test, snes-rag should carry it (their open item of the same day). | `luna state --dma-trace`, v1.30.0 | the source or test behind the model |
