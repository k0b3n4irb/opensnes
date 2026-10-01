# Probe: does the hardware multiplier misread during auto-joypad?

`make` here, then:

```
../../../../tools/luna-test/bin/luna state --until-frame 600 \
  --peek r_ok_busy:2 --peek r_bad_busy:2 --peek r_ok_idle:2 --peek r_bad_idle:2 \
  --out - mulprobe.sfc
```

Main thread only, NMI off (auto-joypad on): 2000 times `123 x 45`, read
after > 8 CPU cycles, classified by HVBJOY bit 0 sampled right after the
read. luna v1.30.2, 2026-10-02: `r_ok_busy` 19, `r_bad_busy` 0,
`r_ok_idle` 1981, `r_bad_idle` 0. Negative control: `cpx #5536` instead
of `#5535` flags all 2000. An emulator's answer, not a console's; it
retires our claim of a coupling (KNOWN_LIMITATIONS, fixMul entry).
