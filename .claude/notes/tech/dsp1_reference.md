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
| `$02`,`$12`,`$22`,`$32` | Parameter | 7 | **4** | set global viewpoint/perspective (Fx,Fy,Fz,Lfe,Les,Aas,Azs → Cx,Cy,…). Out=4 CONFIRMED via luna SMK trace (#161, 112 consecutive txns); scalar *meanings* still ◆ |
| `$06`,`$16`,`$26`,`$36` | **Project** | 3 (I x,y,z) | 3 (H,V,M) | world point → screen X, screen Y, scale/depth ★ |
| `$0E`,`$1E`,`$2E`,`$3E` | Target | 2 (H,V) | 2 (x,y) | inverse of Project: screen → ground plane (aim/pick) ★ |
| `$0A`,`$1A`,`$2A`,`$3A` | Raster | **5** (setup) | **unbounded** | per-scanline Mode-7 matrix STREAM; in=5 setup words (luna SMK #161), out is open-ended (384 words/frame in SMK) terminated by a CPU sentinel write — NOT a fixed count ★ |

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
  ✅ RESOLVED Phase 0 (LSB-first, per-byte RQM — see §2). Remaining: Parameter
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
