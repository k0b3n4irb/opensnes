# Sprites

**Family 3 of the developer journey — "things that move independently of the
background."** Sprites (OBJ) are the heart of any game: the player, enemies,
bullets, pickups. The SNES gives you 128 hardware sprites; this family is a
ladder from one static sprite up to streaming a multi-tile character every
frame.

## The ladder

| Rung | Example | Developer question |
|------|---------|--------------------|
| 3.1 | [simple_sprite](simple_sprite/) | How do I put one sprite on screen? |
| 3.2 | [sprite_sizes](sprite_sizes/) | How do I use the six OBJ size modes (8×8…64×64)? |
| 3.3 | [animated_sprite](animated_sprite/) | How do I frame-animate a sprite (+H-flip)? |
| 3.4 | [metasprite](metasprite/) | How do I compose one character from many tiles? |
| 3.5 | [dynamic_sprite](dynamic_sprite/) | How do I stream sprite tiles into VRAM per frame? |
| 3.6 | [dynamic_metasprite](dynamic_metasprite/) | How do I stream a *multi-tile* character (dynamic + meta)? |
| 3.7 | [sprite_swarm](sprite_swarm/) | How many sprites can I actually move at 60 fps, and why? |

> 3.7 is the showcase *and* a reality check: a smooth 32-sprite swarm plus
> the honest per-sprite budget (the base CPU tops out around three dozen in
> C — see [chips/sa1_starfield](../chips/sa1_starfield/) for 128 via the SA-1).

Start at 3.1 and climb: each rung adds exactly one idea onto the last.

## Reference — OAM in one screen

**OAM structure** — 128 sprites, 4 bytes each in the low table (512 bytes):
X (low 8), Y, tile number (low 8), attributes (`vhoopppc` — flip, priority,
palette, tile high bit). The high table (32 bytes) holds 2 bits/sprite: X high
bit and size select.

**Sizes (OBJSEL `$2101`)** — each value picks a *small*/*large* pair, e.g.
`1` = 8×8 / 32×32, `5` = 32×32 / 64×64. Rung 3.2 walks all six.

**Many sprites per frame** — write straight to the `oamMemory[]` buffer rather
than calling `oamSet()` repeatedly; the NMI handler DMAs the buffer to OAM
hardware when `oam_update_flag` is set. Rungs 3.5–3.6 build on this.
