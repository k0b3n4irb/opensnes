# SuperFX Hello

> GSU coprocessor boot diagnostic -- detects SuperFX, launches a minimal GSU program, and verifies bidirectional CPU/GSU communication.

![Screenshot](screenshot.png)

## Emulator Compatibility

> **Important**: SuperFX (GSU) examples require a SuperFX-capable emulator.

| Emulator | Status |
|----------|--------|
| **bsnes** | Recommended -- cycle-accurate SuperFX emulation |
| **Mesen2** | Works for boot/registers, but has a bug with GSU backward branches |
| **snes9x** | Does not detect SuperFX in our ROM header |

The screenshot above was captured with opensnes-emu (snes9x-based), which shows
"GSU: NOT DETECTED". This is expected -- the diagnostic messages and test results
are only visible on a SuperFX-capable emulator.

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/memory/superfx_hello
```

Then open `superfx_hello.sfc` in bsnes or Mesen2.

## What You'll Learn

- How to detect the SuperFX (GSU) coprocessor at runtime via `gsuInit()`
- Reading the GSU version register (VCR at $303B)
- Launching a GSU program and polling for completion
- Bidirectional communication: SNES CPU writes R0, GSU writes R0 back
- SRAM readback: GSU writes to SRAM, SNES CPU reads back to verify
- WRAM stub pattern: polling GSU status from WRAM while ROM bus is owned by GSU

## How It Works

1. `gsuInit()` checks VCR ($303B) for a valid SuperFX chip version
2. The GSU program (`gsu_hello.sfx`) sets R0 = `$CAFE`, writes `$42` and `$55` to SRAM bytes 0-1, and writes `$BEEF` to SRAM bytes 2-3 via STW
3. `launchGSU()` copies a stub to WRAM, configures SCMR (bus ownership), sets PBR/R15 to start the GSU, polls SFR until done, then reclaims the buses
4. The SNES CPU reads back R0 and SRAM values, displaying them on screen
5. "ALL TESTS PASSED!" confirms full bidirectional communication

## Status Codes

| Value | Meaning |
|-------|---------|
| R0 = `$CAFE` | GSU executed the program correctly |
| SRAM[0] = `$42` | GSU byte write to SRAM confirmed |
| SRAM[1] = `$55` | Second GSU byte write confirmed |
| STW[2,3] = `$BEEF` | GSU word write (STW instruction) confirmed |
| "NOT DETECTED" | No SuperFX hardware found -- use a capable emulator |

## Project Structure

| File | Purpose |
|------|---------|
| `main.c` | Detection logic, result display, hex formatting |
| `gsu_loader.asm` | WRAM stub, GSU launch sequence, SRAM readback |
| `gsu_hello.sfx` | Minimal GSU program (R0 = $CAFE, SRAM writes) |
| `Makefile` | `USE_SUPERFX := 1`, `GSUSRC := gsu_hello.sfx` |

## Modules Used

| Module | Purpose |
|--------|---------|
| console | System initialization |
| sprite | OAM management |
| dma | DMA transfers |
| background | BG configuration |
| text | Diagnostic text display |
| superfx | GSU coprocessor driver (`gsuInit()`) |
