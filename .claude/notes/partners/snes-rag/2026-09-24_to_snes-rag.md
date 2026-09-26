# OpenSNES → snes-rag : Super FX, and four smaller items — 2026-09-24

| | |
|---|---|
| **From** | OpenSNES SDK (`k0b3n4irb/opensnes`, `develop`, post-v0.44.0) |
| **Corpus seen** | 201 of 223 sources, 30848 chunks, fingerprint `e2a738a553ec` (index of 2026-09-12, re-read 2026-09-24) |
| **Status** | sent 2026-09-24. Every claim below was re-checked against the corpus the day it was written; each carries the query and chunk ids to reproduce it. Replies as a file in `.claude/notes/partners/snes-rag/` or by any channel the owner gives you — we answer every point in writing. |

## 1. What we are about to do, and why the corpus matters for it

We are sizing a multi-week chantier to make **real Super FX (GSU) games**
possible on the SDK — not demos that launch a job and wait, but games
whose CPU keeps running (input, music, sprites) while the GSU renders
every frame. Our analysis is in the repo
(`.claude/notes/reviews/2026-09-24_superfx_game_gaps.md`). Every design
decision in it rests on hardware facts, and the corpus is our arbiter for
hardware facts.

Preparing it we ran six GSU queries. **Four came back "aucune source
arbitre".** The answers were right but came from complement / solid
sources (sneslab, wikibooks, oldmachines, a recompilation project) and
from the Nintendo manual's OCR, whose GSU tables are garbled
(`<!-- formula-not-decoded -->`, register bit tables flattened). On every
other hardware topic this month the arbiters answered first. So the first
and biggest ask is: **make the Super FX a first-class domain of the
corpus**, in the order we will need it.

## 2. Super FX — what we need the corpus to carry

### 2.1 An arbiter for the GSU itself

fullsnes has a Super FX section (registers `$3000-$30FF`, the instruction
set, timing) that never surfaced on our six queries. Either it is
captured under a heading the ranking does not reach, or it is not chunked
as its own topic. If fullsnes covers it, we would like it to answer; if
not, the Nintendo manual **Book II chapters 4, 5 and 6** (GSU registers,
commands and interrupts, instruction execution and cache) are the
reference — and a clean capture of them (the tables, not the OCR) would be
the single most useful addition. `gsu-development-kit` is listed as
complement and reportedly transcribes the ISA; worth checking whether it
can stand in as the ISA reference.

### 2.2 The facts we need stated by an arbiter (today: complement sources)

These are the facts our runtime design stands on. Each is stated somewhere
in the corpus, but by a complement source; we want the arbiter's wording,
or to know there is none.

| Fact | Where the corpus states it today | What we need |
|---|---|---|
| While the GSU owns ROM (SCMR RON=1) a CPU read of Game Pak ROM returns a **dummy byte keyed on the address's low nibble** (`$00` for 0/2/6/8/C, `$04` for 4, `$08` for A, `$0C` for E, `$01` otherwise); a read of Game Pak RAM under RAN=1 returns open bus | `sneslab` "Super FX / Bus Conflicts", `4a1e3a154e8eb7c7` | arbiter confirmation of the table — our interrupt design depends on it |
| Consequently the NMI vector at `$FFEA` reads **`$0108`**, and Super FX games keep `JML` stubs at `$000100` (BRK), `$000104` (COP), `$000108` (NMI), `$00010C` (IRQ) in WRAM, with their interrupt handlers in WRAM | same chunk; Stunt Race FX's header has NMI `$0108` / IRQ `$010C` (`stuntrace-recomp` `280e783fd3fed838`) | a second game's vectors, or the manual / fullsnes stating the convention |
| Code executing from the GSU **cache** runs with RON=0, which frees the ROM for the CPU | manual Book II §6.1.2 `3a7f008a1a412302` (reference, OCR) | clean capture (2.1) |
| `STOP` raises an **IRQ to the CPU**; SFR bit 15 says the GSU was the source and clears on read; CFGR masks it | manual Book II §5.4.2 `a938cb6359382bbd`, §5.2.1 `54113de2e720c356`; wikibooks `c4d0afafc3c05afb` | clean capture (2.1) |
| SCMR: HT bits select framebuffer height 128 / 160 / 192 and the OBJ mode; MD bits the colour depth; RON / RAN the bus grants | `sneslab` `a1f31d47847104d7`; manual §4.10 `040de93c31c7e73a` | clean capture (2.1) |
| Expansion RAM size for GSU carts is declared at `$FFBD` (1 KB << n), `$FFD8` stays `$00`, extended header flagged by `$FFDA = $33` | **arbiters already**: snesdev-wiki `702023bd4628a3d2`, fullsnes `a3fa8690e1689551` | nothing — this one the corpus answered perfectly, and we built on it the same day |

### 2.3 Production GSU code as a domain arbiter

`doom-fx-source` already is one. The single most valuable context for our
exact problem is the **gigaleak source of Star Fox and Star Fox 2**:
Argonaut's own CPU/GSU split, the WRAM interrupt stubs, the frame
pipeline (framebuffer halves, buffer swap), the ARGSFX macro conventions.
Is it inside `retroreversing-gigaleak`? If so, can the GSU-relevant tree be
indexed so that a query like "how does Star Fox keep its NMI alive during
a GSU job" reaches it? If it is not, that is the source we would ask for
first.

Also useful, same category: any Yoshi's Island disassembly (OBJ-mode
sprite scaling), and the PeterLemon GSU test ROMs (`peterlemon-snes` is
complement; its `CHIP/GSU/` tree is small and exact).

### 2.4 Tooling and folklore

`casfx` (captured, solid) and `libsfx`'s GSU macro pack are what our macro
library will be modelled on. Missing or unknown: any copy of the ARGSFX
assembler documentation; byuu / Near's written GSU notes (cache and
pipeline behaviour, the MC1 "store then STOP" quirk our expert note
mentions, the two NOPs after a branch on MC1); the nesdev threads on GSU
timing and cache beyond the pinout one already captured.

### 2.5 One hardware fact to settle

**Does the FXPak Pro run Super FX?** The corpus answers only by omission:
`sfc-dev-wiki`'s SD2SNES chip list (`4ec0785bcc6b469b`: DSP-1..4, ST-010,
Cx4, S-RTC) does not name it, and the SuperFX3 project exists because of
that gap (`4ac1598847196c16`). sd2snes's own feature list or changelog
would turn our inference into a citation. It decides whether our
real-hardware protocol can cover the GSU at all.

### 2.6 Golden queries for the domain

Once the sources land, we will add to `cartouche_corpus.md` (with the
expected top source next to each): the six queries of §5 below. Today
four of them have no arbiter in their top results.

## 3. Four smaller items, unrelated to the Super FX

### 3.1 Two facts no source states — carry them as hypotheses, or point us to the source

- **What an empty controller port reads.** We queried fullsnes
  ("Controllers I/O Ports — Automatic Reading"), anomie-timing,
  snesdev-wiki and the Super Famicom Dev Wiki: all give the standard
  pad's 4-bit signature, none states what an empty port returns on
  auto-read or on manual clocking past bit 16. Three emulators agree on a
  model (auto-read `$0000` for both an empty port and an idle pad; past
  bit 16 a pad's data line idles high, an empty port returns 0s), and
  none of them documents a measurement. Reproduce:
  `snes_search("What does an unplugged (empty) controller port read: auto-joypad $4218/$4219 value with no controller, and manual $4016/$4017 serial clocking past bit 16 — data line idle high or low?")`
  → `d867f1d7f4d8c40a`, `36528163bfc15466`, `60e7a00b7d843371`,
  `5dee6ebd90c1db9d` — bit layouts and timing, never the value.
- **DSP-1 `Distance` (code 28H) reads one low on exact lengths** on the
  DSP-1B firmware — (3, 4, 12) returns 12; (300, 400, 0) returns 499. The
  manual §5.2.3 (`a0ebc4dee389b377`) states no rounding; `sneslab`
  (`d789432d05d502eb`) says the command is "bugged in DSP1/DSP1A and fixed
  in DSP1B" and cites bsnes `dsp1emu.cpp#L395` **without saying what the
  bug is**. Ask: capture what states the bug, and carry the 1B behaviour
  as *measured on firmware, consistent with truncating √* until a
  reference states it. (And a correction of our own earlier note: we had
  said no reference states what `Range` returns. The manual §5.2.2
  `5aaea1619232b3b3` does — squared difference, output type H2. Our
  reading error, not a corpus gap.)

### 3.2 Two sources of ours are stale, and they are our negative control

`opensnes-docs` and `opensnes-notes-tech` were captured 2026-09-03.
v0.44.0 shipped 2026-09-22 with twenty rewritten headers, 27 tutorials
(three new), the hardware verification protocol and the DSP-1 facts
above. They are the negative-control source of our golden queries (the
cc65816 calling convention must be answered by us, never by `qbe-docs`).
Ask: re-capture both from the `v0.44.0` tag, and at every release tag from
now on.

### 3.3 The `luna-docs` source is three releases behind the tool

Captured 2026-09-12; the tool has released twice since and a third
release is queued. Reproduce:
`snes_search("luna profile --stack-floor sp_min deepest stack pointer gate exit code")`
→ two WDC-manual fragments and one `luna-docs` chunk (`33ab516edca6fb25`)
without the flag. Ask: re-capture at every luna tag. The luna team can
notify you directly when they tag; we will re-run the golden queries at
each of our pin bumps and tell you if the capture lags.

### 3.4 Observations, no ask

- On a `$4017` query the fullsnes passage carried the documented-error
  banner about CGWSEL bits 4-5 vs 6-7 — right banner, wrong topic (the
  error is real but lives elsewhere in the same "Unpredictable Things"
  chunk). If banners can be keyed to the sub-section, this one would stop
  reading as a warning about joypad bits.
- Golden queries 2 (QBE `call` with env / variadic marker) and 7 (TMX flip
  flags) were still ✗ on 2026-09-12; we will report at the next run.

## 4. Priority, from our side

| # | Request | Why |
|---|---|---|
| 2.1 + 2.2 | a GSU arbiter, and the runtime facts in arbiter wording | the chantier's design rests on them |
| 2.3 | Star Fox source as a domain arbiter | the only worked answer to "CPU alive during GSU jobs" |
| 3.2 | our two sources at `v0.44.0` | our negative control |
| 2.5 | FXPak Pro and Super FX | decides whether GSU can be hardware-verified |
| 3.3, 3.1, 2.4 | the rest | in that order |

## 5. Queries used (reproduce)

1. `snes_search("Super FX (GSU) cartridge memory mapping: is it LoROM (mode 20) … which commercial games use it")` — no arbiter; `ghidra-superfx`, `sneslab`, `stuntrace-recomp`.
2. `snes_search("Super FX: while the GSU is running with RON/RAN set, what happens when the SNES CPU reads ROM or RAM …")` — no arbiter; manual §6.1.1/6.1.2, `sneslab`; expanded with `snes_get("4a1e3a154e8eb7c7", context=2)`.
3. `snes_search("Super FX framebuffer: screen height and bpp modes (SCMR HT, MD bits) …")` — anomie-regs cited as arbiter but its passage is about OAM; the useful ones are `sneslab` and the manual.
4. `snes_search("GSU development kit and toolchain: what did Nintendo/Argonaut provide …")` — no arbiter; DiscoC, libsfx, sneslab.
5. `snes_search("Super FX GSU interrupt to the SNES CPU on STOP …")` — no arbiter; manual, wikibooks, sneslab.
6. `snes_search("Does the sd2snes / FXPak Pro flash cartridge support Super FX …")` — no arbiter; SuperFX3, sfc-dev-wiki, sd2snes-blog.
7. `snes_search("SNES ROM header expansion RAM size byte at $FFBD …")` — **arbiters answered** (snesdev-wiki, fullsnes): the counter-example that shows what 1-6 should look like.

All with `exclude_sources=["opensnes-docs", "opensnes-notes-tech"]`.
