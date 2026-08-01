# Learn SNES Development {#learning_path}

A curated path through the 75 examples — not a feature tour, a **developer's
journey**. You don't wake up wanting "Mode 7"; you wake up with a question
about the game you're building, and it changes as you go. Each stage below is
one of those questions, and the confidence it buys you. This is a hand-picked
subset; for the exhaustive index see @ref examples_by_category.

At every step the goal is the same: never a wall you can't climb with what
you just learned, always a result that works, always the *why*.

## Stage 0 — "Does my setup even work?"

*Confidence: the toolchain isn't scary.* Get one ROM built and running, and
learn the rhythm of a frame — you set things up during blanking, then let the
machine run at 60 Hz.

@subpage examples_text_print_string

## Stage 1 — "Can I put something on screen?"

*Confidence: I control the picture.* Text, a background from your own art, a
sprite that isn't tied to the grid. The tile / palette / VRAM trinity that
underpins everything visual on the SNES.

@subpage examples_text_scroll_message

@subpage examples_backgrounds_mode1

@subpage examples_sprites_simple_sprite

@subpage examples_sprites_animated_sprite

@subpage examples_sprites_sprite_swarm

> Under the hood — how a glyph becomes pixels with no module at all:
> @subpage examples_fundamentals_text_glyphs

## Stage 2 — "Can the player act?"

*Confidence: it's a game, not a demo.* Read the pad, move something with it,
then branch out to the SNES's exotic controllers.

@subpage examples_input_controller

@subpage examples_input_two_players

@subpage examples_input_mouse

@subpage examples_input_superscope

## Stage 3 — "Can I build a world?"

*Confidence: bigger than one screen.* Scroll a background past the camera,
stream a world larger than VRAM, drive it from a Tiled map, and stand on the
ground with tile collision.

@subpage examples_scrolling_mixed_scroll

@subpage examples_scrolling_continuous_scroll

@subpage examples_maps_map_scroll

@subpage examples_maps_tiled

@subpage examples_maps_slope_collision

@subpage examples_basics_collision_demo

## Stage 4 — "Can I make it feel good?"

*Confidence: it feels like a real game.* This is where the SNES becomes the
SNES — fades and mosaic transitions, per-scanline HDMA effects, colour math,
windows, the Mode 7 plane, and sound.

@subpage examples_transitions_fading

@subpage examples_transitions_mosaic

@subpage examples_hdma_hdma_wave

@subpage examples_hdma_gradient_colors

@subpage examples_color_palette_cycle

@subpage examples_color_transparency

@subpage examples_color_shadow_tint

@subpage examples_windows_window

@subpage examples_mode7_rotate_scale

@subpage examples_mode7_perspective

@subpage examples_audio_snesmod_music

@subpage examples_audio_snesmod_sfx

@subpage examples_audio_soundboard

## Stage 5 — "Can I hold it all together?"

*Confidence: architecture, not spaghetti.* Stop hand-rolling the main loop
and the state machine; assemble a skeleton, and pick up the reusable maths
every game needs.

@subpage examples_basics_timer

@subpage examples_basics_scene_stack

@subpage examples_basics_panel_hud

@subpage examples_basics_game_skeleton

@subpage examples_basics_aim_target

@subpage examples_basics_fix32_orbit

@subpage examples_basics_random

## Stage 6 — "Can I finish and ship?"

*Confidence: a complete cartridge.* Persist progress, choose a mapper, reach
for a coprocessor when you need more, and study complete games that fuse
everything above.

@subpage examples_memory_save_game

@subpage examples_memory_hirom_demo

@subpage examples_games_breakout

@subpage examples_games_likemario

@subpage examples_games_mapandobjects

@subpage examples_games_rpg

@subpage examples_games_shmup_1942

@subpage examples_games_mode7_racing

@subpage examples_games_mode7_flying

### More horsepower — the cartridge coprocessors

When the base hardware isn't enough. SA-1 is the same 65816 ISA at 10.74 MHz;
SuperFX / GSU is a custom RISC processor for bitmaps and 3D.

@subpage examples_chips_sa1_hello

@subpage examples_chips_sa1_starfield

@subpage examples_chips_superfx_hello

@subpage examples_chips_superfx_3d
