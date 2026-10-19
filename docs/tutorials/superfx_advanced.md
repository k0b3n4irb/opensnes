# Super FX: Running a Game Around the GSU {#tutorial_superfx_advanced}

This page follows @ref tutorial_superfx and assumes a program that builds,
launches and returns a result, and covers what changes once the game has to
keep running while the GSU works.

## The Exclusive Bus

When SCMR has RON=1, the GSU owns the ROM bus. **The SNES CPU cannot
read ROM -- not even its own code or its own interrupt vectors.** The
hardware plans for it: while the GSU owns the ROM, a vector fetch returns a
dummy vector — `$0108` for NMI, `$010C` for IRQ, `$0104` COP, `$0100`
BRK/ABORT (Nintendo dev manual Book II §5.4.1) — and a Super FX cartridge
keeps a jump at each of those WRAM addresses. The SDK does this for you in
every `USE_SUPERFX=1` build:

1. crt0 installs the four WRAM stubs at boot, and the header's vectors
   point at them, so an interrupt takes the same path whether the GSU is
   running or not;
2. the NMI stub enters a small WRAM handler: while a GSU job runs
   (`gsuLaunch` sets `gsu_owns_cart`) it acknowledges the NMI, advances
   `frame_count` and uploads OAM if you flagged it; the rest of the VBlank
   work — tilemap, scroll, pads, your NMI callback — is ROM code and is
   **deferred** to the next VBlank after the job, not lost;
3. the IRQ stub enters a WRAM handler too: during a job it acknowledges an
   H/V-timer IRQ (`$4211`) and a GSU IRQ (SFR high byte) and returns — your
   IRQ handler is ROM code and runs again after the job; BRK and COP land on
   a WRAM `rti`;
4. the launch/poll code itself still executes from WRAM, as in every
   reference project (casfx, DOOM-FX, PeterLemon).

The WRAM handlers and `gsuLaunch`'s wait loop live in the RAM code window
(below): a Super FX build reserves 768 bytes of it for the SDK on top of
your own `RAM_CODE_SIZE`. crt0 installs the four stubs before it enables the
NMI.

The NMI is **not** disabled during a job, so no VBlank is lost while the GSU
runs, and an H/V-timer IRQ the game has armed stays armed.

## Letting the Game Run During a Job: the Code Cache

`gsuLaunch()` waits in WRAM for the whole job, because a GSU running its
program from ROM owns the ROM. A program that fits in the GSU's 512-byte
code cache does not need the ROM: loaded there by the CPU and started from
the cache, the GSU keeps running with RON = 0, and the CPU carries on with
its own code in ROM (Nintendo dev manual Book II §6.1.2: a program in the
cache "will not stop" when RON is 0, "it becomes possible to access the game
pak ROM or RAM from the Super NES CPU"; §6.8.4 for loading the cache from
the CPU).

```c
extern const u8 gsu_job[], gsu_job_end[];      /* the .incbin'd .sfx.bin */

gsuCacheLoad(gsu_job, (u16)(gsu_job_end - gsu_job));
gsuStartCached(0);              /* returns at once */
while (gsuBusy()) {
    WaitForVBlank();            /* the game's frame, from ROM */
    update_game();
}
gsuWait();                      /* gives Game Pak RAM back to the CPU */
```

`gsuCacheLoad()` stops the GSU (which empties the cache), copies the code to
`$3100` and pads the last 16-byte line with NOP, as krom's cache-injection
test does. `gsuStartCached()` writes your `gsu_cfgr` / `gsu_scbr` /
`gsu_scmr`, with RON forced to 0, then R15, which starts the GSU.

What such a program may not do:

- be longer than 512 bytes;
- use `CACHE` or `LJMP`: both empty the cache and refetch from ROM, which
  the GSU does not own;
- read ROM data (`GETB` and the ROM buffer).

If `gsu_scmr` grants the Game Pak RAM to the GSU (RAN, needed for `PLOT` and
`STW`), the CPU must not touch `$70:xxxx`, nor its `$6000-$7FFF` mirror,
until `gsuWait()`. `testing/fixtures/libtests_gsu` runs a job of about seven frames
this way; its manifest checks that the game loop counted seven frames during
the job and that the CPU never read the ROM the GSU owned (luna's
`gsu.bus_violations` stays 0, against 149 340 when RON is left at 1).

### Being told when it ends: the IRQ on STOP

Instead of asking `gsuBusy()`, the game can let the GSU tell it. With
CFGR bit 7 clear, STOP raises an IRQ on the SNES CPU; bit 15 of the GSU
status register (SFR) says the GSU was its source, and reading that byte
resets it (Nintendo dev manual Book II §5.4.2, which describes the IRQ as the
way to "continue its own processing without having to periodically monitor
the GSU"; fullsnes, CFGR: "0=Trigger IRQ on STOP opcode, 1=Disable IRQ").

The Super FX IRQ entry in crt0 does that test first. A GSU IRQ is
acknowledged and counted in `gsu_stop_irqs`, and never reaches your
`irqSet()` handler, which stays the H/V timer's. When the timer fired too,
its IRQ comes straight back after the GSU's, for your handler.

```c
irqEnable(IRQ_VTIMER);          /* or anything that leaves the I flag clear */
gsu_cfgr = 0x00;                /* CFGR_IRQ_MASK clear: IRQ on STOP */
u8 stops = gsu_stop_irqs;
gsuCacheLoad(gsu_job, (u16)(gsu_job_end - gsu_job));
gsuStartCached(0);
while (gsu_stop_irqs == stops) {
    WaitForVBlank();
    update_game();
}
gsuWait();
```

The CPU takes no IRQ at all while its I flag is set, which is how crt0 boots:
the count moves only once something has cleared it (`irqEnable()` does). Keep
`CFGR_IRQ_MASK` set when you poll: whether SFR bit 15 is also set by a STOP
whose IRQ is masked is an open question in fullsnes ("also set if IRQ
masked?"); ares and bsnes set it only when the IRQ is unmasked
(`instructionSTOP`: `if(regs.cfgr.irq == 0) { regs.sfr.irq = 1; … }`), an
emulator's choice rather than a measurement. If a console did set it, the
next timer IRQ would count that old STOP.
`testing/fixtures/libtests_gsu` runs its job a second time this way (the count moves
by exactly one, seven game frames during the job).

### A ROM job, with the wait loop in RAM

A program too big for the cache, or one that reads ROM data, runs with
RON = 1, and then the CPU cannot fetch a single instruction from the ROM
(Nintendo dev manual Book II §5.3). `gsuLaunch()` waits from the RAM code
window, which a Super FX build always has (768 bytes for the SDK). For your
own code, ask the build for more room and write the loop as ordinary
assembly:

```makefile
RAM_CODE_SIZE := 256        # bytes at the top of WRAM bank $7E
```

```asm
RAM_CODE_SECTION "rom_job"  ; labels resolve to $7E:xxxx
ramRunRomJob:
    ; ... start the GSU with RON = 1 and gsu_owns_cart = 1 ...
@wait:
    jsr count_something     ; a call inside the window
    lda.l $3030
    and #$20                ; SFR GO
    bne @wait
    lda #$00
    sta.l $303A             ; ROM and RAM back to the CPU
    sta.l gsu_owns_cart
    rtl
.ENDS
```

The window is stored in the top `RAM_CODE_SIZE` bytes of ROM bank 1 and
linked at the same 16-bit address in bank `$7E`, so jumps, calls and labels
inside it are the RAM ones; crt0 copies it at boot. C calls it like any
function (`ramRunRomJob();` is a `jsl` to `$7E:xxxx`). While the GSU owns the
ROM, code in the window must not call or read the ROM: no lib function, no
C code, no const data. The NMI keeps counting frames (`gsu_owns_cart`, see
above). `testing/fixtures/libtests_gsu` runs its job a third time this way: 33 frames
of a ROM-resident job with the CPU polling from RAM and no bus violation; the
same loop in a plain ROM section loses the CPU at once.

The loop can be C. `RAM_CODE` on a function puts it in the same window:

```c
RAM_CODE static void wait_rom_job(void) {
    u16 polls = 0;
    gsu_owns_cart = 1;
    REG_CFGR = gsu_cfgr;
    REG_CLSR = 1;
    REG_SCBR = gsu_scbr;
    REG_SCMR = (u8)(gsu_scmr | 0x18);   /* RON + RAN */
    REG_PBR = gsu_prog_bank;
    REG_GSU_R15 = gsu_prog_addr;        /* the GSU starts */
    while (REG_SFR_L & SFR_GO) {
        polls++;
    }
    REG_SCMR = 0;
    gsu_owns_cart = 0;
}
```

What the compiler does not check is the same as in assembly: while the GSU
owns the ROM, nothing in such a function may reach the ROM. In C that
excludes more than it looks: any call that is not itself `RAM_CODE` (library
functions, and inline ones the compiler chose not to inline), the runtime
helpers a multiplication, a division or a 32-bit shift calls, switch tables
and const data. Register writes, bank-0 variables and `FAR` variables are
fine. luna's `gsu.bus_violations` tells you at once when one slipped through:
the fixture's fourth job asserts 0, and the same function without `RAM_CODE`
counts 49 585.


## Several Jobs in One Program: Entry Points

The build links every `.sfx` of `GSUSRC` into a binary and writes a header
next to it: one `#define` per global label, its offset in the binary.
`gsu_job.sfx` with the labels `gsu_start` and `mul_job` gives
`gsu_job.sfx.h`:

```c
#define GSU_JOB_GSU_START 0x0000u
#define GSU_JOB_MUL_JOB 0x0028u
```

Labels starting with `_` or `@` are local and left out. Pass an entry point
to `gsuCall()` (from ROM, like `gsuLaunch()`, which is entry 0) or to
`gsuStartCached()` (from the cache), and the arguments in the GSU registers
before the start:

```c
#include "gsu_job.sfx.h"

REG_GSU_R1 = 300;
REG_GSU_R2 = 7;
gsuCall(GSU_JOB_MUL_JOB);       /* R0-R7 and R9-R13 are yours; the launcher writes R8 and R15 */
```

`testing/fixtures/libtests_gsu` runs `mul_job` both ways. The offsets are relative to
the binary. By default the program is assembled at 0 and placed by the
linker where it fits, so only position-independent code is right: relative
branches, `LOOP` through `MOVE R13, R15`.

### Absolute jumps and tables in ROM: `GSU_BANK`

A program that jumps to an absolute address or reads a table of its own
needs its labels to be the addresses the GSU sees. One line in the Makefile
does it:

```makefile
GSU_BANK := 1        # the program is linked at $01:8000
```

The `.sfx` is then assembled at `$8000` and `GSU_SECTION` forces the binary
to the start of ROM bank 1, which the GSU reads at the same address (it
sees banks `$00-$3F` as LoROM, ROM at `$8000-$FFFF`). `GSU_BANK` is also
defined inside the `.sfx`:

```asm
rom_job:
    IBT R0, #GSU_BANK
    ROMB                    ; ROM bank for GETB = the program's bank
    IWT R14, #_table        ; R14 = ROM address: the ROM buffer loads
    GETB                    ; R0 = table[0]
    IWT R15, #_far          ; absolute jump (delay slot follows)
    NOP
_table:
    .db $A5, $3C
_far:
    ...
```

The generated `.sfx.h` still holds offsets, so `gsuCall()` and
`gsuStartCached()` are called the same way. One `.sfx` per ROM in this mode
(`.include` the others from it); bank 1's top is the RAM code window, so a
program longer than what is left below it fails at link rather than at
run. `testing/fixtures/libtests_gsu` is built this way: its `rom_job` returns the
two table bytes, and returns 0 when the same binary is placed elsewhere.

## Presenting Frames: a Double Buffer the NMI Moves

`gsuDmaFullFrame()` makes every frame a sequence: the job, then a wait for
bottom band, then a 16 KB DMA that halts the CPU. The GSU is idle while the
frame moves, and the CPU can do nothing but wait. `gsuPresent()` overlaps
the three instead:

- two framebuffers in Game Pak RAM (`gsu_scbr` and the one after it), two
  char blocks in VRAM, one tilemap for both;
- the job draws in one buffer while the NMI moves the previous frame to the
  back VRAM block, a piece per VBlank, taking Game Pak RAM from the GSU for
  each piece: the CPU writes SCMR's RAN to 0, the GSU waits on its next RAM
  access, and resumes when RAN is back (Nintendo dev manual Book II §5.3);
- BG1's char base (BG12NBA) swaps to the back block only once the whole
  frame is in it, so a half-landed frame is never shown.

```c
gsuSetupBitmapTilemap(0x4000);
gsu_scbr = 0x00;                        /* buffer A at $70:0000, B at $70:4000 */
gsuPresentInit(0x0000, 0x2000, 0);      /* two 16 KB char blocks, A shown first */
/* ... */
gsuSetupHdmaBlanking(40, 40);
setScreenOn();
while (1) {
    if (!gsuBusy() && !gsuPresentBusy()) {
        gsuWait();                      /* Game Pak RAM back to the CPU */
        gsuPresent();                   /* queue this frame, draw the next in the other buffer */
        prepare_next_frame();
        gsuCacheLoad(renderer, renderer_size);
        gsuStartCached(0);
    }
    game_logic();                       /* every frame, whatever the GSU does */
    WaitForVBlank();
}
```

`examples/chips/superfx_game_skeleton` is that loop: a crosshair follows the
D-pad at 60 frames per second while the cube is rendered and presented at
30. With a blocking `gsuLaunch()` the loop is simpler (`gsuLaunch();
gsuPresent();`) and the transfer still overlaps the next job, but the game
waits for each job.

**How fast frames land.** The NMI reads the V counter and moves what the
rest of the blank window allows, at most 152 bytes per line (DMA is 8
master cycles a byte, 1324 usable per line: snesdev-wiki, Timing). The
window is the VBlank, extended into the top band of `gsuSetupHdmaBlanking`
when the bottom band is at least one line (it keeps force blank across
line 0). The bottom band gives the NMI nothing: the NMI starts at line
225. Measured on luna with `superfx_game_skeleton` (a 256×128 4bpp frame,
16 KB; the game loop at 60 fps in every case):

| Letterbox (top / bottom) | Most bytes moved in one VBlank | Frames presented per second |
|---|---|---|
| none | 4 712 | 15 |
| 40 / 40 | 10 640 | 30 |
| 76 / 4 | 16 112 | 30 |
| 84 / 4 | 16 384, the whole frame | 30: the job is now the limit |

Past that point the job decides: while the previous frame moves, the job
waits for Game Pak RAM, and in the skeleton the worst job took 135 000
clocks of work plus 137 000 of waiting, about three quarters of a frame.

**What it costs.** The DMA halts the CPU and the GSU waits for Game Pak RAM
for as long as a piece takes; a RAM-heavy job (clearing a buffer, PLOT)
loses most of that time. The single-buffer path reaches the same 30 frames
per second on this scene (`superfx_3d`, which also uses the bottom band),
but its CPU does nothing else: it waits for the job, then for the band,
then for the DMA.

**Rules.**

- The NMI moves frames only when the main thread is parked, in
  `WaitForVBlank()` or waiting for a job, like the tilemap and OAM uploads:
  a main thread in the middle of its own VRAM access would be corrupted. A
  game that never touches VRAM, VMAIN, VMADD, DMA channel 7 or the H/V
  counter latch outside the NMI can pass `GSU_PRESENT_ON_LAG_FRAMES` and
  have frames move at every VBlank.
- The NMI callback runs after the transfer: it must not write VRAM while a
  frame is in flight, the window is gone.
- Keep the letterbox on while presenting: the NMI trusts the bands
  `gsuSetupHdmaBlanking` last set.
- Code of your own that writes SCMR while frames are in flight keeps
  `gsu_scmr_live` (the library's copy of the write-only register), set
  before handing the GSU the buses and cleared before taking them back.
- Two framebuffers must fit in the Game Pak RAM the header declares
  (`GSU_RAM_KB`, 64 KB by default): `gsuPresentInit()` returns 0 otherwise.

`testing/vram_dma_blank.py` checks, with luna's DMA trace, that every
presented byte lands in blank or force blank, in whole frames into
alternating blocks, and that no swap shows a block before its frame is
complete.

## Saving

A Super FX cartridge has no separate save chip: when it has a battery, the
battery keeps the GSU's Game Pak RAM. `USE_SRAM := 1` beside
`USE_SUPERFX := 1` declares it (cartridge type `$15`), and the `sram`
module then reads and writes that RAM:

```c
#include <snes/sram.h>

#define SAVE_AT 0xE000          /* past the two 16 KB framebuffers */

sramSaveOffset((u8 *)&save, sizeof(save), SAVE_AT);
sramLoadOffset((u8 *)&save, sizeof(save), SAVE_AT);
```

Two things differ from an ordinary cartridge:

- **You choose where the save lives.** Offsets count from `$70:0000`, where
  the first framebuffer is, so `sramSave()` and `sramClear()` (offset 0)
  would write over it. Use the Offset functions and a region your SCBR
  settings leave alone; the whole RAM (`GSU_RAM_KB`, 64 KB by default) is
  what the battery keeps, framebuffers included.
- **The RAM is shared with the GSU.** While a job runs with RAN = 1 the CPU
  cannot touch it. The module clears RAN for the transfer, the GSU waits on
  its next RAM access, and SCMR is put back afterwards, through the same
  `gsu_scmr_live` the presentation NMI uses. A save in the middle of a job
  therefore works, and costs the job the time of the copy: keep it small,
  or save between jobs.

`testing/fixtures/libtests_gsu` saves while a job runs, and two luna manifests
(`f_gsu_save_write.toml`, `g_gsu_save_read.toml`) power the cartridge off
and on between the save and the read.

## Example ROMs

| Example | What it demonstrates |
|---------|---------------------|
| [superfx_3d](../../examples/chips/superfx_3d/) | Single-buffer presentation in the letterbox bands |
| [superfx_game_skeleton](../../examples/chips/superfx_game_skeleton/) | A game loop at 60 fps while the GSU renders from its cache; double-buffered presentation |

## See also

- @ref tutorial_superfx "Super FX: a first job" — build, GSU code, launch, PLOT.
- @ref tutorial_sram "SRAM saves" — the `sram` module this page's saving section uses.
- @ref superfx.h "superfx.h API reference"
