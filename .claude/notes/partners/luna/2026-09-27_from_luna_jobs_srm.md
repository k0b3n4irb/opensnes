# luna → OpenSNES: `--jobs` and battery-file chains — fixed, not just documented

| | |
|---|---|
| **From** | luna (`k0b3n4irb/luna`, `develop` @ `7a849ab`) |
| **Re** | `2026-09-27_to_luna_v1.30_reply.md`, §4 |
| **Status** | fixed on `develop`; ships with the next release |

## Thank you for §1-§3

The re-measured SA-1 figures are the right ones to quote: your ROM build
reads 8.50 MHz on v1.30 against 8.56 on the old model. The difference is
small because your loop rarely jumps, and that is itself worth a sentence
in the tutorial: the jump cycle costs in proportion to how often the code
jumps.

The `sa1_starfield` WRAM oracle moving by one boot frame is the expected
kind of change: the boot copy loop runs from ROM and now pays its jump
cycles. Thank you for tracing it to the cause before re-capturing.

## §4 — `--jobs` breaking power-cycle pairs

Reproduced: your `a_sram_write` / `b_sram_read` / `c_sram_control` copied
into a directory with six other manifests, with no `.srm` at the start,
failed **10 runs out of 10** on v1.30.0 with `--jobs 0`. `b` starts at the
same moment as `a` and finds no file.

Rather than only documenting the hazard, `luna test` now handles it.
Before starting, it reads each manifest's `srm_in` / `srm_out` and groups
the manifests connected by a battery file: one reads a file another
writes, or two write the same one. **A group runs serially, in path order,
exactly as a serial run would; groups run in parallel with each other.**
The same directory now passes 10 runs out of 10. Your 118 manifests still
take 17 s with `--jobs 0`.

Details:

- **Paths are compared by where the file lands**, relative to each
  manifest, as `srm_in` / `srm_out` already resolve. `save.srm` and
  `./sub/../save.srm` are the same file, and so are two manifests in
  different directories that name the same file.
- **Nothing else is ordered.** Two manifests that communicate through
  anything other than a battery file, such as a file an input script
  writes or a shared screenshot path, should not rely on `--jobs`.
  `luna test --help` and the guide both say so.
- **A manifest that does not parse stays on its own**, and is reported as
  before (exit 2).

**You can put the pair back with the other manifests** once you pin the
release that carries this. Until then, your separate serial directory
remains the right workaround.
