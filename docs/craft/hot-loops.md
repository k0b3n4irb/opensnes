# Hot loops: writing the inner loop the compiler likes {#craft_hot_loops}

@ref craft_frame_budget says how much CPU a frame has; this guide is about the
few loops that eat it. A loop that runs 64 or 1,000 times a frame pays its
per-iteration cost that many times, so one idiom swapped inside it moves the
frame. Measure first (@ref tutorial_profiling); only the loop `luna profile`
names as the biggest `max/frame` row is worth the changes below.

The figures are the per-iteration cost of the generated code, counted
instruction by instruction with `devtools/cyclecount/cyclecount.py -v` on the
`cc65816` output (CPU cycles, no wait states, a taken branch counted at 3).
They are an estimate of the loop body, not a luna measurement: confirm any
change on your own scene before keeping it.

## 1. Walk an index, not a pointer

Summing a 64-byte `u8` table, same result, three ways:

| Loop | Cycles per iteration |
|------|---------------------:|
| `for (i = 0; i < 64; i++) s += buf[i];` | 26 |
| `u8 i = 64; while (i--) s += buf[i];` | 37 |
| `while (p < e) s += *p++;` with `u8 *p`, `u8 *e` | about 87 |

The index loop keeps `i` in X and indexes with `lda.w buf,x`. A `u8 *` is a
24-bit pointer: the compiler carries bank and offset, increments both, and
compares both on each test, so the pointer loop costs 3.3 times the index
loop. Counting down looks free in C but costs a mask (`and #$00FF`) and a
`jmp` per iteration on a 16-bit machine: **count up, 16-bit `u16` index,
compare against a constant**.

The same holds for a copy: `dst[i] = buf[i]` is 27 a byte (with the `sep`/`rep`
pair around the 8-bit store); the pointer form is several times that.

## 2. Unroll only when the body is small

Unrolling `s += buf[i]` four times drops the loop to 23.5 cycles a byte
instead of 26, about 10 %, for twice the code. Each element needs its own
temporary in the direct page, so the gain is the loop test and the `inx`
only. It is worth it for a loop of a few instructions run hundreds of times;
it is not worth it for one that does real work per element (a lookup, a
multiply), where the test is already a small share.

## 3. Keep the loop a leaf

A function that calls nothing keeps its temporaries in the direct page
(`tcc__lf`); one that calls something spills the temporaries that cross the
call to the stack (`docs/BENCHMARK.md`, "Inlining"). Two consequences:

- Put the call **outside** the loop when you can: fetch what the call returns
  once, loop, call after.
- When a loop sits in a function that must call something, move it to a
  function of its own marked `__attribute__((noinline))`: it becomes a leaf
  again and the caller keeps its own frame. Neither the explicit `inline` nor
  the automatic inliner will fold it back.

By default a leaf is inlined into any caller, even one that calls something
else, because the saving usually wins (`docs/BENCHMARK.md`, "Inlining").
`CC_INLINE_LEAF_MAX=N` makes the compiler keep a leaf apart when it has more
than N IR instructions and its caller still calls something. It is a
measuring switch, not a setting to ship: build twice, compare the profile.
The break-even measured on a ray-walking loop is about 23 iterations a call
(the call and its arguments cost ~410 master cycles, the direct-page frame
saves ~18 an iteration): under that the default (inlined) is faster, above it
`noinline` on that one loop pays. Count the iterations of your own loop before
choosing.

## 4. Do not multiply inside the loop

A multiply by a constant is inlined as shifts and adds; a variable multiply
(`i * m`) is a `jsl __mul16` call per iteration. In a hot loop, replace a variable
multiply by a table lookup or a running sum (add the step each iteration
instead of multiplying the index).

## The rule

One change at a time, measured on the scene that stutters, with a diff of the
output (`testing/diff_corpus.py`) to prove only the speed moved. If the loop
is still the biggest row after the idioms above, the next step is
assembly.

> **Sources:** `devtools/cyclecount/README.md` · `docs/BENCHMARK.md` ·
> `docs/tutorials/profiling.md`
