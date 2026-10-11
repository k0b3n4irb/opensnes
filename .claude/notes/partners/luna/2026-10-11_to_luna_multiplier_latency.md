# To luna: the 5A22 multiplier answers at once (2026-10-11)

From OpenSNES, luna 1.38.0. Sent as is, by direct message, the same day.
Weight: moyen. Does not block.

## What we saw

A product is complete in `$4216/$4217` as soon as `$4203` is written. The
references say it is built over eight CPU cycles:

- snesdev-wiki, MMIO registers, WRMPYB: "The multiplication process takes
  up to 8 cycles and the result is written to RDMPY as it goes" (chunk
  `67a9f1c5dbc42499`);
- fullsnes, 4202h/4203h: "set WRMPYB, wait 8 clk cycles, then read the
  16bit result" (chunk `4d3930010d15cd22`).

## The piece

`~/workspace/partner-reports/opensnes/pieces/2026-10-11_mulprobe.sfc`
(with its `.sym`, and the routine in `2026-10-11_mulprobe.asm`). It does
0xFF x 0xFF five times and reads `$4216` then `$4217` with `lda.l`, after
0, 1, 2, 3 and 4 `nop`, into `probe[0..9]`; `probe[10]` = 0x5A says the
routine ran.

```sh
luna state ~/workspace/partner-reports/opensnes/pieces/2026-10-11_mulprobe.sfc \
    --until-frame 30 --peek probe:c
```

gives `01fe01fe01fe01fe01fe5a00`: 0xFE01 in all five, the first read being
four CPU cycles after the write. We have no console figure for what the
first pairs should hold; the references only say that it is too early.

## Why it matters to us

`tcc_mul16`, the compiler's 16-bit multiply, waits four `nop` after each of
its three hardware multiplies, then `rep #$20` and `lda $4216`: fourteen
cycles where the references ask for eight. It is 2.5 % of the work of our
example corpus. We wanted to shorten the wait and could not judge it: our
library tests pass with four, two, one and no `nop` at all. We leave the
routine as it is.

## What we ask

That a read of RDMPY (and of RDDIV / RDMPY after `$4206`, sixteen cycles)
before the unit has finished returns what the hardware would — or, short
of the exact intermediate values, that luna says so (a diagnostic, like the
bus-violation count) when a ROM reads them early. Either lets a test fail
on a wait that is too short.

## Closed when

luna models it or flags it, or tells us why it will not.
