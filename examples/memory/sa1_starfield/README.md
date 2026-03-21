# SA-1 Starfield (Murmuration)

> 128 dots in Lissajous sine patterns computed by SA-1 at 10.74 MHz

![Screenshot](screenshot.png)

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/memory/sa1_starfield
```

Then open `sa1_starfield.sfc` in your emulator (Mesen2 recommended).

## What You'll Learn

- SA-1 computing 128 sprite positions per frame using sine harmonics
- I-RAM as a shared buffer: SA-1 writes positions, SNES CPU reads them
- Procedural sprite generation (no graphic assets needed)
- Depth illusion via 4 brightness palettes cycling across sprites
- Synchronization protocol between SA-1 and main CPU

## What to Observe

- 128 dots moving in smooth, coordinated flock-like patterns
- The pattern resembles a murmuration (starling flock)
- 4 brightness levels create a subtle depth effect
- All math computed by SA-1 coprocessor at 10.74 MHz

## Modules Used

| Module | Purpose |
|--------|---------|
| console | System initialization |
| sprite | OAM management |
| dma | DMA transfers |
| background | BG configuration |
| input | Joypad reading |
| sa1 | SA-1 coprocessor driver |
