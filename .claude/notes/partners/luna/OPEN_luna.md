# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.28.0 (2026-09-27).

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| 2026-09-27 | **Super FX: a program run from the code cache loses Game Pak RAM writes that the same program, run from ROM, makes.** Program `devtools/libtests_gsu/gsu_job.sfx` (42 bytes, no CACHE / LJMP / ROM read): a 600 000-iteration add loop, then `STW` of the sum to `$70:0000`, of R4 (0) to `$70:0002`, of `$C0DE` to `$70:0004`, `NOP`, `STOP`. Loaded by the CPU into `$3100` (SFR = 0 first, last line padded with `$01`, as krom's GSUCACHEINJECT does), started with R15 = 0 and SCMR = RAN only: RAM after the job `40 16 00 00 de 00`. The same binary started from ROM (`gsuLaunch`, SCMR RAN+RON): `40 16 00 00 de c0`. Same `gsu.instructions_executed` (2 401 219) in both. The marker's high byte is missing; a further `STW $BEEF` to `$70:0006` after it is not written at all; 3 to 12 `NOP`s before `STOP` change nothing, nor do three frames of delay before the CPU clears SCMR. The Nintendo manual (Book II 7.2.1) says a program in cache or ROM writes RAM through the buffer "while the subsequent program is being executed", nothing about STOP dropping writes; we have not run it on hardware. | v1.28.0, 2026-09-27: `luna state --until-frame 60 --peek 70:0000:6 --out - cached.sfc` vs `… fromrom.sfc` (ROMs in `/tmp/luna_repro_gsu_cache_2026-09-27/`, rebuilt by `make -C devtools/libtests_gsu` on `wip/superfx-runtime-c`, the second with `gsuSetProgram(gsu_job); gsuLaunch();` in place of the two cache calls) | the same RAM content from both paths, or the hardware rule that makes them differ |
