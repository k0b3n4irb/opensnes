# Luna corpus coverage (whole-suite headless liveness pass)

luna v1.1.0 · `luna state -n <steps>` per ROM · 56 ROMs · **54 OK, 2 INPUT-DEP, 0 DEAD, 0 FAIL**

> Liveness from `luna state` (NMI/VBlank advancing, CPU not halted) — not a PNG-size heuristic. **INPUT-DEP** = runs+renders but its device input (Mouse/Super Scope, gap G4) is unmodelled → boot+visual only, *not* a clean functional pass. **DEAD** = ran but not live (crash/hang). **FAIL** = luna errored. PNGs: `/tmp/luna-test-corpus/`. (In-ROM `SNES_ASSERT`/WDM is caught separately by the visual pass via `--wdm-out`.)

| Example | Status | Detail |
|---|---|---|
| `audio/snesmod_music` | OK | live (118f/109nmi) |
| `audio/snesmod_music_hirom` | OK | live (131f/122nmi) |
| `audio/snesmod_music_large` | OK | live (128f/119nmi) |
| `audio/snesmod_sfx` | OK | live (97f/88nmi) |
| `basics/aim_target` | OK | live (77f/77nmi) |
| `basics/collision_demo` | OK | live (82f/81nmi) |
| `basics/fix32_orbit` | OK | live (76f/75nmi) |
| `basics/random` | OK | live (298f/297nmi) |
| `basics/scene_stack` | OK | live (72f/71nmi) |
| `basics/timer` | OK | live (184f/183nmi) |
| `games/breakout` | OK | live (85f/84nmi) |
| `games/likemario` | OK | live (107f/98nmi) |
| `games/mapandobjects` | OK | live (88f/87nmi) |
| `games/shmup_1942` | OK | live (151f/150nmi) |
| `games/tetris` | OK | live (89f/80nmi) |
| `graphics/backgrounds/continuous_scroll` | OK | live (74f/73nmi) |
| `graphics/backgrounds/mixed_scroll` | OK | live (71f/70nmi) |
| `graphics/backgrounds/mode0` | OK | live (71f/70nmi) |
| `graphics/backgrounds/mode1` | OK | live (70f/70nmi) |
| `graphics/backgrounds/mode1_bg3_priority` | OK | live (71f/70nmi) |
| `graphics/backgrounds/mode1_lz77` | OK | live (74f/73nmi) |
| `graphics/backgrounds/mode3` | OK | live (71f/70nmi) |
| `graphics/backgrounds/mode5` | OK | live (71f/70nmi) |
| `graphics/backgrounds/mode7` | OK | live (71f/71nmi) |
| `graphics/backgrounds/mode7_perspective` | OK | live (74f/73nmi) |
| `graphics/effects/fading` | OK | live (71f/70nmi) |
| `graphics/effects/gradient_colors` | OK | live (72f/71nmi) |
| `graphics/effects/hdma_helpers` | OK | live (71f/70nmi) |
| `graphics/effects/hdma_wave` | OK | live (73f/72nmi) |
| `graphics/effects/mosaic` | OK | live (71f/70nmi) |
| `graphics/effects/parallax_scrolling` | OK | live (72f/71nmi) |
| `graphics/effects/superfx_3d` | OK | live (289f/145nmi) |
| `graphics/effects/transparency` | OK | live (71f/70nmi) |
| `graphics/effects/transparent_window` | OK | live (72f/71nmi) |
| `graphics/effects/window` | OK | live (72f/71nmi) |
| `graphics/sprites/animated_sprite` | OK | live (73f/72nmi) |
| `graphics/sprites/dynamic_metasprite` | OK | live (89f/88nmi) |
| `graphics/sprites/dynamic_sprite` | OK | live (75f/75nmi) |
| `graphics/sprites/metasprite` | OK | live (93f/92nmi) |
| `graphics/sprites/object_size` | OK | live (75f/74nmi) |
| `graphics/sprites/simple_sprite` | OK | live (71f/70nmi) |
| `input/controller` | OK | live (109f/108nmi) |
| `input/mouse` | INPUT-DEP | live (73f/72nmi) |
| `input/superscope` | INPUT-DEP | live (73f/72nmi) |
| `input/two_players` | OK | live (75f/74nmi) |
| `maps/dynamic_map` | OK | live (79f/78nmi) |
| `maps/mapscroll` | OK | live (76f/75nmi) |
| `maps/slopemario` | OK | live (87f/86nmi) |
| `maps/tiled` | OK | live (73f/72nmi) |
| `memory/hirom_demo` | OK | live (71f/70nmi) |
| `memory/sa1_hello` | OK | live (72f/71nmi) |
| `memory/sa1_starfield` | OK | live (120f/119nmi) |
| `memory/save_game` | OK | live (73f/72nmi) |
| `memory/superfx_hello` | OK | live (74f/73nmi) |
| `text/hello_world` | OK | live (71f/70nmi) |
| `text/text_test` | OK | live (71f/70nmi) |
