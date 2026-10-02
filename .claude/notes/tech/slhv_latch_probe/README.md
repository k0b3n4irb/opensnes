# Probe: does SLHV ($2137) re-latch the H/V counters without STAT78?

`make` here, then
`../../../../tools/luna-test/bin/luna state --until-frame 20 --peek r_first:2 --peek r_second:2 --out - latch.sfc`.
luna v1.30.2: `r_first` 233, `r_second` 117 — a second `$2137` read with no
`$213F` in between latches a fresh line (anomie-regs' reading;
snesdev-wiki describes a latch only STAT78 re-arms, "not fully confirmed").
An emulator's behaviour, not a console's. Sent to snes-rag 2026-10-02.
