# Sprites

**"things that move independently of the
background."** Sprites (OBJ) are the heart of any game: the player, enemies,
bullets, pickups. The SNES gives you 128 hardware sprites; this family is a
ladder from one static sprite up to streaming a multi-tile character every
frame.

## The ladder

| Example | Developer question |
|---------|--------------------|
| [simple_sprite](simple_sprite/) | How do I put one sprite on screen? |
| [sprite_sizes](sprite_sizes/) | How do I use the six OBJ size modes (8×8…64×64)? |
| [animated_sprite](animated_sprite/) | How do I frame-animate a sprite (+H-flip)? |
| [dynamic_sprite](dynamic_sprite/) | How do I stream sprite tiles into VRAM per frame — single sprites, and (SELECT) a *multi-tile* character? |
| [sprite_swarm](sprite_swarm/) | How many sprites can I actually move at 60 fps, and why? |
| [world_sprites](world_sprites/) | How do I place sprites in world coordinates under a camera (culling, depth sort, one call per batch)? |
| [aseprite_pipeline](aseprite_pipeline/) | How do I animate a metasprite from an Aseprite export? |

> 3.7 is the showcase *and* a reality check: a smooth 32-sprite swarm plus
> the honest per-sprite budget (the base CPU tops out around three dozen in
> C — see [chips/sa1_starfield](../chips/sa1_starfield/) for 128 via the SA-1).

Start at the top and climb: each example adds one idea onto the last.

## Reference — OAM in one screen

**OAM structure** — 128 sprites, 4 bytes each in the low table (512 bytes):
X (low 8), Y, tile number (low 8), attributes (`vhoopppc` — flip, priority,
palette, tile high bit). The high table (32 bytes) holds 2 bits/sprite: X high
bit and size select.

**Sizes (OBJSEL `$2101`)** — each value picks a *small*/*large* pair, e.g.
`1` = 8×8 / 32×32, `5` = 32×32 / 64×64. `sprite_sizes` walks all six.

**Many sprites per frame** — write straight to the `oamMemory[]` buffer rather
than calling `oamSet()` repeatedly; the NMI handler DMAs the buffer to OAM
hardware when `oam_update_flag` is set. `dynamic_sprite` builds on this.
