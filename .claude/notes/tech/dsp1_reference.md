# DSP-1 — technical reference (merged research, 2026-08-02)

Deep reference for the SNES DSP-1 coprocessor, merged from five parallel
research passes + direct inspection of our own luna binary. The **strategy /
decision** view (Path A vs B, why it matters for us) lives in
[`dsp1_coprocessor.md`](dsp1_coprocessor.md); this file is the *how it works*.

Confidence tags: **★** = confirmed by ≥2 independent sources; **◆** =
single-source or reconstructed (verify before relying on it). Where sources
conflict it is called out inline.

---

## 1. What the chip is

- **DSP-1 = NEC µPD77C25** (a µPD7725-family 16-bit fixed-point DSP) running
  **Sony's fixed mask-ROM firmware**. It is a *fixed-function math* coprocessor:
  you cannot run your own code on a genuine chip — you invoke ~30 pre-baked
  firmware **commands** over a byte port. ★
- **DSP-1 / 1A** are functionally identical; **DSP-1B** is the bug-fixed
  firmware revision (§8). ST010/ST011 are the bigger **µPD96050** cousins (§9).
- Internal layout (mask ROM, on-die): **program ROM 2048 × 24-bit = 6144 B**,
  **data ROM 1024 × 16-bit = 2048 B** (math LUTs), **data RAM 256 × 16-bit =
  512 B**. Program+data = **8192 B** — exactly the size of the `dsp1(b).rom`
  firmware dumps. ★

## 2. Register interface

The CPU sees exactly **two byte-wide registers**, mirrored across a bank window
whose location depends on the cart mapper.

| Mapper | Data reg (DR, r/w) | Status reg (SR, r/o) | Selector |
|--------|--------------------|-----------------------|----------|
| **LoROM / mode 20** (SMK) | `$30-$3F:8000-BFFF` (mirror `$B0-$BF`) | `$30-$3F:C000-FFFF` | A14: 0=DR, 1=SR |
| **HiROM / mode 21** (Pilotwings) | `$00-$0F:6000-6FFF` (mirror `$80-$8F`) | `$00-$0F:7000-7FFF` | `$6xxx`=DR, `$7xxx`=SR |

The wide ranges are decode mirrors — pick one concrete address per register.
These match fullsnes ("6000h-7FFFh DSP-n on HiROM; F8000h.. on LoROM"). ★

- **SR bit 7 = RQM** ("request for master"): `0` = busy, `1` = ready for the
  next byte transfer. **Poll `SR & $80` before every byte.** ★
- DR is an **8-bit port**; command I/O is logical **16-bit words**, each sent as
  two consecutive bytes. ★
- ✅ **Byte order = LSB-then-MSB (little-endian)** — RESOLVED empirically on
  luna (2026-08-02, wip/dsp1 Phase 0): a `Multiply` of `$4000 × $4000` (0.5×0.5
  in 1.15) read back LSB-first as `$2000` (0.25, exact). RQM gates **per byte**
  (poll SR bit 7 before every DR access). DR `$30:8000` / SR `$30:C000` (LoROM)
  and far `lda.l`/`sta.l $30xxxx` from ASM confirmed working.
- ◆ The underlying µPD77C25 has DRC/DRS bits; the *SNES DSP-1 interface* exposes
  only RQM. Treat RQM as the sole handshake bit.

## 3. Command handshake

Per command, every access gated by RQM:
1. Poll RQM=1 → write **command byte** to DR.
2. For each input word (fixed order): poll RQM=1 → write low byte, poll RQM=1 →
   write high byte.
3. For each output word: poll RQM=1 → read low, poll RQM=1 → read high;
   reassemble.

`Project` ($06) takes ~627 DSP cycles (~82.5 µs) — the RQM wait before outputs
is real; **poll, don't fixed-delay**. ★

**Opcode mirroring:** the command byte is partially decoded, so several opcodes
hit the same routine (e.g. `$00`/`$20` = Multiply; `$01`/`$05`/`$31`/`$35` =
Attitude A). Canonical = the lowest opcode. High nibble often selects the
matrix slot A/B/C for the matrix family. ★

## 4. Fixed-point formats

Each operand slot is **typed** — there is no single global format. ★

| Type | Meaning | Format | Notes |
|------|---------|--------|-------|
| **I** | integer | 16-bit signed | coords in/out |
| **T** | fraction | signed **1.15** | −1.0…+0.99997; a "unit" is `$7FFF`, never `$8000` |
| **A** | angle | 16-bit signed, full circle = 2¹⁶ | `$4000`=+90°, `$8000`=±180°, wrap is free |
| **D** | 32-bit | two words | Radius returns squared length as 32-bit |

- ◆ One source's summary mislabels A as "8-bit" — treated as a transcription
  error; every command reads a full 16-bit angle word.
- `Multiply` ($00) rounds to ≤15 bits (1.15×1.15→1.15), **not** a full 32-bit
  product. `Radius` (squared length) and `Distance` (rounded length) are the
  magnitude ops.

## 5. Command table (full firmware set)

In/Out = number of **16-bit words**. Consolidated from snes9x `dsp1.cpp`
(opcodes/mirrors) + SNESdev wiki (typing). Flags where out-counts conflicted.

### Scalar math
| Opcode | Name | In | Out | Purpose |
|--------|------|----|----|---------|
| `$00`,`$20` | Multiply | 2 (T,T) | 1 (T) | rounded 1.15 product ★ |
| `$10`,`$30` | Inverse | 2 | 2 | reciprocal (coeff+exp) ★ |
| `$04`,`$24` | Triangle | 2 (A,I) | 2 (I,I) | radius·sinθ, radius·cosθ — core trig ★ |

### Vector length
| Opcode | Name | In | Out | Purpose |
|--------|------|----|----|---------|
| `$08` | Radius | 3 (I×3) | 2 (32-bit) | x²+y²+z² (squared) ★ |
| `$18`,`$38` | Range | 4 | 1 | magnitude²−range² (sphere test / LOD) ★ |
| `$28` | Distance | 3 | 1 | √(x²+y²+z²) rounded ★ |

### Rotation
| Opcode | Name | In | Out | Purpose |
|--------|------|----|----|---------|
| `$0C`,`$2C` | Rotate (2D) | 3 (A,I,I) | 2 (I,I) | rotate (x,y) about Z ★ |
| `$1C`,`$3C` | Polar (3D) | 6 | **3** | rotate (x,y,z) by 3 Euler angles ★ (◆ one src said 6 out; math ⇒ 3) |

### Attitude matrix + frame transforms (3 slots A/B/C)
| Opcode | Name | In | Out | Purpose |
|--------|------|----|----|---------|
| `$01`(A) `$11`(B) `$21`(C) | Attitude | 4 (S,θz,θy,θx) | 0 | build scaled Euler rotation matrix into slot ★ |
| `$0D`(A) `$1D`(B) `$2D`(C) | Objective | 3 | **3** | apply matrix: local→global ★ (◆ one src said 6/"F,L,U"; vector ⇒ 3) |
| `$03`(A) `$13`(B) `$23`(C) | Subjective | 3 | 3 | apply transpose: global→local (view) ★ |
| `$0B`(A) `$1B`(B) `$2B`(C) | Scalar | 3 | 1 | dot with a matrix axis ★ |
| `$14`,`$34` | Gyrate | 6 | 3 | incremental rotation of an orientation ◆ |

◆ **Objective vs Subjective direction labels** are stated loosely across
derivatives (the *math* is consistent). Verify empirically: rotate a known axis
and see where it lands.

### Perspective projection (the pseudo-3D pipeline)
| Opcode | Name | In | Out | Purpose |
|--------|------|----|----|---------|
| `$02`,`$12`,`$22`,`$32` | Parameter | 7 | **4** | set global viewpoint/perspective (Fx,Fy,Fz,Lfe,Les,Aas,Azs → **Vof, Vva, Cx, Cy** — order per the official manual §5.4.1, confirmed by the Raster probe 2026-09-03, see §15). Out=4 CONFIRMED twice: luna SMK trace (#161) AND direct probing 2026-09-02. Behaviour characterised empirically (luna DSP-1B): all-zero setup is **degenerate** (every Project → 0,0); `azs=$4000` → view axis = **+Y** (X across, Z up); projected offset ≈ `(lfe+les)·x/y`; `fz` = camera height; H is mirrored (negative for +x) and Project V is up-positive. ★ |
| `$06`,`$16`,`$26`,`$36` | **Project** | 3 (I x,y,z) | 3 (H,V,M) | world point → screen X, screen Y, scale/depth ★ |
| `$0E`,`$1E`,`$2E`,`$3E` | Target | 2 (h,v) | 2 (x,y) | inverse of Project: screen → ground plane (aim/pick). Probed 2026-09-03: (h,v) are **raster-style** screen coords relative to the imaginary centre (v down-positive, NOT Project's up-positive V); Target(0,0) returns exactly (Cx,Cy). ★ |
| `$0A` (**use this**), `$1A` (wedges — see §15) | Raster | **1** (Vs) + sentinel | 4 per raster, **unbounded** | per-scanline Mode-7 matrix STREAM: A→B→C→D→A… for rasters Vs, Vs+1, … until the CPU writes **`$8000` to DR in place of reading a D**. luna's "5 setup words" on SMK = Vs + 4×`$8000` (the game writes the sentinel four times so one lands on a D slot). Fully characterised by the 2026-09-03 probe, §15. ★ |
### Control / diagnostics (not demo math)
- `$80` **Sync/Reset** — 0 in / 0 out. Flushes pending command state
  (`in_count=0`, `waiting4command`, `first_parameter` reset in snes9x HLE) →
  DSP-1 back to command-wait. Written ~128× at boot as a sync handshake.
  Our module should issue it at init for robustness. ★ (snes9x HLE)
- `$0F`/`$07` RAM test (1→2) · `$2F`/`$27` ROM size/version (1→2) ·
  `$1F`/`$17`/`$37`/`$3F` ROM dump (1→2048, RE only). ★

## 6. Pseudo-3D pipeline — minimal wireframe cube

What the games did: **SMK/Suzuka** = `Raster` Mode-7 floor + `Parameter` camera
+ `Project`/`Target`; **Pilotwings** = `Attitude`→`Objective`/`Subjective`
frame transforms + `Parameter`+`Project` + `Raster` ground (and the DSP-1B
attract-demo bug lives in this path, §8). ★/◆

Minimal cube (the module's first target):
```
Init (once, or when camera moves):
  Parameter ($02): camera setup  -> cache Cx,Cy (tune Lfe/Les for FOV empirically)
  [dsp1-v2, 2026-09-02: working reference config = (0,0,0, lfe=96, les=256,
   aas=0, azs=$4000) — the dsp1_cube example + docs/tutorials/dsp1.md use it]
Per frame:
  Attitude A ($01): S=$7FFF, θz,θy,θx        -> rotation matrix in slot A
  for each of 8 vertices (model x,y,z):
    Objective A ($0D): (x,y,z) -> world (x',y',z')   [reuse the one matrix]
    Project    ($06): (x',y',z') -> (H,V,M)          [store H,V]
  CPU draws the 12 edges between the 8 projected (H,V) points
      (the DSP-1 has NO line/pixel command — rasterization is the CPU's job)
```
**Even-more-minimal:** skip Parameter/Project; use `Polar` ($1C) to rotate each
vertex, then software-divide x/z, y/z for perspective. Fewer under-documented
constants; slightly more CPU. Good first bring-up path.

Throughput: every operand crosses the byte port under RQM — an 8-vertex cube is
dozens of round-trips/frame. Fine for a cube; reuse the Attitude matrix, only
re-run Parameter when the camera moves.

## 7. ROM header / mapping

- **Cartridge-type byte `$FFD6`**: upper nibble `$0x` = DSP; lower nibble = mem
  config. `$03` = ROM+DSP (Pilotwings); `$05` = ROM+DSP+RAM+battery (Super
  Mario Kart). Header does **not** distinguish DSP-1/1B/2/3/4 — all read `$0x`;
  the specific chip is a board fact. ★
- Map-mode byte `$FFD5`: `$20` LoROM / `$21` HiROM. Register decode into the §2
  windows is a board/PCB wiring fact; emulators infer it from cart-type +
  map-mode. ★
- Our integration pattern (cf. SA-1/SuperFX): a new `templates/hdr_dsp1.asm` +
  `memmap_dsp1.inc` + the cartridge-type byte. SA-1 needed a post-link
  `sa1_patch` for `$FFD5`; check whether DSP-1 needs an analogous patch.

## 8. DSP-1 vs 1A vs 1B

- DSP-1 = DSP-1A (identical firmware). **DSP-1B** fixes a math bug in the
  rotation/attitude/projection path. ★
- ◆ Sources confirm *that* a routine was corrected and *that* it's in that path,
  but do **not** pin it to a single opcode. Don't rely on bit-exact equality
  across revisions in that path.
- Famous consequence: **Pilotwings' attract-mode landing depends on the buggy
  DSP-1 result — on DSP-1B the plane crashes.** Real carts shipped both. ★
- **Target DSP-1B** for new work: it's the corrected version and the emulator
  default for everything except Pilotwings.

## 9. µPD77C25 architecture + Path B (custom microcode)

For the wla-dx#392 "assemble your own DSP code" path. The chip:
- Harvard, 16-bit fixed-point, **24-bit instruction word**, four formats
  **OP / RT / JP / LD**. Two 16-bit accumulators (A/B) each with c/z/s0/s1/
  ov0/ov1 flags; K/L multiplier inputs; M/N product halves; DP (data-RAM ptr),
  RP (ROM ptr, auto-dec); PC + 16-deep return stack. ★
- Datapath: **16×16→31-bit signed multiply every cycle** (`M=prod>>15`,
  `N=prod<<1`), 16-op ALU (OR/AND/XOR/SUB/ADD/SBB/ADC/DEC/INC/CMP/SHR1/SHL1/
  SHL2/SHL4/XCHG/NOP); one MAC + RAM access + ptr update + accumulator op per
  cycle. Q15 fractional convention. ★
- **Best machine-readable spec = MAME `cpu/upd7725`**: `dasm7725.cpp`
  (encoding) + `upd7725.cpp` (semantics). An assembler inverts the disassembler.
- **µPD96050** (ST010/011): backward-compatible ISA, bigger — 16384×24 program,
  2048×16 data ROM, 2048×16 battery-backed RAM; 52 KB dump. ★
- Prior art: MAME's disassembler (canonical); Romhacking.net util #705 "DSP
  Assembler/Deassembler" ◆ (unverified target/author, page 403'd); **no
  widely-adopted open-source µPD77C25 assembler** — the gap #392 would fill. No
  public clean reassembly of the DSP-1 firmware exists.
- **Delivery:** custom microcode **cannot** run on a genuine chip (mask ROM).
  Targets = **LLE emulators** (bsnes/higan, MAME) and **FPGA** (SD2SNES/FXPak,
  MiSTer) that load an external 8 KB image. The copyright problem *disappears*
  for custom code (you ship your own image); remaining barriers = no real-HW
  target + flashcart variant auto-selection (title heuristic) ergonomics.

## 10. Emulation & firmware — the load-bearing constraint

Two approaches:
- **HLE** (snes9x, no$sns): intercepts commands, computes results in host code.
  **No firmware ROM needed.** Targets DSP-1B semantics. Fast, imprecise, no
  timing. Origin: ZSNES team (zsKnight/_Demo_/pagefault/Nach) + Overload
  lineage.
- **LLE** (bsnes/higan, ares, Mesen2, MAME, FPGA): runs the real µPD77C25
  microcode. **Requires the user-supplied firmware dump** `dsp1.rom` /
  `dsp1b.rom` (8 KB, Sony copyright, **non-redistributable**; obtained by
  decapping, 2010). bsnes deliberately dropped HLE for LLE.

| Emulator | Method | Needs firmware? |
|----------|--------|-----------------|
| snes9x, no$sns | HLE | **No** |
| bsnes/higan, ares, Mesen2, MAME | LLE | **Yes** |
| SD2SNES/FXPak, MiSTer | LLE (FPGA) | **Yes** |

### luna is LLE — CONFIRMED (binary inspection + user confirmation 2026-08-02)
Strings in `tools/luna-test/bin/luna` prove it: a dedicated **`luna-cpu-upd96050`**
crate (real DSP core), `struct Dsp1State` / "DSP-1 (NEC uPD7725) coprocessor
snapshot", explicit **`dsp1b.rom`** references, a **`<config>/luna/firmware`**
folder, errors "missing coprocessor firmware" / "needs coprocessor firmware",
and a subcommand *"Install a DSP coprocessor firmware (`dsp1b.rom`) … then
load — needed for DSP-1 games (Super Mario Kart, Pilotwings). Persists for
future runs."* **No HLE path.** The user confirmed: luna runs the DSP natively
from an external ROM dump.

**Consequence for us:**
- The dev installs `dsp1b.rom` into luna's firmware folder once (like
  `install-luna.sh` fetches luna itself). Local build + test of a DSP-1 example
  is fully possible.
- **CI/corpus cannot ship the firmware** → a DSP-1 example's luna test must be
  **gated on firmware presence** (skip when absent, exactly like the
  `INPUT-DEP` examples in `luna_runner`). Documented prereq, not a blocker.

## 11. Test ROMs (free content)

- **PeterLemon/SNES: NO DSP-1 content** (has GSU, no DSP folder). ★
- **ARM9/snesdev** (`dsp-1/raster/`): the one free DSP-1 homebrew — **source
  only** (build with `bass`), and its **LICENSE file is empty/unstated** →
  redistribution unclear. Still needs firmware on any LLE stack. ◆
- No cleanly-licensed **prebuilt** DSP-1 test ROM found. "DSP1Demo" is an
  official Nintendo cart (copyrighted).
- ⇒ For our own example we author original 65816 code driving the command
  interface; no third-party ROM needed.

## 12. Annotated bibliography (best resources, tiered)

**Tier 1 — canonical**
- **snes9x `dsp1.cpp`** (github.com/snes9xgit/snes9x) — the most complete
  command-set enumeration; what homebrew actually programs against. ★
- **bsnes/ares LLE core + MAME `cpu/upd7725`** — ground truth for chip
  internals/timing; MAME disasm = the assembler spec for Path B.
- **caitsith2.com/snes/dsp/** — firmware dumps, sizes, SHA256/MD5, endianness
  (`.bin`=BE, `.rom`=LE), game compat.
- **fullsnes** (problemkaputt.de/fullsnes.htm) — "NEC DSP" section: memory map,
  µPD7725 ISA, SR/DR, flags. (Large single page; jump to the section.)
- **NEC µPD77C25/µPD7725 datasheet** — the base silicon.

**Tier 2 — solid**
- **jsgroth**, "SNES Coprocessors: DSP-1 and Friends"
  (jsgroth.dev/blog/posts/snes-coprocessors-part-1/) — best "read first"
  narrative (HLE vs LLE, handshake, Mode-7 math). *(Anubis-blocks bots; open in
  a browser.)*
- **SnesLab DSP1** (sneslab.net/wiki/DSP1 + per-command pages Project/Rotate/
  Objective/Attitude) — command names/semantics.
- **SNESdev wiki DSP-1 / DSP_Expansion** (snes.nesdev.org/wiki) — fixed-point
  types; overview/index.
- **romhacking.net doc #320** "DSP-1 Emulation Code" — compact SR/DR reference
  impl. *(403 to bots; browser/Wayback.)*
- **NESdev threads**: "upd7725 overflow (attn: byuu)" (flag semantics);
  "Dumping the DSP1B firmware" (decap); "Running DSP-1 Homebrews".
- **ARM9/snesdev** `dsp-1/raster/` — practical 65816-side usage.

**Tier 3 — history/context**
- byuu/near "State of Emulation IV" (decap→LLE story); Wikipedia enhancement-
  chips lists; Nintendo Life Pilotwings-crashing-plane (DSP-1 vs 1B).

**Structural finding:** there is **no single standalone "DSP-1 programmer's
reference."** Knowledge is split across (a) generic µPD7725 ISA docs, (b) the
reverse-engineered command set living inside emulator HLE source + forum
threads, (c) the decapped firmware + LLE cores. The snes9x command table + the
fullsnes NEC-DSP section together are the practical spec. Overload's/Anomie's
originals are the ultimate primaries but only reachable via those derivatives.

## 13. Implications for OpenSNES (Path A plan seed)

- **Feasible now:** a `dsp1` lib module wrapping the command interface (RQM
  poll + typed word transfer helpers) + a pseudo-3D **wireframe cube** example.
  First command subset: **Attitude → Objective → Project** (+ the software-
  perspective `Polar` fallback for bring-up).
- **Build integration:** `hdr_dsp1.asm` + `memmap_dsp1.inc` + cartridge-type
  `$03`/`$05`; verify whether a `$FFD5`-style post-link patch is needed.
- **Testing:** gate the example's luna pass on `dsp1b.rom` presence; document
  "install the firmware into luna" as a dev prereq beside `install-luna.sh`.
  CI skips it (no firmware) — the `INPUT-DEP` treatment.
- **Verify-before-code items** (the ◆s): ~~DR byte order + RQM per-byte/word~~
  ✅ RESOLVED Phase 0 (LSB-first, per-byte RQM — see §2); Raster protocol, Parameter outputs and Target ✅ RESOLVED 2026-09-03 (§15). Remaining: Parameter
  operand scaling; Objective/Subjective direction; exact out-counts for
  Polar/Objective/Gyrate — resolve during the module bring-up (Attitude→
  Objective→Project) against a luna run + snes9x source.
- **Phase 0 status (2026-08-02, wip/dsp1):** ✅ build wiring (USE_DSP1 →
  cart-type $03); ✅ luna detects `Mapper: Dsp1`, runs firmware-inert without
  the dump and names the supply path (`--dsp1-rom` / config folder); ✅
  `dsp1b.rom` (DSP-1B, sha1 78b7248…) installed; ✅ Multiply handshake verified
  (0.5×0.5→0.25). Next: Phase 1/2 = the `dsp1` module + wireframe cube.
- **Path B** (custom microcode via a future `wla-dsp`) stays a watch item on
  wla-dx#392 — see [`dsp1_coprocessor.md`](dsp1_coprocessor.md).

## 14. Cross-references
- [`dsp1_coprocessor.md`](dsp1_coprocessor.md) — strategy / Path A vs B / watch #392.
- `templates/hdr_sa1.asm`, `hdr_superfx.asm`, `memmap_*.inc` — the chip-integration pattern.
- `make/common.mk` GSU two-stage build — the Path B integration template.
- `tools/luna-test/luna_runner.py` — the `INPUT-DEP` gating pattern to reuse for firmware-gated examples.

## 15. Raster protocol — Phase 0 probe results (2026-09-03, luna v1.17.0, DSP-1B LLE)

Source of truth: official SNES Development Manual Book II §5.4.1–5.4.4
(local corpus `snes-rag/corpus/manual/book2_text.pdf`) + a throwaway probe ROM
(`--dsp1-trace-commands` + WRAM peeks). Everything below is measured unless
marked *manual*.

**Protocol (★)**
- Command byte **`$0A`**, then **one word Vs** (raster number where the projected
  display begins). Output then loops A→B→C→D→A→… one 4-word group per raster,
  starting at raster Vs, forever. Every word LSB-first, RQM-gated per byte like
  every other command.
- **Termination**: write `$8000` to DR **in place of reading a D** (manual
  §5.4.2, literal). Measured: a write landing on an A/B/C slot is *swallowed*
  (the stream continues); only the D-slot write ends the command. That is why
  Nintendo-style code (SMK, ARM9/snesdev) writes `$8000` **four times** after a
  full group — one of them hits D. The lib does the manual-literal form: read
  A,B,C of the next raster, then write `$8000` once (`in_words = 2` in the luna
  trace). After the sentinel SR reads `$84` (bit 2 = DRC, 8-bit command mode) and
  a `Multiply` KAT is green immediately.
- **`$1A` wedges the chip on luna.** The manual lists `$0A` = "output via DMA"
  and `$1A` = "not via DMA" (and §4.4 says DMA is unsupported on the SNES), but
  measured: `$1A` never returns to command mode — every later byte is swallowed,
  and **`dsp1Init()`'s 128× `$80` Sync does NOT recover it** (the `$80 $80` bytes
  pair up as a 16-bit word inside the open stream). SMK and ARM9/snesdev both use
  `$0A`. The lib uses `$0A` only. *Hypothesis unconfirmed on real silicon.*
- A `Parameter` result persists across streams: three consecutive `$0A`
  streams after one `Parameter` all produced consistent data.
- **Quirk**: `Vs = 0` yields a duplicated first group (raster 0 appears twice,
  then raster 1, 2, …). Any other Vs tested (−104, −46, −45, 32, 200) was clean.
  Start ground streams at `Vva + 2` (below) and the quirk is never hit.

**Parameter outputs = Vof, Vva, Cx, Cy (manual §5.4.1, ★)**

Raster numbers used by Parameter/Raster/Target are **down-positive and relative
to the screen centre** (the "imaginary centre" convention below); Project's V is
the opposite (up-positive). Sweep (fz=100, lfe=96, les=256 unless noted):

| azs | Vof | Vva | note |
|---|---|---|---|
| `$4000` (horizontal) | 46 | −46 | horizon = Vof+Vva = 0 = screen centre |
| `$3800` (11.25° down) | 0 | −50 | 256·tan(11.25°) = 51 |
| `$3000` (22.5° down) | 0 | −105 | 256·tan(22.5°) = 106 |
| `$4800` (11.25° up) | 100 | −49 | horizon = 100−49 = +51 below centre |
| `$4000`, les=128 | 23 | −23 | vertical focal = **Les** |
| `$4000`, les=512 | 92 | −93 | |

- **Vva is relative to Vof**: the horizon's screen raster is `Vof + Vva`
  (screen line `112 + Vof + Vva`). When the true horizon sits at or below the
  centre the firmware moves the *imaginary centre* down (Vof > 0) so its ground
  maths has a reference below the horizon; when looking down enough, Vof = 0.
  lfe and fz do not affect Vof/Vva.
- **The stream's raster numbering is Vs-relative to the imaginary centre**: a
  stream started at `Vs = Vva` produces −K at group 0, ∞ (clamped `$7FFE`) at
  group 1 and K/1, K/2, K/3 … after, i.e. **raster `Vva+1` is the singular
  horizon line and `Vva+2` is the first finite ground line** (A = K, very far).
  So: stream from `Vs = Vva + 2`, and place that group on screen line
  `112 + Vof + Vva + 2`.
- A/B/C/D are Mode 7 8.8 matrix values ready for M7A–M7D. For aas = 0 B = C = 0
  and D ≈ 5.6·A (D is the secant slope relative to the imaginary centre, which
  is why M7VOFS must put the pivot on that line — see the dsp1_ground example).
  With aas = `$2000` (45°) B and C are populated (253, −663, 253, 662): rotation
  is free, keep all four words.
- **Cx, Cy**: ground coordinates of the point under the imaginary centre, to be
  written to M7X/M7Y. fx/fy offset them 1:1. Sign convention: aas=0 looks
  toward **−Y on the ground plane** (Cy = fy − dist), aas=`$4000` toward +X,
  aas=`$8000` toward +Y — i.e. ground Y = −world Y (world +Y is Project's
  depth axis). This is the natural Mode 7 map convention (forward = up the map).

**Target (`$0E`, 2→2, ★)**: (h, v) screen point → (x, y) ground, same ground
convention as Cx/Cy, h mirrored like Project's H. Target(0,0) = (Cx,Cy) exactly.

**Cost (measured, per-byte RQM polling, SlowROM)**: a 100-raster stream ≈ 122
scanlines ≈ 7.8 ms ≈ 46 % of a frame, CPU-bound (≈ 8 bus cycles per byte of
polling + read + store). The manual's DSP-side figure is 29.5 + 27.5·(n−1) µs
(2.75 ms for 100 rasters). SMK streams 96 rasters per frame. Budget the ground
at ≈ 100 lines/frame or refresh it every other frame.
