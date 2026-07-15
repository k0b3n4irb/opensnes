# PeterLemon (krom) ports — the behavioral-parity validation pattern

**Rule: a port is validated by MEASURING both ROMs in luna, never by reading
the reference ASM alone.** The hdma_wave_table port shipped THREE times
before being faithful: (1) an invented 112-line wave period (reference:
~26); (2) direction/speed claims derived from reasoning instead of
observation; (3) the right wave on the WRONG canvas — 4-color Mode 1
noise where the original waves a full-screen 256-color Mode 3 image,
because only the reference's main loop had been read, not its INIT.

## The pipeline (all pieces exist, no new tooling)

1. **Run the reference ROM in luna.** krom's ROMs fail header autodetection
   (checksum) — `--force-mapper lorom` (available on `luna frames` and
   `luna state`; NOT on `luna run`).
2. **Capture exactly-consecutive frames from BOTH ROMs**:
   `luna frames -n 3000000 -c 30 --out-dir <dir> [--force-mapper lorom] <rom>`.
3. **Read the WHOLE reference ASM, init included** — video mode, BPP,
   VRAM layout, asset sizes (krom's WaveHDMA: Mode 3, 57 KB of unique
   8bpp tiles, map at $F800). The effect code is half the demo; the
   canvas it runs on is the other half, and "own art" means own PIXELS
   in the SAME configuration, not a different configuration.
4. **Extract the reference's actual DATA, not its vibe**: tables, constants
   and update cadence come from the ASM source verbatim
   (e.g. `grep '^db 1; dw ' WaveHDMA.asm` → the wave samples and their real
   period). Never re-invent values that exist in the original.
5. **Compare the GENERATED ASM to the original, register by register**:
   read the port's `main.c.asm` + the lib routines it calls and confront
   every hardware write with the reference's (DMAP/BBAD/A1T/A1B/HDMAEN,
   BGMODE/BGxSC/TM...). Document the table in the example README, including
   deliberate differences (e.g. full hdmaSetup re-run vs single-register
   rewrite) and equivalences (VRAM mirrors, no-op bits).
6. **Measure the effect on both frame sequences with the same script**
   (numpy + PIL are available). For displacement effects, the working
   estimator: per-scanline horizontal shift between two frames via integer
   SSD correlation on a central slice, then compare displacement-field
   profiles across frame distances (direction = sign of the vertical
   profile shift; speed = shift/Δframes; period = profile autocorrelation;
   amplitude = temporal peak-to-peak per line over ≥ one cycle).
7. **Acceptance = a parity table**, e.g. (hdma_wave_table, 2026-07-12):
   period 25 vs 26 lines, amplitude p2p 19.5 vs 20.0 px, speed ~1 vs 1
   line/frame, direction identical — measured, cited in the example README.

## Traps hit while building this (don't rediscover them)

- **Periodic procedural art breaks the measurement AND the eye**: fixed
  8px stripes make every measured shift ambiguous modulo 8 (the SSD ties
  resolve arbitrarily) and hide horizontal drift bugs. Use aperiodic
  texture (LCG-seeded streak tiles — deterministic, zero assets).
- **Integer-rounded per-frame deltas cumsum to a biased trajectory**
  (true per-frame shift < 1px rounds to 0): for speed, correlate the
  displacement profile between DISTANT frame pairs (model-free), don't
  cumulate consecutive-pair integers.
- `--peek 00:4302` (HDMA A1T0L) reads 0 — DMA registers are not readable
  through luna's peek path; register-level comparison is a dead end,
  measure pixels.
- `luna run` has `--print-fbhash` but no `--force-mapper`; the reference
  ROM therefore can't go through the fbhash path — use `frames`.

## Checklist per port

- [ ] Reference ROM runs in luna (`--force-mapper lorom`), eyeballed.
- [ ] FULL reference ASM read: init (mode/BPP/VRAM/assets) AND effect loop.
- [ ] Reference data extracted from the ASM (tables/constants/cadence).
- [ ] Generated-ASM register comparison table written into the README.
- [ ] Both ROMs captured with `luna frames` (same warm-up, ≥1 effect cycle).
- [ ] Parity table measured with the same script; cited in the README.
- [ ] Side-by-side stills eyeballed (character match, not just numbers).
- [ ] Own art, aperiodic if the effect is a displacement; krom credited.
- [ ] FINAL build rendered and LOOKED AT after the last edit (not just
      measured mid-iteration — the wave port shipped a broken commit
      after all measurements passed because nobody looked again).
- [ ] Fresh-checkout build proof (git worktree add + make): every
      committed file present — the gitignore catch-alls shipped 4 broken
      commits before being fixed (c7ddf12d).
- [ ] CI green BEFORE announcing the port done.
