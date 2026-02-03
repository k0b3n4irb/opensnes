# HDMA Wave Effect

Demonstrates animated wave distortion using HDMA per-scanline horizontal scroll manipulation.

## Effect Uses

- **Water reflections** - Wavy distortion for lake/ocean surfaces
- **Heat shimmer** - Desert or fire heat effects
- **Dream sequences** - Surreal visual distortion

## Controls

| Button | Action |
|--------|--------|
| A | Toggle wave effect on/off |
| Left/Right | Adjust amplitude (1-8 pixels) |
| Up/Down | Adjust frequency (1-8) |
| L/R | Adjust animation speed (1-4) |

## How It Works

The HDMA system writes different horizontal scroll values to BG1HOFS on each scanline, creating the wave pattern. A sine table provides smooth wave motion, and the animation updates the phase offset each frame.

## API Functions

```c
hdmaWaveInit();                         // Initialize wave system
hdmaWaveH(channel, bg, amplitude, freq); // Setup horizontal wave
hdmaWaveSetSpeed(speed);                 // Animation speed (1-4)
hdmaWaveUpdate();                        // Call each frame to animate
hdmaWaveStop();                          // Disable wave effect
```

## Technical Notes

- Uses HDMA channel 6 (channels 0-5 often used for audio DMA)
- Wave table is 224 entries (one per visible scanline)
- Higher frequency = more wave cycles on screen
- Higher amplitude = more horizontal displacement
