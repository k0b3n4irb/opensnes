# Profiling: measuring where the frame goes {#tutorial_profiling}

@ref craft_frame_budget explains what fits in 1/60th of a second. This page is
the other half: **how to find out where your frame actually went**, so an
optimisation is a decision backed by a number instead of a hunch.

There are two instruments, and they answer different questions.

| Instrument | Lives | Answers |
|---|---|---|
| `luna profile` | outside the ROM | Which symbol burned the cycles, and what its worst frame cost |
| `<snes/profile.h>` | inside the ROM | Where a section of *my own* code starts and ends, live, on screen |

Reach for the emulator first. It needs no code change, it sees every routine
including the library and the runtime, and its numbers are master cycles the
machine actually paid.

## The unit: master cycles per frame

luna reports **master clocks** (mclk). One NTSC frame is about **357 400**
of them:

| Quantity | Value |
|---|---|
| Master clock | 21.477 MHz |
| Frames per second | 60.1 |
| Master cycles in one frame | ≈ 357 400 |
| Master cycles in VBlank (~38 lines of 262) | ≈ 51 800 |

Those two last rows are the numbers to hold in your head. A routine whose
worst frame costs 30 000 mclk has eaten 8 % of the frame; one that costs
400 000 cannot finish inside a frame at all.

## Profiling from outside: luna profile

```bash
tools/luna-test/bin/luna profile examples/sprites/sprite_swarm/sprite_swarm.sfc \
    --until-frame 60
```

```
profile: frames 0..60 (60 completed), 534807 instructions, 21441784 master cycles, 120 symbol(s)
      %            mclk         instr   idle%     pcs   max/frame  symbol
 33.23%         7125282        208581    0.0%     125      143678  main@if_false.95
 20.72%         4443238        136374    0.0%      83       87958  main@for_body.85
 16.30%         3495376           520   99.5%       9      307120  WaitForVBlank
  4.10%          879998         16464    0.0%      84      474000  Start
```

Every column earns its place:

- **%** and **mclk** — share of the whole run, and the master cycles behind it.
  This is where the time went in total.
- **instr** — instructions retired. Compare it with mclk: a row with few
  instructions and many cycles is waiting on something, not computing.
- **idle%** — how much of those cycles the CPU spent stopped (`wai`). The
  `WaitForVBlank` row above is 99.5 % idle: it is not slow, it is *sleeping*
  until the next frame. Idle time is budget you still have.
- **pcs** — distinct program counters seen in that symbol. A large number on a
  small routine usually means the label swallowed the routines after it.
- **max/frame** — **the number that decides whether you ship**. The average is
  comfortable; the worst frame is what the player sees as a stutter.
- **symbol** — the nearest `.sym` label, so compiler-generated block labels
  (`main@if_false.95`) point at a specific branch of your C, not at the
  function as a whole.

Two flags make the report describe the *game* rather than the boot sequence:

```bash
luna profile game.sfc --from-frame 120 --until-frame 300 \
    --input "150:START 160:RIGHT 200:RIGHT|A"
```

`--from-frame` discards the warm-up (loading screens, audio driver upload —
the `Start` row above costs 474 000 mclk in its one frame and would otherwise
dominate), and `--input` drives the ROM into the state worth measuring.

For anything beyond reading, add `--out -` for the full JSON report (every
symbol, not just the top rows) or `--top 60`.

## Gating a budget in CI

A measurement you take once decays. `--budget` turns it into a check:

```bash
luna profile game.sfc --until-frame 300 --budget NmiHandler=6000
```

The symbol's **worst completed frame** must stay under the given master-cycle
count. Exit status is the contract: **0** under budget, **1** over it, **2**
for a symbol that does not exist (a typo fails loudly instead of passing
silently). The flag repeats, so several routines can be gated in one run.

@warning Pick the symbol with the profile in front of you. On this SDK the NMI
handler is emitted as five labels — `NmiHandler` plus four internal ones — so
`--budget NmiHandler=…` measures only the entry stub (652 mclk on
`sprite_swarm`), not the handler's real cost. Until luna folds child labels
into their parent, budget a symbol that is genuinely one routine, and read the
handler's cost by adding its rows in the JSON report by hand.

## Profiling from inside: `<snes/profile.h>`

The library's own profiler answers a question the emulator cannot: *where does
my code sit on the screen right now?* Add the module first:

```makefile
LIB_MODULES += profile
```

**Colour bars** are the classic SNES technique: paint the backdrop a colour
while a section runs, and the height of the coloured band on the screen *is*
the time that section took.

```c
profileInit();

while (1) {
    profileColorStart(PROFILE_RED);
    updatePhysics();          /* a red band as tall as physics costs */
    profileColorEnd();

    profileColorStart(PROFILE_GREEN);
    buildSpriteList();        /* a green band under it */
    profileColorEnd();

    WaitForVBlank();
}
```

No tooling, no host, no build flag: the bands appear on real hardware too, and
a section that grows past VBlank is visible as a band that reaches the bottom
of the screen.

**Scanline timing** gives a number instead of a picture:

```c
profileScanlineStart();
updateEnemies();
u16 lines = profileScanlineEnd();   /* scanlines consumed */
```

**Frame counters** answer "am I dropping frames?":

```c
u16 frames = profileGetFrameCount();   /* frames since boot, wraps at 65535 */
u16 lag    = profileGetLagFrames();    /* frames that missed their VBlank */
```

A rising lag count is the symptom; `luna profile` tells you which symbol
caused it.

@note No example links the profile module today, so its code path is exercised
by nothing in the corpus. Treat it as a debugging aid you switch on while
working, not as shipped code.

## Choosing between them

| You want to know | Use |
|---|---|
| Which routine costs the most | `luna profile`, sort by mclk |
| Whether the worst frame fits | `luna profile`, the max/frame column |
| Whether a routine stays under budget forever | `luna profile --budget`, in CI |
| Where my own section sits inside the frame | `profileColorStart` / `profileColorEnd` |
| Whether the game is dropping frames on hardware | `profileGetLagFrames` |
| Which lib functions my ROM never executes | `luna profile --pc-set` (the harness does this: `tools/luna-test/rom_coverage.py`) |

## The optimisation loop

1. **Measure first.** Run the profile on the scene that stutters, with
   `--from-frame` past the boot.
2. **Read max/frame, not the average.** The row that ruins a frame is rarely
   the row with the biggest total.
3. **Change one thing.**
4. **Re-measure the same scene with the same input**, and compare the two
   numbers.
5. **Prove you changed only the speed.** Keep the ROMs from before the change
   and run `python3 tools/luna-test/diff_corpus.py --ref <tree>`: every example
   must still match at its capture frames. An optimisation that changes a pixel
   is a bug until explained.

## See also

- @ref craft_frame_budget — what fits in a frame, and the ~4 KB VBlank DMA ceiling
- @ref tutorial_debugging — stepping, breakpoints and memory inspection with luna
- @ref tools_luna — every luna subcommand and flag, generated from the pinned binary
- `.claude/rules/testing.md` — where the profile and coverage passes sit in the suite
