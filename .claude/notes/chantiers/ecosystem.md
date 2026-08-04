# Chantier: OpenSNES ecosystem — game-craft docs + dev tools

Status: **in progress** (Phase 1 = game-craft docs), started 2026-08-02.
Full plan: was `~/.claude/plans/jolly-wiggling-pascal.md`. This note is the
durable in-repo copy so the roadmap survives.

## Why

The SDK is strong; the ecosystem around it is thin. Three parallel web
research passes (game-craft education · SNES SDK tooling/DX · modern
retro-dev ecosystems) converged: **no SNES resource owns the seam between
"I know the API" and "I made a game."** Strategy = **curate + bridge**: link
world-class universal material, own only the SNES-specific on-hardware layer,
grounded in cited numbers.

## Track A — game-craft docs (`docs/craft/`)

Principles: link the universal / own the SNES-specific; every claim = a
number + a source (fullsnes, Anomie, SNESdev wiki, Bumbershoot); reference
`SNES_GRAPHICS_GUIDE.md` for hardware facts; anchor every guide to an
example; "constraints as creative fuel" framing.

- [Phase 1] `craft/README.md` — landing + curated "Go deeper" external library
- [Phase 1] `craft/planning.md` — VRAM/CGRAM/OAM budget worksheet +
  BG-mode-by-genre + first-game scoping (**biggest gap**)
- [Phase 1] `craft/backgrounds.md` — layer composition, priority, parallax, HUD
- [DONE 2026-08-02] `craft/tiles-to-levels.md` — metatiles as the design unit,
  tile attributes = gameplay (collision/palette/priority), streaming a level
  bigger than VRAM, the Tiled→gfx4snes→tmx2snes→mapLoad pipeline, map-engine vs
  own-array representation, grey-box level design. Credits nesdoug (metatiles) +
  SMW Central (level design). Anchors maps/{tiled,map_scroll,dynamic_map,
  slope_collision}, scrolling/continuous_scroll; cross-links planning (VRAM
  reuse) + frame-budget (column-stream DMA). Completes the core craft set.
- [DONE 2026-08-02] `craft/frame-budget.md` — DMA/VBlank + sprite-per-line +
  CPU as design levers; the *time* companion to planning's *space*. Ties to
  `make budget`, sprite_swarm, panel_hud (force-blank), mode2 (compute→VBlank DMA).
- [backlog] per-technique "craft companions" (camera→Scroll Back+HDMA parallax;
  game-feel→Juice It+color-math/scroll-shake)

### Curated external library (link, don't rewrite)
- Cameras: Itay Keren *Scroll Back* — https://docs.google.com/document/d/1iNSQIyNpVGHeak6isbP6AHdHD50gs8MNXF1GCf08efg/pub
- Game feel: *Juice It or Lose It* (GDC) — https://www.gdcvault.com/play/1016487/Juice-It-or-Lose
- PPU "why": Fabien Sanglard — https://fabiensanglard.net/snes_ppus_why/
- Visual explainers: Retro Game Mechanics Explained — https://www.youtube.com/c/RetroGameMechanicsExplained
- Metatiles: nesdoug — https://nesdoug.com/2018/09/05/11-metatiles/
- Level design: SMW Central — https://www.smwcentral.net/?p=beginners
- Pixel-art palettes: 2D Will Never Die — https://2dwillneverdie.com/tutorial/so-you-want-your-sprites-to-be-16-colors/
- Hardware refs: fullsnes https://problemkaputt.de/fullsnes.htm · Anomie (romhacking.net/documents/199) · SNESdev https://snes.nesdev.org/wiki · Bumbershoot (DMA budget) https://bumbershootsoft.wordpress.com/2023/10/14/dma-and-fastrom-on-the-snes-speed-at-any-cost/

### Best-practice numbers (verified, citeable)
- CGRAM 256 colours (512 B); convention BG 0–127 / sprites 128–255.
- Tile cost: 2bpp=16 B, 4bpp=32 B, 8bpp=64 B. Tilemap 32×32 = 2 KB, entry = 2 B.
- VRAM 64 KB = one shared pool (tilesets + tilemaps + OBJ tiles).
- ~4 KB safe DMA per VBlank (theoretical ~6 KiB; source: Bumbershoot/SNESdev).
- OAM 128 sprites; 32 sprites / 34 8×8 slivers per scanline; off-screen X=−256
  still counts (PPU bug); sliver-cull from lowest OAM priority.
- Modes by genre: platformer→1, RPG→1/3, racer→7, puzzle→0, per-column FX→2.

## Track B — tools & DX (ranked value ÷ effort; reuse > build)

1. **BRR/SFX sample tool** — DONE 2026-08-02 (`tools/wav2brr/`, wired into
   `make tools` + `make test-tools`). Did NOT wrap BRRtools/snesbrr: smconv
   already carries a self-contained BRR encoder (`tools/smconv/src/brr.c`, only
   stdlib/math deps), so wav2brr links *that* — output is byte-identical to a
   soundbank-baked sample, no new dependency, no new encoder to trust. Tool =
   WAV parse (PCM 8/16-bit, mono/stereo→mono downmix) → `brr_encode()` → `.brr`;
   `--loop START END` for looping samples. The play path already existed
   (`audioLoadSample`/`audioPlaySample`, raw-APU audio v2). Gap was purely
   *authoring* — every `.brr` in the tree was made with an external tool.
   Golden test = deterministic sine fixture, one-shot + loop cases. Docs paired:
   `docs/tutorials/audio.md` new "One-shot samples from WAV" section + options
   table row, `tools/README.md` row, `tools/wav2brr/README.md`. Anchored to
   `examples/audio/soundboard`.
   FOLLOW-UP DONE 2026-08-02: the `make/common.mk` `.wav`→`.brr` auto-rule
   (`%.brr: %.wav`, zero-config via the existing INCBIN_DEPS mechanism — drop a
   .wav, .incbin the .brr, done) + a dedicated example `examples/audio/sfx_from_wav`
   (A=blip, B=coin; two OpenSNES-original synthesized .wav, .brr generated not
   committed). Corpus 81→82: baseline + all 6 count claims + ATTRIBUTION updated.
   Remaining v2 idea: allow a per-file loop spec in the build rule (currently
   one-shot only; looping samples run wav2brr by hand).
2. **VRAM/CGRAM/OAM budget report** — DONE 2026-08-02 (`tools/luna-test/budget.py`,
   `make budget`). Runtime approach, not static: many examples build tiles in C
   (no asset files to sum), so it reads luna's `ppu.{vram,cgram,oam}_non_zero`
   footprint at the captured scene. Report-only; accepts an arbitrary ROM so a
   user can budget their own game. Pairs with `craft/planning.md`. Caveat:
   non-zero footprint = lower bound, single snapshot (not linker layout / peak);
   a high-water-mark refinement (scan the VRAM dump) is the obvious v2.
3. **`opensnes-starter`** — DONE 2026-08-02 (in-repo `starter/` form, user's
   pick). A complete movable-sprite game (main.c/data.asm/Makefile/README/
   .gitignore + a template `.github/workflows/build.yml`), OpenSNES-original
   32×32 PNG sprite through the real gfx4snes pipeline. Consumes the SDK via
   `OPENSNES=` (overridable; in-tree default = repo root). Shipped in the
   release zip (`make release` copies `starter/`), so extracting the zip and
   `make` in `opensnes/starter/` works zero-config. OpenSNES CI builds it via
   `make -C starter OPENSNES=$PWD` (proves the consumption contract).
   Reconciled with the EXISTING `opensnes init --template game` CLI scaffolder
   (scripts/opensnes): that stays the light single-file local scaffold
   (procedural tile, no git/CI); the starter is the heavier git-repo-with-CI +
   asset-pipeline path. Cross-referenced both ways (starter README ↔ main
   README ↔ CLI). Not created as a separate external GitHub repo — the in-repo
   dir is the source of truth, publishable as a template later. ATTRIBUTION
   entry added for the sprite.
4. **`palplan`** — project shared-palette planner (build on tiledpalettequant,
   MIT; locked-index repack-on-demand). #1 community pain; highest value.
5. **`aseprite2snes`** — Aseprite CLI JSON (tags→frames) → metasprite+anim.
6. **DX**: watch/live-reload (fswatch→Mesen2), linker→Mesen2 C-symbol export,
   curate ~8–10 examples as annotated "study carts".

### Tool sources / licenses
- SuperFamiconv (MIT) https://github.com/Optiroc/SuperFamiconv
- tiledpalettequant (MIT) https://github.com/rilden/tiledpalettequant
- BRRtools (MIT) https://github.com/Optiroc/BRRtools · snesbrr https://github.com/boldowa/snesbrr
- Aseprite CLI `-b --data --list-tags` (or LibreSprite GPLv2 to avoid EULA)
- devkitSMS quickstart template https://github.com/retcon85/quickstart-sms-devkitsms
- SNES Studio (MIT, UX reference) https://www.snes-studio.com/

## Verification
Docs: `make docs` (pages resolve) + `check_doc_render.py` + `make lint-docs`.
Tools: per-tool goldens in `make test-tools`; example integration + luna.
