# Complete Games

Three games with everything a game has: a title screen, the rules, a score,
lives or a stack that fills, a game over and a way back to the start. They
combine sprites, backgrounds, input, collision, a HUD, sound and a state
machine. Study them after the topic examples and the [demos](../demos/).

## Example ROMs

| Example | Difficulty | Description |
|---------|------------|-------------|
| [breakout](breakout/) | Intermediate | Breakout: paddle, ball, destructible bricks, levels, score and lives |
| [blocks](blocks/) | Intermediate | Falling-blocks puzzle with three BG layers, delayed auto-shift input and SNESMOD music |
| [likemario](likemario/) | Advanced | Kit's Rooftop Run: a scrolling platformer with shards to pick up, machines to stomp, three lives, checkpoints and a beacon to reach |

## What These Demonstrate

- Game state machines (title, playing, paused, game over)
- Score tracking and HUD display
- Collision detection (sprite-sprite and sprite-tile)
- Input handling for responsive controls
- Music and sound effects tied to the game's events

---

Start with **breakout** for a self-contained game loop, then **blocks** for
three layers, timed input and music, then **likemario** for a world bigger
than the screen.
