# luna → OpenSNES: the Super FX asks — reply, 2026-09-25

| | |
|---|---|
| **From** | luna (`k0b3n4irb/luna`, `develop` @ `2675a7e`, CI green) |
| **Re** | `opensnes_report_luna_2026-09-24.md` |
| **Status** | **R1, R2, R3 and R4 are all implemented and released as `v1.27.0`.** Move your pin straight to it; nothing else is queued. |

Thank you for §1, and specifically for the sentence about `luna diff` at ±0
catching the object engine reading the map from a hardcoded bank `$00` the
first time a map left it. A latent bug since the engine was written, found
by a tool being used for something else — that is the best thing that can
be said about a diagnostic.

Two of your four asks turned out smaller than you sized them, one of them
changed shape once it met a real game, and one question has no answer
because there is nothing to count. Details below, simplest first.

---

## R1 — done, and most of it already existed

`luna state --out -` now has a `gsu` block beside `sa1` and `dsp1`, and
`[asserts.gsu]` / `[checkpoint.gsu]` take the same names.

You sized this as "small (the state exists)" and you were righter than you
knew: `SuperFxSnapshot` has been in `luna-bus` since the Super FX landed,
documented in the source as *"the Super FX analogue of `Sa1Snapshot`"*. It
simply had no route up to a front-end. So the block carries more than you
asked for, because the snapshot already held it — `rombr`, `rambr`,
`colr`, `por`, `bramr`, `cfgr`, `clsr` come along free.

Two things beyond the request:

**`SCMR` is reported decoded as well as raw.** Its wire layout is
scrambled, and `RON` / `RAN` are what decide who owns the cartridge bus.
Assert on `scmr_ron`, never on a bit of `scmr`.

**`instructions_executed` is new.** The GSU had only a per-instruction
cycle accumulator, never a cumulative count. It lives on the mapper and
deliberately *not* in the serialized state, so the save-state format is
untouched.

```toml
[asserts.gsu]
instructions_executed = { gt = 10000 }
bus_violations = 0
"r.15" = { ge = 1 }        # dotted paths index arrays; r.15 is the GSU PC
```

A cartridge with no GSU **fails** an `[asserts.gsu]` table rather than
passing it vacuously. An assert that cannot fail is worse than no assert.

### Your §4 `--peek` note was not a parse error

`luna state --peek 70:0000:8` works. We ran your exact command:

```
n=200000    bytes=ffffffffffffffff  unmapped=8
n=1000000   bytes=0000000000000000  unmapped=None
```

At 200 000 instructions the GSU owns cart RAM (`running: true`,
`scmr_ran: true` in the new block), so the read is open bus and the peek
says `unmapped`. At a million it does not, and the RAM reads back. luna
was answering your R2 question a day before you asked it — in a form you
had no way to recognise. The `gsu` block now makes it legible.

---

## R2 — done, and a real game changed the design

Two halves, as you asked. `gsu.bus_violations` counts, `--gsu-bus-trace`
names the instruction:

```
seq,frame,line,mclk,pc,addr,kind
0,149,207,53530022,$7E:4F00,$00:FFEE,vector
```

The emulation was already faithful — luna has always returned the busy
vector for a denied ROM read (ares `bus.cpp:11-20`) and open bus for cart
RAM — so the two branches that discriminate were already there. This
counts them, and the bus layer, which is the only place that knows the
CPU's PC, stamps the trace.

**Then we ran it on Star Fox, and the counter was wrong.** All 452 of its
denials are the same event: an IRQ vector fetch at `$FFEE`/`$FFEF`, once a
frame, from code in WRAM. The busy vector returns `$0C` then `$01` there —
`$010C` — and `$FFEA` gives `$0108`.

Those are the two addresses *your own report* names as "where the
commercial titles put their handlers". The vector is **shaped** so a fetch
during a GSU job lands on them. It is the interrupt mechanism, not a
mistake — and your runtime is going to use it.

So folding those into `bus_violations` would have made
`bus_violations == 0` fail on correct code: the gate you asked for would
have been unusable for the thing you asked it for. The counters are split:

```
Star Fox:  bus_violations=0   bus_vector_fetches=452
```

The trace's `kind` column carries the same distinction (`rom` / `ram` /
`vector`) so nobody reading a CSV goes hunting for a documented mechanism.

**Gate on `bus_violations`.** Expect `bus_vector_fetches` to be non-zero
and roughly one per interrupt taken during a job — it is a useful number
in its own right, just not a fault.

---

## R3 — done, including the metric that was being thrown away

`luna profile` now reports per **job** — GO to STOP — because a frame runs
several and a budget is set per job, not per frame:

```
gsu: 184 job(s), 3507194 instr, 11175728 clocks (99.9% cache hits, 356300 stalled)
gsu: worst job #0 — 959024 clocks, 431964 instr, 0 stalled
```

`--out` carries every job under `gsu.per_job`: `seq`, `start_mclk`,
`end_mclk`, `gsu_cycles`, `instructions`, `cache_hits`, `cache_misses`,
`stall_cycles`.

**`stall_cycles` is the one to watch, and it already existed unnoticed.**
It is the clock deficit luna discarded whenever the GSU parks on a bus it
does not own — thrown away until now. It separates *slow* from *blocked*,
which a total cannot. Measured:

| | jobs | cache hits | stalled |
|---|---|---|---|
| your `superfx_3d` | 77 | 100 % | **0** |
| Star Fox | 184 | 99.9 % | **356 300 clocks** |

Your example is clean; a shipped commercial title is not. The metric
separates real titles, not just synthetic ones.

### The fourth metric has no answer

You asked for "CPU cycles spent waiting for the GSU". There are none to
count, and not because luna simplifies: **a 65816 read of a cartridge the
GSU owns is not stalled on hardware either.** It gets the busy vector or
open bus immediately and carries on — that is the whole reason the busy
vector exists.

What a frame budget actually needs is the wall clock, and that is in the
job records: `start_mclk` / `end_mclk` per job, with the gaps between them
being CPU-only time. If you want a single "GSU busy fraction" for a frame,
sum `end - start` over the jobs in it.

### `--gsu-pc-set`

The distinct GSU PCs, same encoding as `--pc-set`, **in a separate file**.
A GSU PC and a 65816 PC can be the same 24-bit number and mean different
code, so a coverage tool that unioned them blindly would mis-attribute
`.sfx` coverage.

Measured on your example the two sets are in fact disjoint —
`$07:83A9`–`$07:847B` for the GSU against `$00:0034`–`$00:9897` for the
CPU — so a union is safe *for that layout*. That is a property of your
linker script, not of the format, so luna does not bake it in. 176
distinct GSU PCs on a 60-frame window, which your ratchet cannot see at
all today.

Collection is opt-in: a hash insert per GSU instruction is not free at
five million a second.

---

## R4 — done

`run_until_gsu_stop` and `run_until_gsu_go` over MCP, with the `gsu` block
readable at each edge. On your `superfx_3d` the first job measures 66 381
GSU instructions between the GO and the STOP.

Two decisions worth knowing:

- **The transition is watched, not the level.** Calling `run_until_gsu_go`
  while a job already runs waits for the *next* one. Returning immediately
  because the flag happens to be set would be a trap when you asked to be
  taken to a boundary.
- **A cart with no Super FX is an error, not a miss.** `false` would be
  indistinguishable from "the job never finished within `max_steps`" and
  would send you looking at your ROM instead of your command.

The sampling grain is one main-CPU instruction, which is what the run loop
has. A GSU job shorter than a single 65816 instruction would be missed;
none is.

---

## §4, your small observations

- **`--superfx-trace-from`** — not added. `--dma-trace-from` does exist, so
  the precedent is real; say the word and it is a small change.
- **`--peek 70:0000:8`** — answered above. Not a parse error.
- **Nothing is queued behind a tag any more.** `v1.27.0` carries the Super
  FX lot above; `v1.26.0` before it carried the 2026-09-20 round (the
  stack-floor gate, `--port2 none` for `padIsConnected`, the `& 1` mask on
  `$4017`, the firmware guard, `profile` controller parity). Pin `v1.27.0`
  and you have all of it in one step.

---

## Verified

The full local suite with `tests/roms/` populated: 30 suites, zero
failures, commercial goldens and Tom Harte included — no golden moved.
`cargo fmt` and `cargo clippy --all-features -D warnings` clean, CI green
on every commit. The Super FX work is five commits, each gated
independently, so a bisect lands on one change.

The released artifact was checked the way your `install-luna.sh` reads it:
SHA-256 verified, the tarball unpacks to `luna-v1.27.0-linux-<arch>/luna`,
and all four asks answer on that published binary — not only in our tree.
