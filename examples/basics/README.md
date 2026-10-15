# Basics Examples

Core game mechanics that go beyond pure graphics or audio. These examples demonstrate
fundamental patterns -- collision detection, movement, and interactive logic -- that
you will reuse in every game project.

## Example ROMs

| Example | Difficulty | Description |
|---------|------------|-------------|
| [collision_demo](collision_demo/) | Intermediate | Bounding-box collision detection between multiple sprites |
| [panel_hud](panel_hud/) | Intermediate | 9-slice HUD + dialog box on one BG layer (the `panel` module) |
| [game_skeleton](game_skeleton/) | Intermediate | The smallest complete game: title → play → game-over state machine |
| [timer](timer/) | Beginner | A frame counter on the gameloop framework |
| [scene_stack](scene_stack/) | Intermediate | Scenes on a stack: title → counter → a pause overlay |
| [fix32_orbit](fix32_orbit/) | Beginner | A sprite orbiting in 16.16 fixed point (`fixed32`) |
| [aim_target](aim_target/) | Intermediate | Aim, distance and angle with `sqrt16` and `atan2_8`, 8.8 and 16.16 side by side |

## Key Concepts

- **Bounding-box collision**: compare rectangle edges to detect overlaps
- The SNES has no hardware collision detection -- it must be done in software
- Efficient collision checks matter because the 65816 CPU is slow at 32-bit math

---

After completing the graphics and input examples, study **collision_demo** to see
how sprites, input, and math come together into interactive gameplay.
