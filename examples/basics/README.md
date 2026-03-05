# Basics Examples

Core game mechanics that go beyond pure graphics or audio. These examples demonstrate
fundamental patterns -- collision detection, movement, and interactive logic -- that
you will reuse in every game project.

## Examples

| Example | Difficulty | Description |
|---------|------------|-------------|
| [collision_demo](collision_demo/) | Intermediate | Bounding-box collision detection between multiple sprites |

## Key Concepts

- **Bounding-box collision**: compare rectangle edges to detect overlaps
- The SNES has no hardware collision detection -- it must be done in software
- Efficient collision checks matter because the 65816 CPU is slow at 32-bit math

---

After completing the graphics and input examples, study **collision_demo** to see
how sprites, input, and math come together into interactive gameplay.
