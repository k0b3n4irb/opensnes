# Luna corpus coverage (whole-suite headless pass)

luna v0.2.0 · `luna run -n 3000000` per ROM · 57 ROMs · **57 OK, 0 SUSPECT (tiny/likely-blank), 0 FAIL**

> SUSPECT = rendered but the PNG is implausibly small (often a title/forced-blank screen that needs scripted input, not a real bug). FAIL = luna errored/timed out. PNGs: `/tmp/luna-test-corpus/`.

| Example | Status | Detail |
|---|---|---|
| `audio_snesmod_music_music` | OK | 5318 B |
| `audio_snesmod_music_hirom_music_hirom` | OK | 5008 B |
| `audio_snesmod_music_large_music_large` | OK | 5220 B |
| `audio_snesmod_sfx_sfx` | OK | 4648 B |
| `basics_aim_target_aim_target` | OK | 6560 B |
| `basics_collision_demo_collision_demo` | OK | 2683 B |
| `basics_fix32_orbit_fix32_orbit` | OK | 1405 B |
| `basics_random_random` | OK | 3284 B |
| `basics_scene_stack_scene_stack` | OK | 1971 B |
| `basics_timer_timer` | OK | 2249 B |
| `games_breakout_breakout` | OK | 18031 B |
| `games_likemario_likemario` | OK | 5552 B |
| `games_mapandobjects_mapandobjects` | OK | 9569 B |
| `games_shmup_1942_shmup_1942` | OK | 19157 B |
| `games_tetris_tetris` | OK | 5635 B |
| `graphics_backgrounds_continuous_scroll_continuous_scroll` | OK | 52881 B |
| `graphics_backgrounds_mixed_scroll_mixed_scroll` | OK | 2670 B |
| `graphics_backgrounds_mode0_mode0` | OK | 37675 B |
| `graphics_backgrounds_mode1_mode1` | OK | 2487 B |
| `graphics_backgrounds_mode1_bg3_priority_mode1_bg3_priority` | OK | 33493 B |
| `graphics_backgrounds_mode1_lz77_mode1_lz77` | OK | 2515 B |
| `graphics_backgrounds_mode3_mode3` | OK | 54958 B |
| `graphics_backgrounds_mode5_mode5` | OK | 41906 B |
| `graphics_backgrounds_mode7_mode7` | OK | 67239 B |
| `graphics_backgrounds_mode7_perspective_mode7_perspective` | OK | 38224 B |
| `graphics_effects_fading_fading` | OK | 2487 B |
| `graphics_effects_gradient_colors_gradient_colors` | OK | 2743 B |
| `graphics_effects_hdma_gradient_hdma_gradient` | OK | 2515 B |
| `graphics_effects_hdma_helpers_hdma_helpers` | OK | 5014 B |
| `graphics_effects_hdma_wave_hdma_wave` | OK | 1499 B |
| `graphics_effects_mosaic_mosaic` | OK | 2487 B |
| `graphics_effects_parallax_scrolling_parallax_scrolling` | OK | 40858 B |
| `graphics_effects_superfx_3d_superfx_3d` | OK | 2189 B |
| `graphics_effects_transparency_transparency` | OK | 38106 B |
| `graphics_effects_transparent_window_transparent_window` | OK | 18800 B |
| `graphics_effects_window_window` | OK | 4331 B |
| `graphics_sprites_animated_sprite_animated_sprite` | OK | 1810 B |
| `graphics_sprites_dynamic_metasprite_dynamic_metasprite` | OK | 5111 B |
| `graphics_sprites_dynamic_sprite_dynamic_sprite` | OK | 1723 B |
| `graphics_sprites_metasprite_metasprite` | OK | 5141 B |
| `graphics_sprites_object_size_object_size` | OK | 4664 B |
| `graphics_sprites_simple_sprite_simple_sprite` | OK | 2869 B |
| `input_controller_controller` | OK | 1957 B |
| `input_mouse_mouse` | OK | 2982 B |
| `input_superscope_superscope` | OK | 8243 B |
| `input_two_players_two_players` | OK | 1421 B |
| `maps_dynamic_map_dynamic_map` | OK | 15546 B |
| `maps_mapscroll_mapscroll` | OK | 9611 B |
| `maps_slopemario_slopemario` | OK | 6316 B |
| `maps_tiled_tiled` | OK | 29557 B |
| `memory_hirom_demo_hirom_demo` | OK | 1830 B |
| `memory_sa1_hello_sa1_hello` | OK | 2634 B |
| `memory_sa1_starfield_sa1_starfield` | OK | 3144 B |
| `memory_save_game_save_game` | OK | 3205 B |
| `memory_superfx_hello_superfx_hello` | OK | 5019 B |
| `text_hello_world_hello_world` | OK | 1712 B |
| `text_text_test_text_test` | OK | 1850 B |
