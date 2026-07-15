# Chantier: Mode 7 per-line HDMA port (#110) — Perspective

Status: IN PROGRESS (started 2026-07-14). Predecessor: hicolor_port.md
(pattern + IRQ surface). Target decided and posted on #110:
**`luna_tests/PPU/Mode7/Perspective`** (StarWars' scroller is a 2-channel
subset buried under a bass text-crawl pipeline — no SDK surface there).

## Reference facts (full-ASM read, Perspective.asm — 311 lines)

- Mode 7 (BGMODE=$07), BG1 to main, M7SEL=$80 (screen-over repeat? verify
  bit semantics at port time), 256-color BG.
- Assets: BG.pal 512 B, BG.map 16384 B, BG.pic 10944 B, loaded via
  LoadM7VRAM (Mode 7 interleaved VRAM: map in low bytes, tiles in high).
  Our lib has `dmaCopyVramMode7(tilemap, size, tiles, size)` for this.
- **4 HDMA channels, one per matrix register**:
  ch0 M7A←M7COSTable (bank 1), ch1 M7B←M7SINTable (bank 2),
  ch2 M7C←M7NSINTable (bank 3), ch3 M7D←M7COSTable (bank 1);
  DMAP=%00000010 (mode 2: 1 reg written twice = 16-bit/line), HDMAEN=$0F.
- 48 angle tables × ~224 lines each per trig function; per-frame the main
  loop repoints all four A1Tx to `M7COSTable[angle]` (NOTE: krom points
  ALL FOUR channels' A1Tx to the same cos-table pointer array entry —
  looks like a bug for ch1/ch2 (sin tables live in other banks at the
  same offsets?) — verify: A1Bx stay fixed (1/2/3) so the 16-bit offset
  is shared but the BANK differs → tables must be offset-aligned across
  banks. Confirmed by design: same label offsets in each bank.)
- Main loop (NMI-paced): writes BG1HOFS/VOFS + M7X/M7Y from WRAM shadows,
  repoints tables by angle, reads joypad (dpad move, L/R rotate ±,
  X/B FOV, Y/A pivot — the FOV/pivot handlers were NOT read in detail
  yet, lines 215-241 only cover Y/A/X/B as pos tweaks; re-read before
  claiming input parity).
- Init positions: BG1Scr=(384,768), M7Pos=(512,1152), angle 0.

## SCOPE CORRECTION (2026-07-14, post-decision)

`examples/graphics/backgrounds/mode7_perspective` ALREADY does M7A/M7D
per-line HDMA (F-Zero split-screen, no rotation). The #106 audit
under-weighted it. The port's REAL delta = **full-matrix rotation**
(M7B/M7C sin channels, 48 angles, L/R interactive). Example renamed:
`examples/graphics/effects/mode7_perspective_rotate` (res/ already
holds the extracted tables).

## Table math (REVERSE-ENGINEERED, 32256/32256 entries explained)

`entry(a, y) = trig(2*pi*a/48) * 20480 / y` — 8.8 fixed point
(20480 = 80*256), y = scanline 1..224, trig = cos / sin / -sin.
Rounding convention mixes trunc/round vs Python float — for byte
identity we ship krom's bytes verbatim (res/m7{cos,sin,nsin}.bin,
48 tables x 673 B stride = [1][val16]x224 + terminator each) and the
devtool documents/verifies the math instead of regenerating.

## Port plan

1. Extract krom's 3 table banks verbatim (`dw` lines → .bin) — incbin.
2. Devtool `devtools/m7ptables.py`: regenerate from math (cos/sin ×
   per-line perspective divide), prove byte-identity vs extraction —
   documents the math instead of shipping magic numbers.
3. Example `examples/graphics/effects/mode7_perspective`: hdmaSetup ×4
   (existing API, same mode as wave port), dmaCopyVramMode7 assets,
   pad input via lib, WRAM shadows synced in main loop.
   Own art for the floor (procedural track/checkerboard, krom credited
   for technique + tables math).
4. Validation per pattern: reference renders in luna ✓ (2026-07-14,
   perspective floor OK, frames stable check pending), luna frames
   parity, input probes (rotate/move), README fidelity table.

## State

- [x] Full-ASM read of Perspective + StarWars technique inventory
- [x] Reference runs in luna (force-mapper), floor renders
- [x] Decision posted on #110
- [ ] Table extraction + devtool byte-identity proof
- [ ] Example implementation (wip/m7perspective branch)
- [ ] Pattern validation + merge + close #110

## Session checkpoint (2026-07-14 soir)

Implemented on `wip/m7rotate`: example builds and renders perspective +
tables verbatim (res/m7{cos,sin,nsin}.bin) + tile-conscious 9-tile track
art (flat colors — 1px dither aliases horribly under Mode 7 sampling)
+ HDMA_DEST_M7B/C/D added to hdma.h. hdmaSetup x4 per frame works.

**OPEN QUESTION (blocking the final look)**: with krom's exact pose
(scr 384,768 / pivot 512,1152) and M7SEL=$80 verified in the PPU state,
krom's reference shows a BLACK band (out-of-bounds transparent,
pivot outside the 1024px world) while ours WRAPS (grass from y mod
1024). Same m7sel value, different out-of-bounds behavior — next
step: dump krom's full PPU state in luna side by side; suspect either
(a) my misreading of which screen area is out-of-bounds, (b) a
tile-0-fill difference, or (c) art-dependent: krom's world corners are
NOT black (checked: red/green), so his black band is genuinely the
transparent path. Compare line-by-line world sampling before touching
anything else. Camera pose for OUR art (ring r=280-436 centered
512,512) also still frames too close — consider shrinking the ring
(radii /2) so the circuit fits krom's framing.

Validation TODO (pattern): side-by-side stills, rotation parity probe
(L press x N → angle register/table offset check via dma.channels[]),
input probes, README fidelity table, docs counts (60 examples!),
baselines, WRAM rebaseline (Class C only — no crt0 this time), CI.

## Session 2 (2026-07-14/15) — RESOLVED, port behaviorally exact

The "wrap vs black" open question dissolved: **HDMA was never enabled**
(`hdmaSetup` configures only — `hdmaEnable(0x0F)` is a separate call;
the wave/gradient examples do call it). The un-enabled state rendered a
static 1:1 view of the ring whose curbs mimicked a perspective — a
convincing decoy. Diagnosis: luna's `dma.hdmaen: 0` + the point-sampled
projection model (which matched krom's render exactly and exposed ours
as identity-matrix). Out-of-map = backdrop = CGRAM color 0 (white from
gfx4snes's palette → forced black at init like krom's).

Second finding: **hdmaSetup from an nmiSet callback silently no-ops**
(m7x/m7y writes from the same callback DO land) → filed #113. Port uses
krom's own placement (main loop right after WaitForVBlank — still in
VBlank).

**Parity achieved**: tables byte-identical (m7ptables.py verify
32256/32256); in-bounds point-samples match through the documented
matrix model; at +90° rotation both ROMs show the identical black
screen + green wedge — **590 non-black px in both, IoU = 1.0000**;
at 180° both fully black (pivot outside the world, reference's own
behavior). Landmarks (4 colored infield markers) added for rotation
readability.

Remaining: suite green + docs (60) done, commit on wip/m7rotate,
user triage, squash-merge, CI, close #110 + note on #4.
