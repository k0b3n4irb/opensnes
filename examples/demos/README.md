# Demos

Playable demonstrations: each puts several modules together in one scene
you can move through, without a game's rules — no score, no end. They show
how the parts assemble; the complete games are in [games](../games/).

## Example ROMs

| Example | Difficulty | Description |
|---------|------------|-------------|
| [platform_objects](platform_objects/) | Advanced | The map engine and the object engine together: slopes, patrolling machines, streamed sprites |
| [shmup](shmup/) | Intermediate | A vertical shoot 'em up: scrolling, an enemy pool, bullets and collision |
| [rpg](rpg/) | Advanced | An RPG template: a Tiled-driven overworld with dialog boxes |

The Mode 7 race, `mode7_racing`, is with the other Mode 7 examples in
[mode7](../mode7/).

## What These Demonstrate

- Sprite management across many on-screen objects
- Scrolling camera systems
- Collision against a tile map and between sprites
- Level data written by tools (Tiled, the asset build) and read by the engine

---

Start with **shmup** for one screen of moving parts, then **platform_objects**
for a scrolling world run by the map and object engines.
