# SuperFX Hello

> GSU coprocessor boot diagnostic with SRAM tests and FMULT validation

![Screenshot](screenshot.png)

## Emulator Compatibility

| Emulator | Status |
|----------|--------|
| **Mesen2** | ✅ Recommended — detects the GSU correctly and runs SuperFX cleanly |
| **bsnes** | ✅ Cycle-accurate; useful as a second reference |
| **luna** | ✅ Detects and runs the GSU natively (used by the test harness) |
| **snes9x** | ❌ Does not detect GSU despite correct header — example shows "GSU: NOT DETECTED" |

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/memory/superfx_hello
```

Then open `superfx_hello.sfc` in Mesen2 (recommended) or bsnes.

## What You'll Learn

- Detecting the SuperFX (GSU) coprocessor via the VCR register ($303B)
- Launching a GSU program with the WRAM stub (CPU cannot read ROM while GSU runs)
- SRAM shared memory: byte writes (STB: $42, $55) and word write (STW: $BEEF)
- FMULT fixed-point multiplication validation (2.0×2.0=$4000, 1.5×3.0=$4800)
- Reading GSU registers (R0=$CAFE) after STOP

## Status Codes

| Value | Meaning |
|-------|---------|
| `GSU VERSION: $04` | GSU-2 detected (GSU-1=$03, MC1=$01) |
| `R0 RESULT: $CAFE` | GSU executed code correctly |
| `SRAM[0,1]: $42 $55` | STB byte writes to SRAM work |
| `STW[2,3]: $BEEF` | STW word write to SRAM works |
| `2*2=$4000 1.5*3=$4800` | FMULT 4.12 fixed-point validated |

## Modules Used

| Module | Purpose |
|--------|---------|
| `console` | System initialization |
| `sprite` | OAM setup (required by consoleInit) |
| `dma` | DMA transfers |
| `background` | BG configuration |
| `text` | Diagnostic text display |
| `superfx` | GSU detection and communication |
