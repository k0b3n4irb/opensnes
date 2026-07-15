# Chantier: Interlace / hi-res port (#108) — video.h surface

Status: IN PROGRESS (started 2026-07-15). Arc effets 3/7 (dernier gros).

## Prerequisite done (2026-07-15)

luna check: all 6 krom Interlace demos render correct CONTENT in luna
v1.9.0; the capture pipeline emits 256×224 (≈2×2 downsample of the
512×448 field pair) → luna#115 filed for native-res capture (NOT a
blocker; content-level validation works). Posted on #108.

## Reference facts (full-ASM read of the family)

All six: BGMODE=$0D (Mode 5, priority 1, BG1 16×8 tiles),
SETINI=$01 (screen interlace), **TM AND TS both = BG1** (interlace
displays through main+sub — krom's own comment; the classic trap),
BG1SC 64×32 map. Font: map at word $4000, tiles at word $8000,
16-color. Variants: Myst/Simpsons add HDMA overlays, RPG adds
sprites + OBSEL + obj-interlace.

## Plan

1. lib `video.h`/video.c (or console.c): `setini_shadow` (crt0 sysvar,
   NMITIMEN-shadow pattern from #113) + setters:
   - videoSetInterlace(u8 on)      — SETINI bit 0
   - videoSetObjInterlace(u8 on)   — bit 1
   - videoSetOverscan(u8 on)       — bit 2 (239 lines)
   - videoSetPseudoHires(u8 on)    — bit 3 (mode-1 512 blend)
   Doxygen: TM+TS requirement, modes 5/6 tile geometry (16×8),
   overscan interaction with NMI timing (V=240 vs 224 — check crt0
   assumptions before shipping overscan!).
2. Port target: **InterlaceFont** → `examples/graphics/effects/
   hires_text` (own 16×8 hi-res font — generate procedurally, krom
   credited for technique). Init: mode 5 via setMode(BG_MODE5, 0x08),
   bgSetMapPtr 64x32, videoSetInterlace(1), setMainScreen+setSubScreen.
3. Validation: content-level luna parity vs krom Font demo (256×224
   downsample domain), upgrade to native 512×448 when luna#115 ships.
   Probes: SETINI shadow via peek; visual: legible text rows.

## Watch out

- Overscan (bit 2) changes VBlank start line 224→239 — crt0 NMI
  assumptions + luna coverage liveness may care. Ship the setter with
  a @warning; the port itself uses interlace only.
- The examples-count anchors (docs) go 60→61 with the new example.
- WRAM baselines: new sysvar setini_shadow shifts .system → corpus
  WRAM rebaseline expected (same class as in_nmi_ctx).

## State

- [x] luna support check + luna#115 filed + #108 comment
- [ ] video surface (shadow + setters) + header docs
- [ ] hires_text example (own font) on wip/interlace
- [ ] validation + docs counts + merge + close #108
