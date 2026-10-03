# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.31.0 (2026-10-03).

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| - | (empty: the audio comparison request went out on 2026-10-03 in `2026-10-03_to_luna_messages-region_reply.md`, §2) | - | - |
| - | (done 2026-10-03 at the v1.32.0 pin: the three runs of the diff-audio reply replayed on the pinned binary, rule written in `testing.md`) | | |
| 2026-10-03 | `luna state --dsp-trace` writes `spc_cycles = 0` on every line (seen on `games/likemario`, 80 589 lines, and `audio/echo`, 92 lines) — so the order of a voice's KON against its volume, pitch and ADSR writes cannot be timed; the examples auditor could not settle whether SNESMOD sets voice 7 up after KON | v1.31.0 | `luna state examples/audio/snesmod_sfx/sfx.sfc --until-frame 800 --dsp-trace sfx.csv --input "100:0x80,104:0,200:0x8000,…"` | 🟡 hypothèse non tranchée | pilote SNESMOD (amont). À joindre au suivi `mukunda-/snesmod` si le corpus confirme. |` with a non-zero `spc_cycles` per row |
