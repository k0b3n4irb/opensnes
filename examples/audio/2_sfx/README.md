# Audio Example: Sound Effects

A minimal sound effect demo using the OpenSNES audio library.

## What This Example Does

- Initializes the audio system
- Plays a sound effect when A button is pressed
- Visual feedback: screen color changes when playing

**Controls:**
- A button: Play sound effect

---

## Code Type

**C + Assembly Audio Library**

| Component | Type |
|-----------|------|
| Hardware init | Direct register access |
| Audio | Library (`audioInit`, `audioPlaySample`) |
| Input | Direct register access |

---

## Key Functions

```c
extern void audioInit(void);          /* Initialize audio system */
extern void audioPlaySample(u8 id);   /* Play sample by ID */
extern void audioSetVolume(u8 vol);   /* Set volume (0-127) */
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
| `main.c` | Main program |
| `data.asm` | Audio library and sample data |
| `Makefile` | Build configuration |

---

## See Also

- [1_tone](../1_tone/) - Bare-metal SPC700 programming
- [6_snesmod_music](../6_snesmod_music/) - Full music playback

---

## License

MIT
