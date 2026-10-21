# Luna corpus coverage (whole-suite headless liveness pass)

luna v1.40.0 · `luna state --until-frame <N>` per ROM · 60 ROMs · **58 OK, 2 INPUT-DEP, 0 DEAD, 0 FAIL**

> Liveness from `luna state` (NMI/VBlank advancing, CPU not halted, and the latest NMI within one frame of the capture) — not a PNG-size heuristic. **INPUT-DEP** = runs+renders but its device input (Mouse/Super Scope, gap G4) is unmodelled → boot+visual only, *not* a clean functional pass. **DEAD** = ran but not live (crash/hang). **FAIL** = luna errored, or the ROM header's size byte does not cover the file or its checksum complement does not match. PNGs: `/tmp/luna-test-corpus/`. (In-ROM `SNES_ASSERT`/WDM is caught separately by the visual pass via `--wdm-out`.)

| Example | Status | Detail |
|---|---|---|
| `audio/apu_switch` | OK | live (200f/198nmi) |
| `audio/sfx_from_wav` | OK | live (200f/198nmi) |
| `audio/snesmod_music` | OK | live (200f/190nmi) |
| `audio/soundboard` | OK | live (200f/198nmi) |
| `audio/speech_synth` | OK | live (200f/198nmi) |
| `backgrounds/mode0` | OK | live (400f/398nmi) |
| `backgrounds/mode1` | OK | live (200f/198nmi) |
| `backgrounds/mode2` | OK | live (400f/398nmi) |
| `backgrounds/mode3` | OK | live (200f/198nmi) |
| `backgrounds/mode5_hires` | OK | live (200f/198nmi) |
| `basics/aim_target` | OK | live (200f/198nmi) |
| `basics/collision_demo` | OK | live (200f/198nmi) |
| `basics/game_skeleton` | OK | live (200f/198nmi) |
| `basics/panel_hud` | OK | live (200f/198nmi) |
| `basics/scene_stack` | OK | live (200f/198nmi) |
| `chips/sa1_starfield` | OK | live (400f/398nmi) |
| `chips/superfx_3d` | OK | live (400f/397nmi) |
| `chips/superfx_game_skeleton` | OK | live (400f/389nmi) |
| `color/direct_color` | OK | live (200f/198nmi) |
| `color/hicolor_1792` | OK | live (200f/198nmi) |
| `color/palette_cycle` | OK | live (400f/398nmi) |
| `color/shadow_tint` | OK | live (200f/198nmi) |
| `color/transparency` | OK | live (400f/398nmi) |
| `demos/mapandobjects` | OK | live (200f/198nmi) |
| `demos/rpg` | OK | live (200f/198nmi) |
| `demos/shmup_1942` | OK | live (400f/398nmi) |
| `fundamentals/text_glyphs` | OK | live (200f/198nmi) |
| `games/blocks` | OK | live (400f/390nmi) |
| `games/breakout` | OK | live (200f/198nmi) |
| `games/likemario` | OK | live (200f/190nmi) |
| `hdma/gradient_colors` | OK | live (200f/198nmi) |
| `hdma/hdma_helpers` | OK | live (200f/198nmi) |
| `hdma/hdma_wave` | OK | live (400f/398nmi) |
| `input/controller` | OK | live (200f/198nmi) |
| `input/mouse` | INPUT-DEP | live (200f/198nmi) |
| `input/superscope` | INPUT-DEP | live (200f/198nmi) |
| `maps/map_scroll` | OK | live (200f/198nmi) |
| `memory/hirom_demo` | OK | live (200f/198nmi) |
| `memory/save_game` | OK | live (200f/198nmi) |
| `mode7/dsp1_ground` | OK | live (400f/398nmi) |
| `mode7/extbg` | OK | live (400f/398nmi) |
| `mode7/mode7_racing` | OK | live (200f/198nmi) |
| `mode7/perspective` | OK | live (200f/198nmi) |
| `mode7/perspective_rotate` | OK | live (200f/198nmi) |
| `mode7/rotate_scale` | OK | live (200f/198nmi) |
| `scrolling/continuous_scroll` | OK | live (200f/198nmi) |
| `scrolling/mixed_scroll` | OK | live (400f/398nmi) |
| `scrolling/parallax_scroll` | OK | live (400f/398nmi) |
| `sprites/animated_sprite` | OK | live (200f/198nmi) |
| `sprites/aseprite_pipeline` | OK | live (400f/398nmi) |
| `sprites/dynamic_sprite` | OK | live (260f/258nmi) |
| `sprites/simple_sprite` | OK | live (200f/198nmi) |
| `sprites/sprite_sizes` | OK | live (200f/198nmi) |
| `sprites/sprite_swarm` | OK | live (400f/398nmi) |
| `sprites/world_sprites` | OK | live (200f/198nmi) |
| `text/pixel_text` | OK | live (200f/198nmi) |
| `text/print_string` | OK | live (200f/198nmi) |
| `transitions/fading` | OK | live (200f/198nmi) |
| `windows/window` | OK | live (200f/198nmi) |
| `windows/window_multi_hdma` | OK | live (200f/198nmi) |
