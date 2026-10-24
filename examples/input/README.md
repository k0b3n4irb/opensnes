# Input & peripherals

**"can the player act?"** This is the step that turns a demo into a
game. Read the pad, move something with it, then branch out to the SNES's
exotic controllers. The console auto-reads every input device each VBlank; you
just read the result.

## The ladder

| Example | Developer question |
|---------|--------------------|
| [controller](controller/) | How do I read the joypads — both pads, held, just pressed, just released? |
| [mouse](mouse/) | How do I read the SNES Mouse? |
| [superscope](superscope/) | How do I read the Super Scope light gun? |

Driving a sprite with the pad is the `starter/` project at the SDK root —
the same program, with the asset pipeline of 1.x.

## The idea in one screen

`padHeld(0)` returns a 16-bit bitmask of the buttons held this frame;
`padPressed(0)` returns only those *newly* pressed (edge detection — without
it, a single tap registers dozens of times at 60 Hz). Act on the bits, and for
movement push the result to a sprite with `oamSetXY`.

```
Bit: 15  14  13   12   11  10  9   8   7  6  5  4
     B   Y   Sel  Sta  Up  Dn  Lt  Rt  A  X  L  R
```

The **mouse** and **Super Scope** are read the same way after a device-ID
check; the Super Scope lives on port 2 and latches its aim from the PPU's H/V
counters.

> Start with `controller` to read the pad and feel the screen respond to it —
> that's the moment it becomes a game.
