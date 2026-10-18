# Complete Game Examples

Full game implementations that combine multiple SNES subsystems: sprites, backgrounds,
input, scrolling, collision detection, and game state management. Study these after
working through the individual topic examples.

## Example ROMs

| Example | Difficulty | Description |
|---------|------------|-------------|
| [breakout](breakout/) | Intermediate | Classic Breakout clone with paddle, ball, and destructible blocks |
| [likemario](likemario/) | Advanced | Side-scrolling platformer with scrolling camera and animation |
| [mapandobjects](mapandobjects/) | Advanced | Tile map world with interactive objects |
| [tetris](tetris/) | Intermediate | Tetris with three BG layers, delayed auto-shift input and SNESMOD music |
| [shmup_1942](shmup_1942/) | Intermediate | A vertical shoot 'em up: scrolling, an enemy pool, bullets and collision |
| [rpg](rpg/) | Advanced | An RPG template: a Tiled-driven overworld with dialog boxes |
| [mode7_racing](mode7_racing/) | Advanced | An F-Zero-style racer on the Mode 7 plane |
| [mode7_flying](mode7_flying/) | Advanced | A Pilotwings-style flight over the Mode 7 plane, with landings |

## What These Demonstrate

- Sprite management across many on-screen objects
- Input handling for responsive controls
- Collision detection (sprite-sprite and sprite-tile)
- Game state machines (title, playing, game over)
- Score tracking and HUD display
- Scrolling camera systems

---

Start with **breakout** for a self-contained game loop, then study **likemario**
for scrolling platformer techniques.
