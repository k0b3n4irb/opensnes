# Audio Example: Sound Effects

A minimal sound effect demo using bare-metal SPC700 programming (like 1_tone).

## What This Example Does

- Uploads a minimal SPC driver (~100 bytes) and BRR sample to SPC RAM
- Plays the TADA sound when A button is pressed
- Visual feedback: GREEN screen = ready, BLUE flash = sound playing

**Controls:**
- A button: Play sound effect

---

## Code Type

**C + Bare-metal Assembly**

| Component | Type |
|-----------|------|
| Hardware init | Direct register access |
| Audio | Custom SPC driver (`spc_init`, `spc_play`) |
| Input | Direct register access |

---

## Architecture

```
┌─────────────────┐     ┌─────────────────┐
│  Main CPU       │     │  SPC700         │
│  (65816)        │     │  Audio CPU      │
├─────────────────┤     ├─────────────────┤
│ spc_init()      │────>│ Upload driver   │
│                 │     │ + sample as one │
│                 │     │ blob to $0200   │
├─────────────────┤     ├─────────────────┤
│ spc_play()      │────>│ Key-on voice 0  │
│                 │     │ via port0       │
└─────────────────┘     └─────────────────┘
        │                       │
        │   4 I/O ports         │
        │   ($2140-$2143)       │
        └───────────────────────┘
```

This uses the same approach as 1_tone - a simple custom SPC driver.

---

## SPC Memory Layout

```
$0200 - Driver code (~100 bytes)
$0300 - Sample directory (4 bytes per entry)
$0304 - BRR sample data
```

---

## Build and Run

```bash
cd examples/audio/2_sfx
make clean && make

# Run in emulator
/path/to/Mesen sfx_demo.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Main program with input handling |
| `spc.asm` | SPC driver, upload code, BRR sample |
| `tada.brr` | BRR audio sample |
| `Makefile` | Build configuration |

---

## See Also

- [1_tone](../1_tone/) - Similar bare-metal approach with text display
- [6_snesmod_music](../6_snesmod_music/) - Full music playback with SNESMOD

---

## License

MIT
