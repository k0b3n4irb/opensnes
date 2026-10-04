# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.31.0 (2026-10-03).

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| - | (empty: the audio comparison request went out on 2026-10-03 in `2026-10-03_to_luna_messages-region_reply.md`, §2) | - | - |
| - | (done 2026-10-03 at the v1.32.0 pin: the three runs of the diff-audio reply replayed on the pinned binary, rule written in `testing.md`) | | |
| 2026-10-03 | `luna state --dsp-trace` writes `spc_cycles = 0` on every line (seen on `games/likemario`, 80 589 lines, and `audio/echo`, 92 lines) — so the order of a voice's KON against its volume, pitch and ADSR writes cannot be timed; the examples auditor could not settle whether SNESMOD sets voice 7 up after KON | v1.31.0 | `luna state examples/audio/snesmod_sfx/sfx.sfc --until-frame 800 --dsp-trace sfx.csv --input "100:0x80,104:0,200:0x8000,…"` with a non-zero `spc_cycles` per row | **Réglé sur luna `develop` `33116ad`** (vérifié le 2026-10-05 sur leur binaire : 92 lignes, `spc_cycles` croissants, 95632, 95642, …) ; à fermer à l'épinglage de la version qui le porte |
| 2026-10-05 | `rom.checksum_valid` in `luna state` is true as soon as checksum XOR complement == 0xFFFF; it does not sum the ROM. A copy of `print_string.sfc` with byte $0100 flipped (true sum 0xB01F, header 0xAF40) still reports `checksum_valid: true`, `checksum: 44864`. Needed: `checksum_valid` = header sum equals the computed sum (with the usual mirroring of a non-power-of-two tail), or a separate `checksum_computed` field. Our corpus gate (`luna_runner.py --coverage`, `header_problem`) reads this field and can only catch an inconsistent pair until then | v1.32.0 | `cp print_string.sfc bad.sfc; printf '\xff' \| dd of=bad.sfc bs=1 seek=256 conv=notrunc; luna state bad.sfc --until-frame 0 --out - \| jq .rom` |
