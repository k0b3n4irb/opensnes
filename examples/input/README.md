# Input Examples

The SNES supports standard controllers (12 buttons + D-pad), the SNES Mouse, and the
Super Scope light gun. The console reads controller state automatically every frame
via auto-joypad read and stores the result in hardware registers.

The key skill is **edge detection** -- distinguishing "button is held" from "button was
just pressed this frame." Without it, a single tap registers dozens of times at 60fps.

## Examples

| Example | Difficulty | Description |
|---------|------------|-------------|
| [two_players](two_players/) | Beginner | Reading both controllers independently |
| [mouse](mouse/) | Intermediate | Mouse detection, cursor movement, button handling, sensitivity |
| [superscope](superscope/) | Advanced | Light gun detection, calibration, PPU H/V counter targeting |

## Key Concepts

### Joypad Bit Layout

```
Bit: 15  14  13  12  11  10  9   8   7   6   5   4   3-0
     B   Y   Sel Sta Up  Dn  Lt  Rt  A   X   L   R   (ID)
```

### Edge Detection Pattern

```c
u16 held = padsCurrent(0);
u16 pressed = padsDown(0);   // newly pressed this frame only
```

### Input Devices

| Device | Port | Detection |
|--------|------|-----------|
| Joypad | 1-2 | Always present (default) |
| Mouse | 1-2 | Check device ID bits |
| Super Scope | Port 2 only | Check device ID bits, requires CRT/emulator support |

---

Start with **two_players** for basic joypad input, then explore **mouse** and
**superscope** for alternate input devices.
