# G1 R2 — the invariant suite and the mutation score of the equivalence harness (result)

Branch `g1-r2` from `develop` `9c7d873c`, built `make clean && make`, no `QBE_*` variable. New files only:
`testing/fixtures/g1_invariants/` (the six fixtures and their tests), `devtools/g1bench/mutate.py`, this note. Nothing of the
compiler, the library or develop's tests changes; nothing enters `develop` before the conductor re-validates.

## Prediction (written 2026-11-15 before any fixture or test is written; committed alone)

Read before writing it: the plan's revision 2 and its contest (points 7-8), the conductor's answer on the free convention,
the retrospective's section 8 (the four invariants a port named, plus the two of the plan), `compiler/ABI.md`, the fixtures of
`testing/fixtures/compiler/` (a `main.c`, a `Makefile`, a `test_*.py` asserting through luna), `testing/nmi_budget.py`
(`luna profile --budget`), `opensnes-rom check` (`tools/opensnes-rom/src/asm.c`: the bank-blind read guard reads the `.c.asm`
for `.w SYM` reads and `#SYM` addresses without `#:SYM`, and refuses a symbol in banks `$01+` at `$8000+`), gapbench's harness
(`devtools/gapbench/run.py`: a row's C checksum must equal its assembly twin's). Tool probes only (no fixture yet): luna 1.44.0
`state --trace-log --trace-format '[Disassembly]|[EffectiveAddress]|...' --trace-condition 'isWrite && address >= $2115 ...'`
gives one row per instruction that touches a register, with its PC, operand, P flags and the scanline / dot: the order,
the number and the width of the accesses can be read from luna, filtered by the PC range of the fixture's C functions.

### How every fixture is built and turned red

Each fixture is a one-unit ROM (`main.c`, sometimes a small `.asm`), a `Makefile` on `make/common.mk`, a `test_<name>.py`
that asserts through luna (and on the emitted `.c.asm` / `.sym` where the invariant is static). Each test has a `--fault NAME`
mode (also `G1INV_FAULT=NAME`) that injects one generator fault and must turn the test red; `--all-faults` runs the clean case
then each fault and fails unless the clean case is green and every fault is red. A generator fault is a **patched copy of the
emitted `.c.asm`** (one regex, applied inside one function's body), re-assembled with the same `wla-65816` command
`make/common.mk` uses and re-linked by `make`; the fixture's source is never edited. The one exception is I5, whose brief
names a source fault (a `const` cast away), done as a `-D` variant of the same `main.c`, plus a generator fault on the text.
The ROM is rebuilt clean after the faults.

| inv. | fixture | the clean case checks (today's generator must pass) | how | the faults that must turn it red |
|---|---|---|---|---|
| I1 | `i1_regs` | four functions write console registers in a stated order: the VRAM port (`$2115`, `$2116/7`, `$2118/9`, two equal writes in a row) through `REG_*` and through a raw `volatile u8 *`; the multiplier (`$4202`, `$4203`, a wait of volatile reads, `$4216`/`$4217`); a DMA channel (`$4300-$4306`, `$420B`) and `$420C` written twice with the same value. The sequence of accesses (address, read / write, 8 bits) inside the fixture's functions equals the C program order, the product equals the model, the `$4216` read comes at least 8 CPU cycles (12 dots) after the `$4203` write | luna trace (PC filtered by the functions' address ranges from `.sym`), then the same sequence read statically from the `.c.asm` of each function | (a) two consecutive VRAM writes swapped; (b) the second of the two equal `$420C` writes dropped; (c) `$2118`/`$2119` merged into one 16-bit store; (d) the wait reads dropped so `$4216` is read early |
| I2 | `i2_nmi` | a function `nmi_work()` called from an `nmiSet()` callback: worst completed frame in master cycles under a budget = today's measure + 2 %, and its instruction count in the `.c.asm` under today's count (no growth); its bank unchanged (no move out of the bank it is in today) | `luna profile --budget nmi_work=…` (nmi_budget.py's method) and the `.c.asm` body | (a) four `nop` inserted in its loop body (cycles and count grow); (b) one `nop` outside the loop (count grows, cycles grow a little: the cycle budget may not catch it, the count does) |
| I3 | `i3_layout` | C fills a tilemap of 32 words, an OAM mirror of 128 four-byte entries + the 32-byte high table, a palette of 16 words, then sends each by DMA as it is (VRAM, OAM, CGRAM). The bytes in WRAM and in VRAM / OAM / CGRAM equal a Python model of the C | `luna state --peek` of the symbols and of `VRAM:` / `OAM:` / `CGRAM:` | (a) the OAM entry's tile byte stored at the attribute offset (+1); (b) a tilemap word stored as one byte (the high byte lost); (c) the palette loop's step changed (2 to 4: one word in two) |
| I4 | `i4_abi` | assembly calls C by the documented ABI: a global `abi_sub3(a, b, c)` (order matters: `a - 2b + 3c`) by name, pushed left to right, result read in A; a `static` function whose address is taken, called by assembly through its pointer (trampoline `jml [tcc__r9]`); an `nmiSet()` callback. Results equal the model. A `static` function called directly only (marked `noinline`) is listed FREE: the test asserts nothing about its convention, only its result through its C caller, and checks the generator itself sees the difference (the free one opens without `rep #$20`, the public ones with it) | `luna state --assert` of the results; the entry lines of the `.c.asm` | (a) `abi_sub3` reads its first and last arguments swapped (stack offsets exchanged); (b) the `rep #$20` at the entry of the address-taken static removed (assembly calls it with an 8-bit accumulator); (c) `rtl` of `abi_sub3` turned into `rts`. A fourth, (d), is a FREE change: the free static's arguments read through a changed convention patched consistently in callee and caller; it must stay GREEN (the free convention is allowed there) |
| I5 | `i5_bank` | a `const` table in the asset banks read from C through a `const` pointer: `opensnes-rom check` accepts the link and the value read is right | build, the link's exit and check line; `luna --assert` | (a) source variant `-DI5_FAULT`: the read goes through `(u16 *)` cast away from `const`: the link must FAIL with the bank-blind message; (b) generator fault: the clean unit's far read `lda.l TABLE…` patched into `lda.w TABLE…`: check must refuse the link |
| I6 | `i6_probe` | a probe list (`main`, `game_step`, `wait_frame`): each keeps its exact label in the `.sym`, each non-main is reached by a `jsl NAME` in its caller's `.c.asm` (not inlined, not renamed), `luna --until-pc game_step --hit N` stops there, and a global the caller updates before the call (`frame_no`) reads N-1 at the stop (its value is in memory at the probe, not held in a register across the call) | `.sym`, `.c.asm`, `luna state --until-pc … --peek` | (a) `game_step` renamed in its definition and its call (the label disappears; the probe cannot stop); (b) the store of `frame_no` moved after the `jsl game_step` (the probe reads a stale value); (c) the call replaced by a copy of the body (inlined) |

Prediction: **all six pass on today's generator** (no stop), and **every listed fault turns its test red except I2 (b)**, which
I expect red on the instruction count and possibly green on the cycle budget (2 % of the measured frame is more than one `nop`
outside the loop; predicted: the cycle half green, the count half red, the test red). I4 (d) green by design. The two I
consider likeliest to surprise: I5 (a), if the compiler does not name the table's symbol in the cast read (the guard reads
the symbol; a read through a pointer loaded at run time is outside it: KNOWN_LIMITATIONS), and I1 (d), if luna does not model
the multiplier's latency (then the early read returns the right product and only the distance check turns it red).

### The mutation score

Harness judged: gapbench's own equivalence check (`devtools/gapbench/run.py`: every row's C checksum must equal its assembly
twin's and be non-zero; a run that does not reach `bench_done` fails the harness as well). The differential tests `difftest.py`
/ `difftest_stmt.py` are not used for the score: their programs are drawn per seed, so a fault injected in the `.c.asm` of one
seed says nothing of a fixed corpus; gapbench's rows are fixed C functions with fixed inputs, the shape a generator's output is
judged on in R3 and R4. The private state compare of the games is out of scope.

`devtools/g1bench/mutate.py`: builds gapbench once (SlowROM), takes the emitted `kernels.c.asm`, and for 10 C functions
(leaves and functions that call: `k_copy_c`, `k_fill_c`, `k_oam_c`, `k_col_c`, `k_box_c`, `k_fixed_c`, `k_mul_c`, `k_ents_c`,
`k_call_c` with its callee, `k_soa_c`, `k_glide_c`, `k_chain_c` and callees — 10 to 12 named in the result) draws with a fixed
seed **80 faults**, one instruction each, from six operators: a changed immediate (`#k` to `#k+1`, or a bit flipped), a swapped
register (`x`/`y` in a transfer, a load or an index), a dropped instruction, a flipped branch (`beq`/`bne`, `bcc`/`bcs`,
`bpl`/`bmi`, `bvc`/`bvs`), a wrong width after `sep`/`rep` (`#$20` to `#$30` or `#$10`, both directions), a wrong direct-page
slot (`tcc__lf+N` to `+N±2`, `tcc__rK` to another). Each mutant is re-assembled (one that the assembler refuses is not counted
and another is drawn), re-linked, run to `bench_done` under luna (bounded by a frame limit), and judged by the harness's rule.

Predicted score: **84 % detected (range 76 to 92)**, of the 80: about 67 detected (of which a third by a hang or crash rather
than a checksum), 13 survivors, half of them equivalent mutants (a dropped `rep #$20` where A is already 16 bits, a dropped
`clc` before an `adc` whose carry is known clear, a dropped store to a slot nothing reads, a changed immediate in a path the
inputs never take with a value that does not matter) and half real escapes (a flipped branch on a path the row's fixed inputs
never take: culling and range tests of `oam` / `glide` / `box`; a checksum that folds two wrong values into the same sum).
Reasoning: the rows' checksums cover every byte the C writes, so a fault on the data path is caught; the weak spots are the
paths the fixed inputs do not take and the width faults that change nothing when the width is already right. **Predicted
under the contest's 90 % line**, which, if it holds, means the proof by checksum is not enough for a generator as it stands;
the result will say what would raise it (inputs that take every branch of each row, a checksum that is not additive).

Stop rule (the brief's step 5): if one of the six invariants does not pass on today's generator, the work stops there and
the result reports the failing one with its assembly.

## Result (2026-11-15)

Commits on `g1-r2`: `6084dfd5` (this prediction, alone), `143fe733` (the six fixtures), the mutation script, this result.
Pair: Opus alone (no other model called, as the brief says). The fixture files were drafted while the gate that the commit
hook asks for (`make tests`) ran before the prediction's commit; nothing was built or run before that commit.

### The six invariants: all pass on today's generator (no stop)

`python3 testing/fixtures/g1_invariants/run.py` (each test clean, then under each of its faults, `--all-faults`):
`G1 invariants: 6/6 as expected`. One line each, clean and red:

| inv. | clean (today) | fault -> the failing line |
|---|---|---|
| I1 | GREEN: every access of the four functions in program order, 8 bits, on the trace and in the text; products 143 and 50 250 | `swap`: `luna vram_regs: accesses W2115 W2116 W2117 W2119 W2118 …` ; `drop`: `… W420B W420C != … W420B W420C W420C` ; `merge`: `… W2118/16 W2118 W2119 …` ; `early`: `multiplier: $4216 read 10 dots after the $4203 write (< 12)` and `value r_prod2 != 0xc44a: … got dac4` |
| I2 | GREEN: `nmi_work` worst frame 3 218 mclk (budget 3 282), 20 instructions, 45 bytes, bank $00 | `loop`: `cycles: nmi_work worst frame 3666 mclk > 3282` (and the size lines) ; `once`: `size: nmi_work has 21 instructions > 20` (cycles 3 232, under budget) |
| I3 | GREEN: WRAM, VRAM $1000, OAM and CGRAM equal the model byte for byte | `tile`: `OAM: byte 2: got 00 30 02 54 want 03 30 02 54` ; `byte`: `VRAM $1000: byte 3: got 00 07 … want 04 07 …` ; `stride`: `CGRAM: byte 2: got 00 00 21 04 want 21 04 42 08` |
| I4 | GREEN: asm -> `abi_sub3` 101, asm -> `cb_mix` through its address with A in 8 bits 1 189, C -> `free_mix` 3 861, NMI callback ran; public functions open with `rep #$20`, `free_mix` does not | `swap`: `luna: r_sub3 … = 0x0123, want 0x0065` ; `width`: `text: PUBLIC cb_mix opens with lda 6,s` and `r_ptr … = 0x00a5, want 0x04a5` ; `rts`: `PUBLIC abi_sub3 does not return with rtl`, `r_sub3 = 0x0a0a` ; `free` (allowed change): GREEN as required |
| I5 | GREEN: link accepted (`OK: no bank-blind C reads`), `r_k` = 9 | `src` and `gen`: `opensnes-rom check refused the link: … C code reads bank $01+ data with bank-$00 addressing … $07:8483 ktab (read bank-blind in main.c.asm)` |
| I6 | GREEN: three labels, `jsl game_step` / `jsl wait_frame` in main, stops at each with frame_no 5 | `rename`: `sym: probe game_step has no label` ; `late`: `luna: at game_step hit 5 {'frame_no': 4, …} != {'frame_no': 5, …}` ; `inline`: `text: main does not call game_step` (the linker then drops the unreferenced function: no label either) |

Facts found on the way:
- Today's generator reads `REG_RDMPYL` / `REG_RDMPYH` as `lda.w #16918 / tax / lda.l $0000,x` (a constant address through X):
  the text check had to follow that form, and it is why dropping the four wait reads alone did **not** turn I1 red (the
  indexed form already puts 16 CPU cycles between `$4203` and `$4216`). The fault was strengthened to the form a better
  generator would emit (`lda.l $004216` right after the operand), and luna then returns a wrong low byte: luna models
  the multiplier's latency (`dac4` for `4ac4`).
- luna 1.44.0 refuses `--peek oam:#544` for a label named `oam` (it reads it as the `OAM:` region, case blind): the fixture
  names its mirror `oam_mirror`. A small luna gap, to report to luna (a label shadowed by a region keyword).
- `wait_frame()` is emitted as a tail call (`jml WaitForVBlank`, no `rep #$20`): the probe still stops on it.

### The mutation score: 81.2 % detected, under the 90 % line

`python3 devtools/g1bench/mutate.py` (seed 1, 80 faults, 12 functions: `k_fill_c k_oam_c k_col_c k_box_c k_fixed_c
k_mul_c k_ents_c k_call_c k_call_f k_soa_c k_glide_c ch_update`; 3 mutants refused by the assembler and redrawn), 1 min 44 s:

`faults injected 80; detected 65 (81.2 %), survived 15` — by kind: imm 8/14, reg 14/14, drop 12/13, branch 13/13,
width 6/13, dp 12/13; detected by a hang or crash 10, by a checksum 55.

The 15 survivors, each read in the text:

| # | function, line | mutant | class | why |
|---|---|---|---|---|
| 1 | `k_fixed_c` 3015 | `cmp.w #32` -> `#33` | equivalent | the entry test of a rotated loop, run once with i = 0 |
| 2 | `k_ents_c` 3618 | `cpy.w #32` -> `#33` | equivalent | the same, Y = 0 |
| 3 | `k_soa_c` 4764 | `lda.w #16` dropped | equivalent | the branch to it is taken only when `and.w #16` left exactly 16 in A |
| 4 | `k_fill_c` 1247 | `cpx.w #512` -> `#513` | **real escape** | one more turn with X = 512: a word stored after `fl_buf`, outside what the checksum reads |
| 5 | `k_col_c` 2124 | `cpy.w #64` -> `#65` | **real escape** | the same after `colbuf` |
| 6 | `k_box_c` 2816 | `lda.w #250` -> `#251` | **real escape** | a range bound off by one: no input sits on the bound |
| 7 | `k_soa_c` 4758 | `and.w #16` -> `#17` | **real escape** | bit 0 of the flags never separates the inputs |
| 8 | `k_box_c` 2725 | `sta.b tcc__lf+14` -> `+16` | **real escape** | `px2` never written: the stale slot gives the same answers on these inputs |
| 9-14 | entries of `k_fill_c`, `k_col_c`, `k_box_c`, `k_call_c`, `k_call_f`, `ch_update` | `rep #$20` -> `rep #$10` | **real escape of the harness** (ABI) | the driver (`gap_call`) always enters with A and X in 16 bits: an exported function that no longer sets its width passes here and fails an assembly caller in 8 bits (I4's `width` fault is this one) |
| 15 | `k_fixed_c` 3123 | `rep #$20` -> `rep #$10` before `rtl` | **real escape of the harness** (ABI) | returns with an 8-bit accumulator; the driver's `rep #$30` after every call hides it, a C caller would not |

So: 3 equivalent mutants, 12 real escapes; over the non-equivalent mutants, 65 / 77 = **84.4 %**. Both figures are under
the contest's line: **the checksum proof alone is not enough to accept a generator's function**. What would raise it,
each named by the survivors it would catch (hypotheses until re-measured with the same seed):
1. **the ABI contract checked at the boundary** (the contest's point 8, the `jsl` / `rtl` check on the luna trace): the
   driver enters each row with A and X in 8 bits and checks after `rtl` that both are 16 bits, S balanced, D and the data
   bank unchanged — survivors 9 to 15 (7);
2. **guard zones**: the checksum covers the object plus its neighbours (or a hash of the whole WRAM band the row may
   touch, `luna wram-trace`) — survivors 4 and 5;
3. **a poisoned direct page**: `tcc__lf` and `tcc__r*` filled with a pattern before each call (the paranoid build of the
   contest) — survivor 8;
4. **inputs on the bounds**: each row's setup also draws values on every compare bound and every flag bit (`box`,
   `soa`) — survivors 6 and 7.
All four together would leave only the 3 equivalent mutants on this seed (to be measured, not claimed). The I1-I6 suite
already covers the kind of item 1 for the functions it names (I4 `width`), not for every function a generator emits.

### Gates (replayed on `g1-r2` after the three code commits)

`make clean && make`: `OpenSNES SDK build complete!` (the only warnings are the host's `-rdynamic` and a CMake deprecation,
as on `develop`); `testing/fixtures/g1_invariants/run.py`: `G1 invariants: 6/6 as expected`; `make tests`: `gapbench: OK,
nothing above baseline.json` / `ALL CHECKS PASSED (luna)` (twice: before the prediction's commit and after the code);
`make lint`: exit 0, `OK: all 3 commits pass lint`; `make lint-docs`: `OK: no doc drift detected.` The fixtures are not wired
into `make tests` and `mutate.py` into no `make` target: the brief allowed new files only (the conductor decides the wiring).

### Predicted against measured

| item | predicted | measured | held? |
|---|---|---|---|
| the six invariants pass today | yes | yes, 6/6 | yes |
| every listed fault red (I4 `free` green) | yes, except I2 `once` partly | yes; I2 `once` red on size, green on cycles, as predicted | yes |
| I1 `early` | possibly green on value | green as first written (the generator's own form waits long enough); red once strengthened, on value and distance | half: the first fault was too weak |
| I5 `src` | likeliest to surprise | red, the symbol is named (`lda.w ktab+10`) | yes |
| mutation score | 84 % (76 to 92) | 81.2 % | yes, in range |
| survivors | about 13, half equivalent | 15: 3 equivalent, 12 real | **no**: fewer equivalent, more escapes |
| detected by a hang or crash | a third | 10 / 65 (15 %) | **no** |
| under 90 % | yes | yes | yes |

## Verdict of the maintainer session (2026-11-15)

Replayed by me on the branch `g1-r2`: `testing/fixtures/g1_invariants/run.py` reads `G1 invariants: 6/6 as expected` (the six clean cases pass on today's generator, each injected fault goes red, the one allowed change of convention, `free`, stays green). The mutation score (81.2 %, 80 faults, seed 1) is **the agent's, not replayed by me** (a long run; to replay at the start of R3: `python3 devtools/g1bench/mutate.py --json out.json`). The prediction was committed alone before any test run (`6084dfd5`).

What R2 settles:
1. **The six invariants are testable and hold today** (register order and count read on the emulator and in the text, the NMI path's budget, the layout of what the DMA sends, the boundary convention of functions called from assembly or the NMI, bank-blind reads, probe entry points). They are the acceptance tests of any generator: a prototype is run against them first.
2. **The equivalence harness of the bench is not enough to accept a generator: 81.2 % of one-instruction faults detected, under the 90 % the contest set.** Of the 15 survivors, 3 are equivalent mutants and 12 are real escapes: 7 are width faults at the entry or exit of a function (the bench driver always calls with A and X 16 bits wide and restores `rep #$30` after each return, so a wrong `rep` at the boundary is invisible), 2 are writes past the checked buffers, 2 are bounds or bits the inputs never reach, 1 is a stale direct-page slot. **Before R3's figures count, the harness gets the checks that catch them**: the `jsl` / `rtl` contract at the boundary entered with 8-bit registers and checked on exit (7), guard zones or a hash of WRAM (2), a poisoned direct page before each call (1), inputs on the bounds (2); then the score is replayed on the same seed: the target is 90 % on the 77 non-equivalent mutants (84.4 % today).
3. **A defect of the emulator pinned at 1.44.0 found on the way** (a peek of a region named `oam` is refused: the label is taken for the `OAM:` region without regard to case): to report to its session.
4. **Not wired**: the fixtures are not in `make tests` and the mutation script has no target: a decision for the owner of the prototype's gates; the fixtures belong to `develop` when the prototype needs them.
