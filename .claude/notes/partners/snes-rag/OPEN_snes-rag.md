# OpenSNES → snes-rag: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item; every item is re-run against the
corpus the day the report goes out (`.claude/rules/partners.md`).

| Date | Item | Seen on | What we needed |
|---|---|---|---|
| — | (empty: the verify item is folded into `2026-09-27_to_snes-rag_reply2.md` §2) | | |
| 2026-09-27 | **Unsettled: does a STOP whose IRQ is masked (CFGR bit 7 = 1) still set SFR bit 15?** `snes_search("Super FX GSU SFR bit 15 IRQ flag set on STOP, reset on read; CFGR bit 7 IRQ mask: is the flag still set when the IRQ is masked?", exclude_sources=["opensnes-docs","opensnes-notes-tech"])` → fullsnes `55a5eac1da3d4a44` itself asks "(also set if IRQ masked?)"; the manual §5.4.2 (`a938cb6359382bbd`) and wikibooks (`c4d0afafc3c05afb`) do not say. It decides whether our `gsu_stop_irqs` can count a stale STOP (we tell users to keep the mask when polling). | `snes_sources` 2026-09-27, `bb5dbf5eff5d` | a source that settles it (ares / bsnes superfx code, an MAME driver, a hardware test), or the corpus marking it open |
