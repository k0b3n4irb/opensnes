# Learn SNES Development {#learning_path}

A curated path through the examples — not a feature tour, a **developer's
journey**. You don't wake up wanting "Mode 7"; you wake up with a question
about the game you're building, and it changes as you go. Each stage below is
one of those questions, and the confidence it buys you. This is a hand-picked
subset; for the exhaustive index see @ref examples_by_category.

Two companions run alongside this journey: @ref craft for the *design* decisions
behind each step — what to build, and how to budget it on the hardware — and
@ref tools for turning your own art, maps and audio into SNES data.

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

@subpage examples_text_pixel_text

@subpage examples_backgrounds_mode1

@subpage examples_sprites_simple_sprite

@subpage examples_sprites_animated_sprite

@subpage examples_sprites_dynamic_sprite

@subpage examples_sprites_sprite_swarm

> Making it *your* art: @ref tools_opensnes_tileset turns a PNG into the tiles, palette
> and tilemap this stage builds on, and @ref craft_planning budgets the VRAM
> before you draw a pixel.

> Under the hood — how a glyph becomes pixels with no module at all:
> @subpage examples_fundamentals_text_glyphs

## Stage 2 — "Can the player act?"

*Confidence: it's a game, not a demo.* Read the pad, move something with it,
then branch out to the SNES's exotic controllers.

@subpage examples_input_controller

@subpage examples_input_mouse

@subpage examples_input_superscope

Now that the pad moves things, the sprites that answer it: the OBJ size pairs
one character is built from (and, behind SELECT in @ref examples_sprites_dynamic_sprite,
a metasprite streamed under each pair), a world larger than the screen with its sprites
in world coordinates, and an Aseprite timeline played on a button.

@subpage examples_sprites_sprite_sizes

@subpage examples_sprites_world_sprites

@subpage examples_sprites_aseprite_pipeline

## Stage 3 — "Can I build a world?"

*Confidence: bigger than one screen.* Scroll a background past the camera,
stream a world larger than VRAM, drive it from a Tiled map, and stand on the
ground with tile collision.

@subpage examples_scrolling_mixed_scroll

@subpage examples_maps_map_scroll

@subpage examples_scrolling_continuous_scroll

@subpage examples_basics_collision_demo

> Author the world, don't hand-code it: @ref craft_tiles_to_levels is the
> think-in-metatiles workflow, and @ref tools_opensnes_level converts a Tiled level
> into the map binaries `map_scroll` loads.

## Stage 4 — "Can I make it feel good?"

*Confidence: it feels like a real game.* This is where the SNES becomes the
SNES — fades and mosaic transitions, per-scanline HDMA effects, colour math,
windows, the Mode 7 plane, and sound.

@subpage examples_transitions_fading

@subpage examples_hdma_hdma_wave

@subpage examples_scrolling_parallax_scroll

@subpage examples_hdma_gradient_colors

@subpage examples_color_palette_cycle

@subpage examples_color_transparency

@subpage examples_color_shadow_tint

@subpage examples_windows_window

@subpage examples_mode7_rotate_scale

@subpage examples_mode7_perspective

@subpage examples_mode7_perspective_rotate

@subpage examples_audio_snesmod_music

@subpage examples_audio_sfx_from_wav

@subpage examples_audio_soundboard

## Stage 5 — "Can I hold it all together?"

*Confidence: architecture, not spaghetti.* Stop hand-rolling the main loop
and the state machine; assemble a skeleton, and pick up the reusable maths
every game needs.

@subpage examples_basics_scene_stack

@subpage examples_basics_panel_hud

@subpage examples_basics_game_skeleton

@subpage examples_basics_aim_target

## Stage 6 — "Can I finish and ship?"

*Confidence: a complete cartridge.* Persist progress, choose a mapper, fit
more data in the cartridge with compression, reach for a coprocessor when you need more, study playable demos that fuse
everything above, and two complete games.

@subpage examples_memory_save_game

@subpage examples_memory_hirom_demo

@subpage examples_games_breakout

@subpage examples_games_blocks

@subpage examples_demos_likemario

@subpage examples_demos_mapandobjects

@subpage examples_demos_rpg

@subpage examples_demos_shmup_1942

@subpage examples_mode7_mode7_racing

### More horsepower — the cartridge coprocessors

When the base hardware isn't enough. SA-1 is the same 65816 ISA at 10.74 MHz;
Super FX / GSU is a custom RISC processor for bitmaps and 3D.

@subpage examples_chips_sa1_starfield

@subpage examples_chips_superfx_3d

@subpage examples_chips_superfx_game_skeleton

Saving on a coprocessor cartridge uses the same `sram` calls as any other:
@ref tutorial_sa1 shows it in BW-RAM, @ref tutorial_superfx_advanced in the
Game Pak RAM while a job may run.

## Further — the rest of the hardware

Every example not on the path above, by topic. Each stands on what the path
taught; read the one your game needs.

- **The other background modes** — @ref examples_backgrounds_mode0 (four 4-colour
  layers), @ref examples_backgrounds_mode2 (offset-per-tile in Modes 2 and 4),
  @ref examples_backgrounds_mode3
  (256 colours), @ref examples_backgrounds_mode5_hires (512 pixels, interlace).
- **More colour** — @ref examples_color_direct_color, @ref examples_color_hicolor_1792.
- **More HDMA and windows** — @ref examples_hdma_hdma_helpers,
  @ref examples_windows_window_multi_hdma.
- **More Mode 7** — @ref examples_mode7_extbg (a second layer from the same
  plane), @ref examples_mode7_dsp1_ground (the DSP-1's projection).
- **More sound** — @ref examples_audio_apu_switch,
  @ref examples_audio_speech_synth.
- **More chips** — the DSP-1 is in Mode 7: @ref examples_mode7_dsp1_ground.
