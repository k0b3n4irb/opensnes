# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.24.0 until the bump to v1.27.0 lands.

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| 2026-09-26 | **R5 — SA-1 parity with the GSU profile.** `state.sa1` is `{pc, pb, p, running}`; `profile` has no SA-1 key (`budgets, end_frame, entries, frames, from_frame, instructions, rom, total_mclk`). Our SA-1 tutorial claims "10.74 MHz, 3×"; the corpus (higan speed table `7c52e450eef77f7d`) gives 10.74 MHz only for I-RAM code against a WRAM-bound S-CPU, ~5.4 MHz ROM against ROM — the case of our `sa1_starfield`. We cannot measure which one we are in. | v1.24.0 (`luna state examples/chips/sa1_starfield/sa1_starfield.sfc --out -`, `luna profile --help`). To re-check on v1.27.0 before sending. | `state.sa1.instructions_executed`, and in `profile` a `sa1` block with instructions, cycles, and cycles lost to bus conflicts with the S-CPU (the SA-1 analogue of `stall_cycles`). Same shape as `gsu.per_job` minus the jobs. |
| 2026-09-26 | **`--superfx-trace-from <frame>`** — you offered it ("say the word"). We say it, for phase C of the Super FX runtime (IRQ on STOP), where the job of interest starts late in the run. | reply of 2026-09-25 §4 | `luna run superfx_3d.sfc --superfx-trace out.csv --superfx-trace-from 300` |
