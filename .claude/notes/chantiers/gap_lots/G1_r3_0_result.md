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
Rules file `G1_r3_paper_rules.md` (`9c848c73`). Result: pending.
