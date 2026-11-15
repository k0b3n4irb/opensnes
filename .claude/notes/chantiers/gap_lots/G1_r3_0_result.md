# G1 R3-0 result (2026-11-15, in progress)

## Corpus grep: IRQ handlers and the direct page (R2.8)
Checked 2026-11-15 over `examples`, `lib/source`, `templates`, `testing/fixtures`: two raw IRQ handlers exist,
`examples/color/hicolor_1792/irq_stream.asm` and `testing/fixtures/stress/rt32_nmi/irq.asm`. Neither touches D
(`tcd`/`pld`/`phd` absent; no `.b` direct-page access): the first sets DBR itself (`phb`, `plb`) and uses `.w` on
`$21xx/$42xx`, the second uses `.l` only. The contract in `lib/include/snes/interrupt.h` (l.158-172) already says an IRQ
handler must save what it touches and set its own data bank, and that a C pointer must not be passed to `irqSet`. So a
main-thread function that moves D or DBR is not seen by any handler of the corpus. A user handler that assumed D = 0
would contradict the documented contract. Hypothesis closed for the corpus, not proven for user code.

## Paper allocation, `soa`
IR given to the second agent: `qbe -t w65816 -d I` on the cproc IR of `k_soa_c` (SSA, before instruction selection output).
Rules file `G1_r3_paper_rules.md` (`9c848c73`). Written by a Sonnet agent that read only the rules and the IR
(`g1_r3_0/soa_ir.ssa`, `g1_r3_0/soa_paper.asm`); eight assignments tried, no block weights given (none claimed).
Measured by me in a scratch copy of the bench (paper added as a row `k_soa_pp` against the same C checksum; the repository's
bench untouched), master cycles per call:

| speed | C | hand | paper | paper / hand | C / hand |
|---|---|---|---|---|---|
| SlowROM | 36 377 | 24 865 | 26 223 | **1.055** | 1.463 |
| FastROM | 30 624 | 20 809 | 21 714 | **1.043** | 1.472 |

The paper's checksum equals the C's (same work). The `soa` row of C reads 36 377 here, the same as R0 (36 377), not the 35 813 of the probe (R2.2 question left open).
**Reading**: far under the 1.2 stop and under the 1.12 warning on this twin; the paper is a floor of what the prototype
reaches. **This is half of the stop test**: `path` (leaf A's neutral twin) is not written yet, so the stop is not decided.
Caveats: `soa` is leaf B's shape, not leaf B; no static-weight cell yet (the loop is the whole function here);
the agent hand-picked a cheap rebuild of X inside the loop (`tya/lsr/tax`), legal under the rules.
