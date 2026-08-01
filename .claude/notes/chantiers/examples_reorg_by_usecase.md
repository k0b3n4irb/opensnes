# Chantier — The example bundle, designed as a curriculum ("the ultimate tutorial")

Status: IN PROGRESS (2026-07-31). **7 waves + the naming audit shipped;
Audio and the Effects decomposition remain** (see the
Execution log at the bottom). The design below is the north star; the
Execution log tracks what is done and what remains, and the Migration
recipe distils the reusable procedure learned across the shipped waves.
The first of the two frontier projects (the other is
[`audio_beyond_snesmod.md`](audio_beyond_snesmod.md)); start here.

**Pivot (2026-07-29):** this chantier no longer "audits and re-homes the 74."
The maintainer's decision is **greenfield-first** — design the ideal example
tree from 0, as the table of contents of a book *"Developing with the
OpenSNES SDK — the ultimate tutorial,"* then map the existing 74 into it
(fit / accommodate / merge / discard). The curriculum leads; the corpus is
raw material.

## North star (the design philosophy)

A great bundle **accompanies a developer's journey** from first ROM to
shipped game — a CURRICULUM, not a feature catalog. Five rules:
1. **One example = one lesson**, visible on screen, documented (what / why /
   how / what next).
2. **Each topic is a ladder**: minimal → real (asset) → build-up → a final
   **"juicy" showcase rung that shows off the SNES**.
3. **Hardcoded-then-asset**: understand the hardware, then how you'd really
   do it.
4. **Show what makes the SNES special** — HDMA, colour math, Mode 7, mosaic,
   OAM tricks, windows — not generic demos.
5. **Games are capstones**; a tiny **fundamentals/under-the-hood** tier holds
   raw-hardware lessons.

Rung kinds: **[min]** minimal/hardcoded · **[asset]** asset-pipeline ·
**[build]** build-up · **[show]** juicy showcase · **[fund]** under-the-hood.

## The headline finding

**The library is ahead of the examples.** Iris wipe, water ripple,
shadow/tint colour math, S-DSP echo, the full `object` physics engine, the
`panel` HUD/dialog module — all shipped, all **unexampled**. So the backlog
is overwhelmingly **documenting existing power** (authoring), not building
lib features. Mapping tally over the 74: **68 FITS · 2 ACCOMMODATE · 2
MERGE→1 · 2 DISCARD.**

---

## Book table of contents (the greenfield tree)

Each rung cites the real lib entry point that makes it buildable.

### Family 0 — Getting started
0.1 First ROM: a colour on screen `[min]` (INIDISP/backdrop — `consoleInit`, `setColor`, `setScreenOn`)
0.2 Hello, SNES: print one line `[asset]` (`textInit`, `textPrintAt`, `textFlush`)
0.3 The frame: VBlank, NMI loop, frame counter `[build]` (`WaitForVBlank`, `getFrameCount`)
0.4 Link only what you need: modules & build `[build]` (`LIB_MODULES`, `make/common.mk`)
→ Fundamentals x-link: anatomy of a ROM (crt0, header, memory map, `data_init_end.o`).

### Family 1 — Text (the model ladder)
1.1 Print a string, built-in font `[min]` (`textInit`, `textPrint`, `textPrintHex`) ← `text_test`
1.2 Load a custom font from a PNG `[asset]` (`bgInitTileSetData`, `textLoadFont`)
1.3 Multiple fonts & styles `[build]` (`textLoadFont4bpp`, `setColor`)
1.4 Move / scroll a message `[build]` **NEW P1** (`bgSetScroll` on the text layer, `textSetPos`)
1.5 Text effects: typewriter, wave, fade, colour-cycle `[show]` **NEW P2** (`textPutChar` timed, `hdmaWaveH`, `fadeIn`, `setColor` cycling)
→ Fundamentals: glyph = 2bpp bitplanes + tilemap ← `text/hello_world` (demoted)

### Family 2 — Backgrounds
2.1 Mode 1 background from a PNG `[asset]` (`DECLARE_BG_ASSET`, `bgLoad`) ← `mode1`
2.2 A tilemap built by hand in C `[fund]` **NEW P3** (`dmaFillVRAM`, `dmaCopyVram`)
2.3 Compressed background (LZSS) `[asset]` (`LzssDecodeVram`) ← `mode1_lz77`
2.4 BG3 priority overlay for a HUD `[build]` (`bgInit`, BGx priority) ← `mode1_bg3_priority`
2.5 Mode 0: four 2bpp layers `[build]` (`setMode(0)`) ← `mode0`
2.6 Mode 3: 256-colour background `[show]` (`setMode(3)`) ← `mode3`
2.7 Mode 5: hi-res 512-wide + hi-res text `[show]` (`videoSetPseudoHires`, `videoSetInterlace`) ← `mode5`, `hires_text`

### Family 3 — Sprites
3.1 One static sprite `[min]` (`oamInit(OBJ_NAME_BASE())`, `oamSet`) ← `simple_sprite`
3.2 The six OBJ size modes `[build]` (`oamSetSize`) ← `object_size`
3.3 Animate a sprite (frames + H-flip) `[asset]` (`AnimClip`, `animPlay`) ← `animated_sprite`
3.4 Metasprite: one character, many tiles `[build]` (`oamDrawMeta`) ← `metasprite`
3.5 Dynamic sprite: stream tiles per frame `[build]` (`oamDynamicInit`, `oamDynamicDraw`) ← `dynamic_sprite`
3.6 Dynamic metasprite `[build]` (`oamDynamicSetSize`, `oamDrawMeta`) ← `dynamic_metasprite`
3.7 A swarm: all 128 OAM slots, flips, priority, culling `[show]` **NEW P2** (`oamHide`, `oamSet` flags)

### Family 4 — Input & peripherals
4.1 Read a joypad (held vs pressed) `[min]` (`padHeld`, `padPressed`) ← `controller`
4.2 Drive a sprite with the pad `[build]` (`padHeld` + `oamSetXY`)
4.3 Two players independently `[build]` (`padHeld(0/1)`, `padIsConnected`) ← `two_players`
4.4 The SNES Mouse `[build]` (`mouseInit`, `mouseGetX/Y`) ← `mouse`
4.5 The Super Scope `[show]` (`scopeInit`, `scopeGetX/Y`) ← `superscope`

### Family 5 — Scrolling & maps
5.1 Scroll one background `[min]` (`bgSetScroll`)
5.2 Parallax: layers at different rates `[build]` (`bgSetScrollX/Y` per layer) ← `mixed_scroll`, `mode0`
5.3 Per-scanline parallax via HDMA `[show]` (`hdmaParallax`) ← `parallax_scrolling`
5.4 Stream a world bigger than one screen (raw) `[build]` (`bgSetScroll` + `dmaCopyVram`) ← `continuous_scroll`
5.5 Scroll a map bigger than VRAM (map module) `[asset]` (`mapLoad`, `mapUpdateCamera`) ← `mapscroll`
5.6 Author a map in Tiled `[asset]` (tmx2snes + `mapLoad`) ← `tiled`
5.7 Tilemap-driven sprite; swap 32↔64 modes `[build]` (`mapGetMetaTile`) ← `dynamic_map`
5.8 Tile collision & slopes `[build]` (`collideTileEx`, `objCollidMapWithSlopes`) ← `slopemario`

### Family 6 — Colour & effects
**6a Fades & transitions:** 6a.1 fade in/out `[min]` (`fadeIn/Out`) ← `fading` · 6a.2 mosaic pixelate `[build]` (`mosaicFadeIn`) ← `mosaic` · 6a.3 iris wipe `[show]` **NEW P3** (`hdmaIrisWipe`)
**6b Palette tricks:** 6b.1 set/cycle one colour `[min]` **NEW P1** (`setColor`) · 6b.2 colour-cycling (waterfall/fire/lava) `[show]` **NEW P2** (`setColor` loop / `dmaCopyCGram`)
**6c Colour math:** 6c.1 blend two layers `[build]` (`colorMathTransparency50`) ← `transparency` · 6c.2 shadow & tint `[show]` **NEW P2** (`colorMathShadow`, `colorMathTint`) · 6c.3 direct colour `[show]` (`colorMathSetDirectColor`) ← `direct_color`
**6d Beyond 256 colours:** 6d.1 9-bit dither gradient `[show]` (`setColor`+INIDISP) ← `gradient_9bit` · 6d.2 1792 via H-IRQ CGRAM streaming `[show]` ← `hicolor_1792` · 6d.3 3840 via channel-split `[show]` (`colorMathSetChannel`) ← `hicolor_blend`
**6e HDMA & raster:** 6e.1 minimal single-channel HDMA `[min]` **NEW P3** (`hdmaSetup`, `hdmaEnable`) · 6e.2 backdrop gradient `[build]` (`hdmaGradient`) ← `gradient_colors` · 6e.3 indirect HDMA `[build]` ← `hdma_indirect_gradient` · 6e.4 wave: build the table, then use the helper `[build]` ← **MERGE `hdma_wave` + `hdma_wave_table`** · 6e.5 water ripple & the HDMA helper library `[show]` **NEW P3** (`hdmaWaterRipple`) ← absorbs `hdma_helpers`
**6f Windows:** 6f.1 one window `[min]` (`windowSetPos`) ← `window` · 6f.2 both windows per scanline `[build]` (`hdmaWindowShape`) ← `window_multi_hdma` · 6f.3 spotlight: window + colour math `[show]` ← `transparent_window`

### Family 7 — Mode 7
7.1 Rotate & scale `[asset]` (`mode7Init`, `mode7Rotate`) ← `mode7`
7.2 Fake perspective via HDMA matrix split (F-Zero) `[show]` (`mode7SetMatrix` under HDMA) ← `mode7_perspective`
7.3 Drive the full matrix per scanline `[show]` ← `mode7_perspective_rotate`
→ Capstones: `mode7_racing`, `mode7_flying` (Family 13).

### Family 8 — Audio
8.1 Play tracker music with transport `[asset]` (`snesmodInit`, `snesmodPlay`) ← `snesmod_music`
8.2 A large multi-bank soundbank `[build]` (`snesmodSetSoundbank`) ← `snesmod_music_large`
8.3 Mix SFX over music `[build]` (`snesmodPlayEffect`) ← `snesmod_sfx`
8.4 Soundboard: raw APU from C (audio v2) `[asset]` (`audioInit`, `audioPlaySampleEx`) ← `soundboard`
8.5 Hot-swap APU programs `[show]` (`apuUpload`, `apuExecute`) ← `apu_switch`
8.6 Drums from the noise generator `[show]` ← `play_noise`
8.7 Hardware vibrato `[show]` ← `pitch_mod`
8.8 BRR speech playback `[show]` ← `speech_synth`
8.9 Echo / reverb `[show]` **NEW P3** (`audioSetEcho`, `audioSetEchoFilter`)

### Family 9 — Data & memory
9.1 The asset pipeline: bundle tiles + palette `[asset]` (`DECLARE_BG_ASSET`, `gfxLoad`)
9.2 Compression (LZSS) `[build]` (`LzssDecodeVram`; x-link 2.3)
9.3 Where data lives: bank $00, far pointers, `ASSET_SECTION` `[fund]` **NEW P3** (`dmaCopyVramBank`, `templates/assets.inc`)
9.4 Battery SRAM saves `[build]` (`sramSave`, `sramChecksum`) ← `save_game`
9.5 Build for HiROM `[build]` (`USE_HIROM`) ← `hirom_demo`

### Family 10 — Game math & mechanics
10.1 Randomness `[min]` (`rand`, `srand`) ← `random`
10.2 Collision: AABB + tile `[build]` (`collideRect`, `collideTile`) ← `collision_demo`
10.3 Aim / distance / angle `[build]` (`atan2_8`, `sqrt16`, `fixSin`) ← `aim_target`
10.4 Fixed-point 16.16 motion `[build]` (`fix32Mul`, `fix32Lerp`) ← `fix32_orbit`
10.5 Physics & the retained-mode object engine `[show]` **NEW P2** (`objInitGravity`, `objNew`, `objUpdateAll`)

### Family 11 — Enhancement chips
11.1 SA-1: boot & I-RAM handshake `[min]` (`sa1Init`) ← `sa1_hello`
11.2 SA-1: offload real math (starfield) `[show]` ← `sa1_starfield`
11.3 Super FX: boot & hardware test `[min]` (`gsuInit`, `gsuLaunch`) ← `superfx_hello`
11.4 Super FX: 3D polygons `[show]` (`gsuDmaFullFrame`) ← `superfx_3d`

### Family 12 — Structure & framework
12.1 A main loop you don't write `[build]` (`gameLoopRun`) ← `timer`
12.2 Scene stack: title → play → pause `[build]` (`scenePush`, `scenePop`) ← `scene_stack`
12.3 HUD & dialog boxes `[show]` **NEW P2** (`panelInit`, `panelDraw`, `panelFlush`)
12.4 A minimal game skeleton `[build]` **NEW P2** (`gameLoopRun` + `scenePush` + `padHeld` + `oamSet`)

### Family 13 — Games (capstones)
breakout · tetris · likemario · mapandobjects · rpg · shmup_1942 · mode7_flying · mode7_racing — each annotated with the families it fuses, as an index back into the tree.

---

## Family ordering rationale

Follows a developer's dependency graph, not the PPU block diagram: Getting
started → Text (cheapest full tile/palette/VRAM/VBlank surface) → Backgrounds
→ Sprites → Input (after sprites, so 4.2 has something to drive) → Scrolling
& maps (needs bg + sprite) → Colour & effects (mid-book: now you can put an
effect on something *real*) → Mode 7 (its own render model, earns top
billing) → Audio (self-contained co-processor) → Data & memory → Game math →
Enhancement chips → Framework → Games (capstones + index back). One
refinement to the original arc: **Game math & mechanics** is split into its
own family (was implicit in Data & memory) and absorbs the `object` engine —
`object.h` deserves a teaching rung, not just a game cameo.

## Existing-example mapping (74) — verdict summary

**68 FITS** (re-homed to their rung, many just renamed). **2 ACCOMMODATE:**
`text/hello_world` → Fundamentals (glyph internals); `hdma_helpers` → folded
into the 6e.5 helper-library showcase. **2 MERGE→1:** `hdma_wave` +
`hdma_wave_table` → rung 6e.4 (the canonical hardcoded-then-asset
progression belongs in one build-up rung). **2 DISCARD** (below).

### DISCARD
- **`graphics/effects/hicolor_hires`** — a trick-on-a-trick; once 6d.2 (1792
  via CGRAM streaming) and 6d.3 (3840 via channel-split) teach their
  mechanisms cleanly, this corner-case interaction costs a reader more than
  it teaches. (Audit already flagged it "weakest/conditional-drop".)
- **`audio/snesmod_music_hirom`** — identical lesson to 8.1 with only the
  mapper changed; "mapper is orthogonal to audio" is exactly what 9.5 (build
  for HiROM) teaches. A second music ROM to prove it is redundant payload.

## Authoring backlog (NEW rungs)

**P1 (completes a core ladder, cited in `API_INDEX.md` with no example):**
1.4 move/scroll a message · 6b.1 set/cycle one colour (`setColor` shipped
unexampled).
**P2 (a family's showcase, buildable today, high payoff):** 1.5 text effects
· 6b.2 colour-cycling · 6c.2 shadow & tint · 10.5 object/physics engine ·
12.3 HUD & dialog (panel) · 12.4 minimal game skeleton · 3.7 sprite swarm.
**P3 (on-ramps & extra showcases):** 6e.1 minimal HDMA · 6a.3 iris wipe ·
6e.5 water ripple · 8.9 echo/reverb · 2.2 hand-built tilemap · 9.3 where
data lives · 0.1/0.3 first-ROM + frame loop.

## Coverage check — the library is ahead of the examples

Shipped-but-unexampled helpers each get a NEW authoring rung, **no lib work
needed**: `hdmaIrisWipe` (6a.3), `hdmaWaterRipple`/`hdmaBrightnessGradient`
(6e.5), `colorMathShadow`/`colorMathTint` (6c.2), `audioSetEcho` (8.9), the
full `object.h` (10.5), the full `panel.h` (12.3).

**Genuine (optional) "needs lib support"** — sugar the new examples would
motivate, not blockers:
- **text scroll/effects (1.4/1.5):** the `text` module has no scroll/effect
  helper; a small `textScroll`/`textEffect` would make 1.5 a few lines
  instead of a mini-engine. Buildable today without it.
- **palette cycling (6b.1/6b.2):** done today with a manual `setColor` loop;
  a `paletteCycle(bandStart, count, speed)` helper would one-line the classic
  effect and give `API_INDEX`'s "set one colour" a neighbour.

No showcase rung is *blocked* by a missing capability.

---

## Execution mechanics (unchanged from the audit version — kept for the build phase)

When migration begins, each example move touches **10 anchor systems**
atomically: (1) example `Makefile` `OPENSNES` depth; (2) `check_doc_drift.py`
(`check_example_paths`, `check_category_sums`, `check_screenshot_basenames`);
(3) `docs/Doxyfile` `IMAGE_PATH`; (4) `tools/luna-test/manifest.toml` keys;
(5) `tools/luna-test/baselines/<name>.png` (path-derived filename); (6)
Doxygen `@subpage` page-IDs (path-derived); (7) `LEARNING_PATH.md` +
`EXAMPLES_BY_CATEGORY.md`; (8) `API_INDEX.md` paths; (9) `docs/tutorials/*.md`
citations (**the one anchor not currently gated by a lint** → extend
`check_doc_drift.py` with a "no dangling old path" sweep); (10)
`examples/README.md`'s two tables. Migrate one family per wave, each green
(`make tests` + `make lint-docs`) before the next. Recommended pilot wave:
**Text** (renames + demote `hello_world` + author the P1 "move text" rung).

---

# Execution log & remaining plan (2026-07-30)

## Shipped waves (7 + naming audit)

Each wave: pure re-home unless noted (names/ROMs unchanged → baselines
re-keyed, not regenerated → fbhashes identical), family README rewritten to
the charter ladder, all 10 anchors updated, validated green
(`make lint-docs` + luna compare/coverage/probes, corpus fixed at 75).

| # | Wave | Moves | Result | Commit |
|---|------|-------|--------|--------|
| 1 | **Text** (pilot) | `text_test→text/print_string`, `hello_world→fundamentals/text_glyphs` + NEW `text/scroll_message` | new `fundamentals/`; corpus 74→75 | `dc1b70c0` |
| 2 | **Sprites** | `graphics/sprites/*→sprites/` (6) | new `sprites/` | `af240735` |
| 3 | **Enhancement chips** | `memory/{sa1_*,superfx_hello}` + `graphics/effects/superfx_3d` → `chips/` (4) | new `chips/` | `59442f1f` |
| 4 | **Mode 7** | `backgrounds/{mode7,mode7_perspective}` + `effects/mode7_perspective_rotate` → `mode7/` (3) | new `mode7/` | `05cbe29f` |
| 5a | **Scrolling** | `backgrounds/{mixed,continuous}_scroll` + `effects/parallax_scrolling` → `scrolling/` (3) | new `scrolling/` | `6388ec55` |
| 5b | **Backgrounds** | `graphics/backgrounds/mode*` (6) + `effects/hires_text` → `backgrounds/` (7) | new `backgrounds/`; `graphics/backgrounds/` gone | `eef43871` |
| 6 | **Maps** | none (family README only — fixed a 2-example gap) | maps ladder | `d04d27de` |
| 7 | **Input** | none + NEW `input/move_sprite` (rung 4.2) | corpus 75→76 | `3279390f` |
| — | **Naming audit** | 8 in-place renames across mode7/scrolling/backgrounds/maps/sprites | consistent per-family names | `d91286f3`,`1daa4de1`,`d020fbac`,`9fc7d31e`,`11aeb7b2`,`59f15a00` |

`graphics/` has shrunk **36 → 16** (only `effects/` remains). Corpus at **76**.
Naming audit renames: `rotate_scale`/`perspective`/`perspective_rotate`,
`parallax_scroll`, `mode5_hires`, `map_scroll`, `slope_collision`,
`sprite_sizes` (see the Rename addendum above for the two traps hit).

## The migration recipe (distilled from the 6 waves — follow this)

1. **Discover** every live ref: `grep -rlE '<source-paths>|<subpage-ids>'`
   over `*.md *.toml *.py Doxyfile`, EXCLUDING `.claude/ CHANGELOG.md
   /build/ CORPUS_COVERAGE test_check_doc_drift.py devtools/check_doc_drift.py
   README_TEMPLATE.md`. Also list tracked assets per example and manifest/probe refs.
2. **Move with `git mv` on the *directory*** — NEVER pre-`rm` build-artifact
   globs; examples carry **tracked assets** (`data.asm`, `res/*.png`, `.sfx`,
   `sa1_boot.asm`, `*.bin` LUTs, `vram_map.h`). `rm *.asm` deleted a tracked
   `data.asm` in the Sprites wave. Gitignored artifacts ride along harmlessly.
3. **Fix Makefile depth** if the example changed nesting level: moving
   `graphics/X/Y` (3-deep) up to `Y/` (2-deep) needs `../../../..` → `../../..`.
4. **Sed sweep** (paths + `@subpage` IDs) over the discovered files. The
   **shared-prefix trick**: `graphics/backgrounds/mode1` also correctly
   rewrites `mode1_bg3_priority`/`mode1_lz77` (suffix preserved). BUT scope by
   the **full source path** so sibling names elsewhere (`games/mode7_*`)
   aren't hit; verify siblings after.
5. **Bare prose cross-refs** (`` `backgrounds/mode7` `` without the `graphics/`
   prefix) escape the absolute sed — add the bare form, and grep after.
6. **Relative markdown links** (`](../../backgrounds/mode3/)`) from a
   *non-moved* example to a *moved* one break on a depth change and are NOT
   lint-gated. After each wave: `grep -rnE '\]\(\.\./[^)]*<moved-name>/'` and
   fix the `../` depth by hand.
7. **Baselines**: `git mv` the `<key>.png` (+ multi-point `<key>@<steps>.png`),
   plain-`mv` the untracked `.wdm.txt`, and re-key `baselines.json` by prefix
   (preserves scalar OR `fbhash`-list schema; ROMs byte-identical → values
   unchanged). Never regenerate on a pure re-home.
8. **Category table** (`examples/README.md`): subtract from the source
   category, add the new one, keep the sum = corpus count. **Do NOT trust
   mental math** — `check_category_sums` (dir-count vs claimed) is the safety
   net; let `make lint-docs` verify (it caught a 5-3=3 slip).
9. **Family README** rewritten to the charter ladder (developer questions,
   `[kind]`, showcase last, cross-links). Manifest entries + probes re-keyed
   by the path sed. `CORPUS_COVERAGE.md` regenerates — don't hand-edit.
10. **Validate**: build each moved example, `make lint-docs`, luna
    `--compare` (byte-identical → must PASS) + `--coverage` (0 FAIL) +
    `probes/run_all.py`, and — for visual families — a **per-example luna
    screenshot viewed** to confirm the render. Commit one wave = one
    `docs(examples):` commit.

### Rename addendum (a rename is NOT a move — two extra traps)

A pure *move* keeps the example's name (ROM filename, `TARGET`, symbols); a
*rename* changes them. The naming-audit pass (mode7/scrolling/backgrounds/
maps/sprites, 2026-07-31) hit two traps a move never does — both caught by
validation, both worth pre-empting:

- **The ROM filename changes too** (`TARGET := <old>.sfc` → `<new>.sfc`).
  Probe/manifest refs are `dir/<name>.sfc`, so a **dir-scoped** sweep
  (`s|mode7/mode7_perspective|mode7/perspective|`) fixes the directory but
  leaves the *filename* (`.../mode7_perspective.sfc`) dangling. Use a
  **blanket identifier sed** (`s|<old>|<new>|g`, which catches both the dir
  and the `<old>.sfc`) — that is why `parallax_scrolling` was clean but the
  dir-scoped mode7 sweep broke `movement.py` (fix `d020fbac`). Grep the
  `<old>.sfc` form after.
- **The blanket doc-sed does NOT include the Makefile.** The file-list glob
  is `*.md *.toml *.py Doxyfile` — so `TARGET` keeps the old name, the build
  emits `<old>.sfc`, and the stale-artifact cleanup then deletes it → **no
  `.sfc` → discovery drops the example** (76→75, silent). Always
  `sed` the Makefile `TARGET` **explicitly**, and **validate before
  cleaning** stale ROMs (so a missing `.sfc` fails compare/coverage
  immediately, not after commit). Also `git mv` any dedicated probe file
  (`probes/<name>.py`) — `run_all.py` globs `probes/*.py`, so a rename is
  picked up automatically.

Renames stay **byte-identical** if you keep `ROM_NAME` (the cosmetic 21-char
cartridge header) — then baselines re-key, no regeneration.

### Two more traps (Audio wave / naming audit, 2026-07-31)

- **`.incbin` reaches ACROSS the tree — grep beyond the example.** The Sprites
  move and the maps rename broke `devtools/benchrom` and `devtools/libtests`,
  which `.incbin` example `res/*` assets by path (`../../examples/maps/mapscroll/
  res/...`). Those are `.asm`, outside the doc-sed pattern, and **devtools are
  NOT in the luna corpus validation**, so the break was silent. After any
  move/rename: `grep -rn '<old path>'` across **all** file types (`.c .h .asm
  Makefile`, not just docs) AND `make -C devtools/benchrom && make -C
  devtools/libtests`. Keep `Ported from PVSnesLib <name>` provenance lines —
  they name the upstream original, not our example.
- **`git commit` commits the whole index, not the path you just `git add`ed.**
  A discard staged before a break (`git rm` example + baseline) got swept into
  an unrelated later commit. When work sits staged, `git status` before every
  commit, or commit explicit paths (`git commit -- <paths>`).

## Remaining waves (simplest → most complex)

### Wave 6 — In-place families (Maps, Input, Audio): enrichment, ~0 moves
These already live in clean topic dirs; the "wave" is a charter family
README + the ladder's missing rungs. Do as three small commits or one.
- **Maps** — family README ladder (mapscroll → tiled → dynamic_map →
  slopemario). Zero moves, zero new. Trivial.
- **Input** — family README + author the **NEW rung 4.2 "drive a sprite with
  the pad"** (`input/move_sprite`, builds on `simple_sprite`+`controller`).
  Corpus 75→76.
- **Audio** — family README + **DISCARD `snesmod_music_hirom`** (identical
  lesson to `snesmod_music`, only the mapper differs — that is what
  `memory/hirom_demo` teaches; net −1) + author the **NEW rung 8.9 echo/reverb**
  (`audioSetEcho` ships unexampled; net +1). First DISCARD of the chantier:
  `git rm` the dir, drop its baseline key + png, drop the `IMAGE_PATH` line,
  drop its nav rows, decrement the audio count.

### Wave 7 — Effects decomposition (the big one; removes graphics/ entirely)
`graphics/effects/` (16) splits into four topic families. Do as four
sub-waves (like 5a/5b) so each stays digestible; after the last,
`graphics/` is empty → `rmdir`.
- **7a `hdma/` (HDMA & raster)** — `gradient_colors`, `hdma_indirect_gradient`,
  `hdma_helpers` move; **MERGE `hdma_wave` + `hdma_wave_table` → one rung 6e.4**
  ("build the table, then use the helper" — the canonical hardcoded-then-asset
  progression). The merge is a new op: keep one dir, fold the other's lesson
  into its README/main.c, `git rm` the second, drop its baseline+nav. Optional
  NEW: 6e.1 minimal single-channel HDMA, 6e.5 water ripple.
- **7b `colour/` (Colour & hi-colour)** — `transparency`, `direct_color`,
  `gradient_9bit`, `hicolor_1792`, `hicolor_blend` move; **DISCARD
  `hicolor_hires`** (trick-on-a-trick, superseded). Optional NEW: 6b.1
  set/cycle a palette colour (P1 — `setColor` unexampled), 6c.2 shadow & tint.
- **7c `windows/` (Windows)** — `window`, `window_multi_hdma`,
  `transparent_window` move. Fix the `hdma_helpers→mode3` relative link again
  once hdma_helpers lands in `hdma/`.
- **7d `transitions/` (Transitions)** — `fading`, `mosaic` move. Optional NEW:
  6a.3 iris wipe (`hdmaIrisWipe` unexampled). Then `rmdir examples/graphics`.

## Authoring backlog (independent track — new rungs, per the charter)
Most families are complete ladders; these are the showcase/gap rungs the
greenfield audit flagged, buildable today (the lib is ahead of the examples).
Write them any time, family by family: P1 `set/cycle a palette colour`; P2
`text effects`, `colour-cycling`, `shadow & tint`, `object`/physics engine,
`panel` HUD/dialog, minimal game skeleton, sprite swarm; P3 minimal HDMA,
iris wipe, water ripple, echo, hand-built tilemap, "where data lives",
first-ROM colour. (`text/scroll_message` = the P1 "move text" rung, already
shipped in wave 1.)

## Final cleanup (after wave 7)
- `rmdir examples/graphics` (empty).
- Grep `graphics/effects`, `graphics/backgrounds`, `graphics/sprites` across
  live docs → zero residual.
- `docs/mainpage.md` / nav: confirm no stale `graphics/` grouping.
- Full `make clean && make` + `make tests`; refresh `CORPUS_COVERAGE.md`.
- Consider whether `graphics/` as a category label should disappear from
  `examples/README.md` once it holds 0.
