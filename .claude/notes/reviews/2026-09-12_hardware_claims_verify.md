# Hardware claims under arbitration — `snes_verify` sweep of the docs (2026-09-12)

The second instrument of the post-v0.42.0 sweep (`.claude/notes/reviews/2026-09-11_gaps_review.md`):
every verifiable hardware claim in `KNOWN_LIMITATIONS.md`, `docs/hardware/`, the
tutorials, the two guides, `templates/crt0.asm` and `registers.h` (143 claims, extracted by
an agent with `file:line` anchors) was put through Cartouche's `snes_verify` with
`exclude_sources=["opensnes-docs", "opensnes-notes-tech"]`, and the **citation was read**
for each verdict — the label alone is not the check (calibration showed `confirmed` on
citations that do not state the claim, and one `contradicted` whose citation supported it).

## Classification

| class | meaning | count |
|---|---|---|
| A | an arbiter citation states the claim (or it follows arithmetically from one) | see table |
| B | verdict `confirmed`/`unsettled` but the citation does not state it — standard fact, weak retrieval; no doc change unless noted | see table |
| C | the doc was wrong — fixed in the same commit | 7 |

Fixed (class C): `docs/hardware/REGISTERS.md` DMAP bits 3/4 (fixed vs decrement were
swapped; the lib was right), `docs/hardware/MEMORY_MAP.md` CGRAM ranges (16 BG palettes over
"0-255" and sprites at "256-511" → colours 0-127 / 128-255) and SA-1 BW-RAM banks ($40-$5F →
$40-$4F), `docs/hardware/README.md` PAL "256x268", `docs/tutorials/dma.md` per-byte costs
(DMA is 8 master cycles/byte, a CPU loop ~90, not "~7" vs "~10"), the "~2,200 cycles" VBlank
figure, and DMA mode 7 (it repeats mode 3's pattern, not mode 4's).

Softened (A*): OBJSEL sizes 6/7 marked undocumented (fullsnes: reserved), the DSP-1 raster
cost marked as measured on luna, the 16-bit APU-port write caution attributed to the
official manual and marked disputed, Mode 7's single-layer rule given its EXTBG exception.

Weak spots of the tool worth telling the corpus team: `confirmed` with a citation that does
not state the claim (C008, C030, C058 before reading, C104 citing the luna changelog, C096
citing the uPD7725), memory-map basics answered by sneslab/wikibooks instead of fullsnes
(C006/C007/C036), and DSP-1 register facts ranked below jsgroth although the snesdev-wiki
DSP-1 page states them (C100/C101).

## Per-claim results

Claim ids and `file:line` anchors are in the extraction list (agent output, 2026-09-12);
the ids below are stable. Format: id | verdict label | class | note.

```
C001 | confirmed | A | snesdev-wiki VMDATA: "VRAM can only be written in vblank or force-blank"
C002 | (search) | A | snesdev-wiki Timing: 37 lines NTSC / 22 overscan, 1324 active clocks/line
C003 | (search) | A | idem
C004 | (search) | A | DMA 8 clocks/byte, upper bound per blank — matches ~6 KB
C005 | contradicted(label) | A | citation IS the Errata we cite (MPY corrupted by BG1 scroll / M7 write between M7A writes); the "contradicted" comes from anomie's blank-only note on scroll regs, unrelated
C006 | unsettled | B | only wikibooks; needs fullsnes/snesdev-wiki memory-map citation (basic fact)
C007 | unsettled | B | idem (56 KB far band = $7E:2000-$FFFF)
C008 | confirmed | B | citation is the JOY4 layout, does not state "invalid during auto-read" — cited in doc already (which source?) — targeted search
C009 | confirmed | A | fullsnes SIWP 0=Protect 1=Write Enable + documented error on sfc-dev-wiki
C010 | confirmed | A | anomie: sprite palettes start at CGRAM 128
C011 | confirmed | A | fullsnes INIDISP
C012 | confirmed | A | snesdev-wiki OBJSEL (name select (NN+1)<<12, base bBB<<13)
C013 | confirmed | A* | snesdev-wiki lists 6=16x32/32x64, 7=16x32/32x32; fullsnes marks 6-7 "Reserved" — doc should say "undocumented values"
C014 | confirmed | A | snesdev-wiki OAMADDH
C015 | confirmed | B | citation is Mode 1 priority prose; bit layout not quoted (standard; low risk)
C016 | confirmed | A | snesdev-wiki BGnSC
C017 | confirmed | A | fullsnes BG12NBA/BG34NBA 4K-word steps
C018 | confirmed | A | anomie VMAIN increment mode (step table not in excerpt; standard)
C019 | confirmed | A | snesdev-wiki VMADD word address
C020 | confirmed | A | snesdev-wiki Palettes CGDATA low byte first
C021 | confirmed | A | snesdev-wiki TM layout
C022 | confirmed | A | snesdev-wiki NMITIMEN (bits 5-4 = V/H timer pair, bit 0 joypad)
C023 | confirmed | B | VTIME 9-bit shown; the 0-339 / 0-261 / 0-311 ranges not quoted (standard)
C024 | confirmed | A | snesdev-wiki RDNMI
C025 | confirmed | A | fullsnes TIMEUP read/ack
C026 | confirmed | A | snesdev-wiki HVBJOY
C027 | confirmed(label) | C? | fullsnes DMAP bits 4-3: 0=increment, 2=decrement, 1/3=fixed -> bit 3 = FIXED, bit 4 = DECREMENT. REGISTERS.md:386 says "bit 4 auto/fixed, bit 3 increment/decrement" -> SWAPPED. Verify + fix doc; check lib values.
C028 | confirmed | A | modes 0-4 per fullsnes; 5-7 per wiki (5 = p,p+1,p,p+1; 6 = mode 2; 7 = mode 3) -> C062 (dma.md:76 "mode 7 = reg..reg+3") suspect
C027 | confirmed(label) | C | CONFIRMED WRONG in REGISTERS.md:386-387 — fullsnes DMAP bits 4-3: value 1 (bit 3) = Fixed, value 2 (bit 4) = Decrement; doc has them swapped. FIX DOC. (lib value check below)
C062 | (pending) | B | modes 5-7: fullsnes says Reserved; snesdev-wiki documents 5 = p,p+1,p,p+1 ; 6 = as 2 ; 7 = as 3 — dma.md:76 says mode 7 = reg..reg+3 (that is mode 4's pattern). targeted search
C029 | confirmed | A | snesdev-wiki DAS: 0 = 65536 bytes (also "power-on DASn = $FFFF" — note for luna's $FF claim!)
C030 | confirmed | B | excerpt is the standard-controller intro; bit order not quoted (standard)
C031 | unsettled | B | only sneslab quoted 10.74 MHz; fullsnes should state it — targeted search
C032 | confirmed | A | fullsnes CCNT bits
C033 | unsettled | B | sneslab: R15 = PC at $301E-$301F; "writing R15 starts" needs fullsnes — targeted search
C034 | confirmed | A | fullsnes SFR bit 5 GO (cleared on STOP)
C035 | unsettled | B | SCBR/SCMR/VCR: sneslab only; fullsnes GSU ports — targeted search
C036 | unsettled | B | LoROM bank layout: retrieval picked sneslab SuperFX; fullsnes memory map — targeted search
C037 | confirmed | B | citation is the HiROM SRAM table; LoROM mirrors/SRAM 70-7D need the LoROM section — targeted search
C038 | confirmed | B | idem
C027 | — | C (confirmed doc error) | anomie-regs too: "Fixed (bit 3 of $43x0)… Increment (bit 4): Direction to adjust". crt0 uses $08 for fixed -> lib right, REGISTERS.md wrong
C031 | unsettled | B | no arbiter passage for 10.74 MHz (sneslab only) — decorative; mark "(sneslab)"
C033 | unsettled | B | R15 write-starts-GSU not surfaced; fullsnes GSU ports section exists (VCR found) — low risk
C035 | unsettled | B | idem SCBR/SCMR; fullsnes VCR 303Bh confirmed
C036 | unsettled | B | LoROM layout: fullsnes memory map exists; retrieval weak — basic fact
C037 | — | A | fullsnes: LoROM SRAM at 70h-7Dh,F0h-FFh:0000h-7FFFh
C038 | — | A | idem
C039 | confirmed | A | VRAM only through registers (anomie)
C040 | confirmed(label) | C? | MEMORY_MAP.md:87 says "indices 0-255 BG (16x16) and 256-511 sprites" — CGRAM has 256 entries: 0-127 BG (8 palettes... mode-dependent), 128-255 OBJ. Check wording; likely bytes/entries confusion -> FIX
C041 | confirmed | A | anomie $43x7 indirect bank; $43x5/6 count
C042 | unsettled | A* | vitor-sa1-hw-doc (solid): $2200-$23FF, I-RAM $3000-$37FF, BW-RAM mirror $6000-$7FFF, BW-RAM banks $40-$4F (256 KB max, rest mirror). Doc says banks $40-$5F -> check/fix to $40-$4F
C043 | confirmed | B | wiki: 512-byte cache; the $3100-$32FF / banks $70-$71 ranges not quoted (fullsnes has them)
C044 | confirmed | A | snesdev-wiki OAM layout 544 = 512 + 32
C045 | confirmed | A | OAM layout page
C046 | (covered by C044) | A |
C047 | confirmed | A | snesdev-wiki OAMDATA latch pseudo-code exact
C048 | confirmed | A | idem (>= $200 immediate)
C050 | confirmed | A | fullsnes OBSEL note: 64-px OBJs may wrap in 224-line mode (C013 excerpt); anomie step 0: X=256 counts for Range/Time
C051 | confirmed | A | idem
C053 | confirmed | A | anomie-timing 21.477 MHz master / 6; snesdev S-SMP 1.024 MHz
C054 | confirmed | A | snesdev-wiki S-SMP
C042 | — | C-minor | MEMORY_MAP.md:179 says BW-RAM banks $40-$5F; vitor (solid) and fullsnes: banks $40-$4F (256 KB max, rest mirror) -> fix to $40-$4F
C055 | confirmed | A? | fullsnes resolution 256x224 / 256x239; CHECK doc text "PAL 256x268" (README.md:34) — 268 lines does not exist
C056 | confirmed | A | snesdev-wiki Sprites 32/line, highest index dropped; 34 slivers
C057 | confirmed | A | snesdev-wiki ROM file formats (LoROM $800000, HiROM $C00000, ExHiROM 8 MB)
C058 | confirmed(label) | C? | DMA 8 master cycles/byte (anomie-timing) confirmed; "CPU lda/sta loop ~10 master cycles/byte" is wrong by an order of magnitude (a copy loop is ~15 CPU cycles = ~90 master cycles per byte); CHECK dma.md:24 wording
C059 | (pending) | ? | "VBlank gives ~2,200 cycles per frame after NMI" — check wording (dma.md:115)
C060 | confirmed | B | wiki: VRAM only in vblank/force-blank (HBlank thus unsafe, implied); "VMADD still increments when ignored" not quoted
C061 | confirmed | A~ | anomie: 8 master cycles overhead per channel; doc's "~12 setup + ~8 trigger" is an SDK-side estimate — say "about"
C062 | (pending) | — | modes 5-7 — snes_get on anomie DMA chunk
C063 | confirmed | A | snesdev-wiki HDMA table format (repeat / non-repeat)
C064 | confirmed | A | snesdev-wiki A1Tn etc.
C065 | confirmed | A | scroll latch text + HDMA table doctrine
C066 | confirmed | A | snesdev-wiki DASBn indirect address
C040 | — | C (doc error) | MEMORY_MAP.md:91-92: "0-255 BG (16x16), 256-511 sprites" — CGRAM has 256 colour entries: 0-127 BG (8 palettes of 16 in the 4bpp modes), 128-255 OBJ (anomie SPRITES/Palettes; snesdev-wiki). FIX
C055 | — | C (doc error) | README.md:34 "PAL 256x239 or 256x268": no 268-line mode; fullsnes: 256x224 / 256x239 (both regions), interlace 448/478. FIX
C058 | — | C (doc error) | dma.md:22-26: DMA is 8 master cycles/byte regardless (anomie-timing/regs), not "~7"; a CPU lda/sta,x loop is ~15 CPU cycles ≈ 90 master cycles/byte (FastROM), not "~10 master cycles". FIX
C059 | — | C (doc error) | dma.md:115 "VBlank ~2,200 cycles per frame after NMI": VBlank is ~50,500 master cycles ≈ 8,400 CPU cycles (KNOWN_LIMITATIONS numbers); the 2,200 figure has no source. FIX to consistent numbers
C062 | — | C (doc error) | dma.md:76 mode 7 "reg..reg+3": anomie $43x0 modes: 5 = 2 regs write twice alternate (= mode 1 pattern p,p+1,p,p+1), 6 = 1 reg write twice (= mode 2), 7 = 2 regs write twice each (= mode 3: p,p,p+1,p+1). FIX row 7 (and note 5/6 as duplicates)
C067 | confirmed | A | anomie-timing/regs HDMA: ~18 + 8/channel (+16 indirect load) + 8/byte
C068 | confirmed | A | anomie-timing clocks; snesdev-wiki 1364/1324
C069 | (covered by C028) | A |
C070 | confirmed | A | snesdev-wiki OBJSEL bBB<<13
C071 | confirmed | A | anomie: sprite char table 16x16 tiles, tile $10 below tile 0
C072 | confirmed | A | snesdev-wiki Color math blend ops
C073 | confirmed | A | snesdev-wiki CGWSEL bit 1 addend
C074 | (windowing before math) | A | CGWSEL region types (same chunk)
C075 | confirmed | A | anomie: only palettes 4-7 participate in color math
C076 | confirmed | A | fullsnes WH0-WH3 0..255
C077 | confirmed | A | idem
C078 | confirmed | A~ | "left > right = empty" not quoted; standard (snesdev-wiki Windows)
C079 | confirmed | A | snesdev-wiki Windows mask logic
C080 | confirmed | A | snesdev-wiki CGWSEL regions; TMW/TSW
C062 | — | C (settled) | anomie-regs $43x0: 101 = 2 regs write twice alternate (p,p+1,p,p+1); 110 = 1 reg write twice (p,p); 111 = 2 regs write twice each (p,p,p+1,p+1). dma.md mode 7 row wrong. FIX
C081 | confirmed | A | snesdev-wiki Tilemaps Mode 7 (128x128, low byte map, high byte tiles)
C082 | confirmed | A | undisbeliever cheat sheet M7A-D 1:7:8 write twice
C083 | confirmed | A | (C081)
C084 | confirmed | A | (C081) BG12NBA ignored implied by fixed layout
C085 | confirmed | A | snesdev-wiki M7SEL bits 7-6
C086 | confirmed | A | snesdev-wiki Backgrounds Mode 7 / EXTBG (doc should mention EXTBG BG2 exception)
C087 | confirmed | A | snesdev-wiki MOSAIC
C088 | confirmed | A | idem (0 = 1x1 … 15 = 16x16)
C089 | confirmed | A~ | anomie mosaic applied before hires combine; "half width in modes 5/6" consistent
C090 | confirmed | B | citation is the register layout, not the reset value; low risk (crt0 clears it anyway)
C091 | unsettled | B | vitor docs (complement/solid) agree: I-RAM at $0000-$07FF on the SA-1 side, $3000-$37FF S-CPU; "cannot access WRAM/PPU/APU" standard
C092 | confirmed | A | fullsnes SIWP "write enable flags"
C093 | unsettled | B | sneslab: CLSR bit 0 selects 21.48 MHz; opcode count decorative
C094 | unsettled | A~ | nintendo-devmanual-book2 §5.1.1.2 (reference): set PBR, SCBR, SCMR (RON must be 1), CFGR, CLSR, then write the program start address to R15 -> matches; fullsnes SCMR bits confirm RAN=bit3 RON=bit4 ($18)
C095 | unsettled | B | "S-CPU cannot read ROM while RON=1, run from WRAM, NMI off": dev-manual design rule; no arbiter passage surfaced — keep as design guidance, cite book2
C096 | confirmed(label) | B | citation is the uPD7725 (wrong chip); GSU branch delay slot is standard (fullsnes GSU opcodes) — targeted search optional, low risk
C097 | confirmed | A | fullsnes code-cache 512 bytes, CACHE sets CBR
C098 | confirmed | A | fullsnes pixel cache: flushed on full / RPIX / R1-R2 change
C099 | confirmed | A | fullsnes SCMR: MD 4-color/16/256, HT height, RAN bit 3, RON bit 4 ($19 = 16-color + RAN + RON + 128 px)
C100 | unsettled(label) | A | snesdev-wiki DSP-1: command $308000-$3FBFFF, status $30C000-$3FFFFF (Mode 20) — the wiki IS an arbiter; verify's ranking missed it (jsgroth quoted)
C101 | unsettled | A~ | same wiki chunk: status bit 7 = R (RQM); LSB-first word transfer standard
C102 | confirmed | B | operand types not in excerpt (wiki DSP-1 page covers them further down)
C103 | confirmed | A | snesdev-wiki DSP-1 command list
C104 | confirmed(label) | B | citation is the luna changelog (irrelevant); the ~50 µs / 40 % figures are SDK measurements — doc should say "measured on luna", not cite hardware
C105 | confirmed | A | anomie: order B, Y, Select, Start, Up, Down, Left, Right, A, X, L, R
C106 | confirmed | B | RDIO cited; "no controller reads $FFFF" follows from anomie's "then ones" but is not quoted verbatim — low risk
C107 | confirmed | A | snesdev-wiki Memory map + fullsnes SRAM lines (LoROM 70-7D:0000-7FFF; HiROM 30-3F:6000-7FFF)
C108 | confirmed | A | snesdev-wiki ROM header size encoding (1 KB << n)
C109 | confirmed | A~ | plain-RAM fact; citation is VRAM's restriction (contrast)
C110 | confirmed | A | 6502.org: MVN/MVP 7 cycles per byte
C111 | confirmed | A | anomie-timing / snesdev-wiki: 262 x 1364 = 357,368
C112 | confirmed | A | anomie DMA: A-bus/B-bus only; WRAM<->$2180 via DMA fails
C113 | (trivial) | A | little-endian
C114 | confirmed | A | fullsnes maths registers
C115 | confirmed | B | wiki Division page excerpt shows layout; the 8 / 16 CPU-cycle latencies are on the wiki Multiplication/Division pages (not quoted) — standard
C116 | confirmed | A | snesdev-wiki Tilemaps format VHPCCCTT…
C117 | confirmed | A~ | anomie tilemap sizes (sizes follow from 32x32 words)
C118 | confirmed | A | tile bytes 16/32/64 (standard, wiki Tiles)
C119 | confirmed | A | snesdev-wiki BG12NBA <<12; BGnSC <<10
C120 | confirmed | A | snesdev-wiki S-SMP memory layout
C121 | confirmed | A | snesdev-wiki APUIOn; mirrored $2140-$217F (anomie)
C122 | confirmed | A* (contested) | snesdev-wiki Errata quotes the dev manual's caution; the corpus flags the manual's crosstalk claim as itself suspect -> doc should say "the official manual advises 8-bit writes" rather than assert the corruption
C123 | confirmed | A | snesdev-wiki DSPADDR $F2/$F3
C124 | confirmed | A | snesdev-wiki S-DSP voice regs
C125 | confirmed | A | snesdev-wiki EDL: DDDD*2048 bytes, DDDD*512 samples = 16 ms
C126 | confirmed | A | anomie-sdsp BRR block ssssffle (note anomie: l = "don't end", e = "loop" naming quirk)
C127 | confirmed | B | filter coefficients not quoted (anomie-sdsp has them further) — standard
C128 | confirmed | A | snesdev-wiki DIR page
C129 | confirmed | A | anomie-spc700 IPL ROM ($AA/$BB, $CC handshake)
C096 | unsettled | A~ | ghidra-superfx (solid): "SuperFX hardware always executes one instruction after a taken branch" — consistent; no arbiter passage
C130 | confirmed | A | snesdev-wiki Init code (already cited in crt0)
C131 | confirmed | A | undisbeliever INIDISP
C132 | confirmed | A (derived) | 56 KB x 8 master cycles / 21.477 MHz = 21.4 ms
C133 | (trivial) | A | $2180-$2183 / BBAD $80
C134 | (trivial) | A | WRAM mirror (C006)
C135 | confirmed | A~ | composition of C046 + C050 (X bit 8 + Y=240)
C136 | confirmed | A | OAMDATA DMA 544 bytes
C137 | confirmed | A | NMITIMEN $81
C138 | confirmed | B | wiki: do not read JOYn while auto-read active; the ~4,224 master-cycle duration not quoted (anomie-timing has it) — standard
C139 | confirmed | A~ | anomie: standard pad returns ...L, R, 0, 0, 0, 0
C140 | confirmed | A | snesdev-wiki MEMSEL 6 vs 8 cycles
C141 | (trivial) | A |
C142 | confirmed | A | snesdev-wiki Controller reading $4016/$4017
C143 | confirmed | A | anomie-spc700 CPUIn/CPUOn latches
```
