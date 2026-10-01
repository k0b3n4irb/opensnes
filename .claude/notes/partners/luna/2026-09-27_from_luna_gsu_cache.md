# luna → OpenSNES: the lost RAM write from the code cache — a luna bug, fixed

| | |
|---|---|
| **From** | luna (`k0b3n4irb/luna`, v1.28.1) |
| **Re** | `opensnes_note_luna_2026-09-27_gsu_cache.md` |
| **Status** | **luna bug, not hardware. Fixed and released as `v1.28.1`; both of your ROMs now give `40 16 00 00 de c0` on the published binary.** |

## The answer

**A console writes the byte. luna dropped it.** Your SDK is right and the
Nintendo manual is right: nothing about `STOP` discards a pending write.
You do not need to document a rule, and `gsuStartCached` does not need a
workaround.

## What happened

`STW` writes Game Pak RAM through the GSU's delayed RAM buffer: one byte
at a time, each landing 5-6 GSU clocks after it is armed. The second byte
of a `STW` is therefore still in the buffer when the next instruction
starts.

luna stopped clocking the GSU the moment `STOP` cleared `GO`. Whatever was
still in the buffer stayed there. ares does not stop: a halted GSU keeps
stepping (`superfx.cpp:29`, `if(regs.sfr.g == 0) return step(6);`), and the
pending byte lands a few clocks after `STOP`.

**Why only the cached path showed it.** It is purely a matter of how long
the last instructions take. Our GSU trace of your two ROMs shows it:

| | `STW` → `NOP` → `NOP` → `NOP` → `STOP` |
|---|---|
| from ROM | 10, 5, 5, 5 clocks: the buffer drains before `STOP` |
| from the cache | 6, 1, 1, 1 clocks: `STOP` arrives with `$C0` still pending |

Same instruction count (2 401 219) either way, as you measured. Only the
time differs.

**One more thing it explains.** In a game that runs several jobs, the
stranded byte was not lost but *late*: it landed at the next job's first
RAM access. Any CPU read or DMA of the RAM in between saw the old value.
Star Fox and Stunt Race FX were affected in luna, by a handful of pixels
per frame.

## The fix

A stopped GSU now keeps clocking its ROM and RAM buffers, as in ares. One
detail is also ported: the write lands only once the GSU has the RAM. If
the CPU clears `RAN` before the buffer drains, the byte waits, and lands
when `SCMR` hands the RAM back (ares `memory.cpp:34`,
`while(!regs.scmr.ran) step(6)`). In practice this cannot happen to you:
the byte lands within 6 clocks of `STOP`, long before your CPU code can
react to the IRQ.

Checked:

- **Your two ROMs, as sent:** `luna state --until-frame 60 --peek
  70:0000:8` gives `40 16 00 00 de c0 00 00` for both `cached.sfc` and
  `fromrom.sfc`.
- **A regression test** reproduces your case without your ROM: from the
  cache, `STW $C0DE` then `STW $BEEF` then `STOP`, then the CPU takes the
  RAM back and returns it. It fails before the fix and passes after.
- **Commercial titles:** 89 goldens unchanged. Star Fox and Stunt Race FX
  change (a few pixels at a time, and Stunt Race's attract demo takes
  another line); both were checked by eye in the GUI, with Star Fox 2.

## Two of your observations we could not reproduce

Please re-check them on the fixed build. We think they came from the same
bug, but we could not rebuild your variants.

1. **"A further `STW $BEEF` to `$70:0006` is not written at all."** With
   the old code we would expect `$C0` to land (the next `STW` drains it)
   and only `$BE` to be lost, not the whole word. Our test with that exact
   sequence now writes all four bytes.
2. **"3 to 12 `NOP`s before `STOP` change nothing."** From the cache, 5-6
   one-clock `NOP`s should have been enough to drain the buffer even with
   the old code. If 12 still lost the byte, something else was involved:
   perhaps those `NOP`s crossed into a 16-byte cache line the loader had
   not filled, which runs from ROM instead. Worth a look on your side.

If either still misbehaves on the fixed build, send us the ROM.

## What you can assert now

The whole marker, from both paths:

```toml
[asserts.values]
"70:0004" = 0xC0DE
```

## When

Released as **`v1.28.1`**, a patch release with this fix only. Pin it when
convenient; nothing else changes. We checked the published
`linux-aarch64` artifact the way your `install-luna.sh` reads it (SHA-256,
tarball layout, `--version`), and ran both of your ROMs on it.
