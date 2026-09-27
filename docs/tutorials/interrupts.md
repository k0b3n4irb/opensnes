# Interrupts: VBlank callbacks and timer IRQs {#tutorial_interrupts}

Two kinds of interrupt reach your code. The **NMI** fires at the start of
every VBlank; the SDK's handler owns it and lets you add a C callback.
The **IRQ** fires when the PPU reaches a horizontal and/or vertical
position you choose; it goes straight to an assembly handler you write.
Both come from `<snes/interrupt.h>` (module `console`).

## Running C at every VBlank: `nmiSet`

```c
static u16 ticks;

static void on_vblank(void) {
    ticks++;                     /* keep it short */
}

int main(void) {
    consoleInit();
    nmiSet(on_vblank);
    /* ... */
    nmiClear();                  /* same as nmiSet(NULL) */
}
```

The callback runs inside the NMI handler, after the library's own VBlank
work (sprite upload, text flush, scroll sync), and only on frames where
the main loop was waiting in `WaitForVBlank()`: on a lag frame it is
skipped with the rest. Inside it, never call `WaitForVBlank()` (it would
wait for itself), never touch the WRAM data port `$2180-$2183`, and keep
it short — `make test-nmi-budget` measures the handler. Plain C
multiplication and division are safe there; `fixMul()` and `fix32Mul()`
are not. The full list is on `nmiSet()` in the header.

## Timer IRQs: code at a chosen scanline

The CPU can be interrupted when the PPU's counters reach a position you
set: the horizontal timer alone fires on every scanline, the vertical
timer alone once per frame on one line, and both together at one exact
point (anomie's register doc, `$4207`; snesdev-wiki, MMIO registers).

```c
extern void my_irq(void);        /* in an .asm file, see below */

irqSet(my_irq);                  /* the handler */
irqSetVTimer(200);               /* line 200 */
irqEnable(IRQ_VTIMER);           /* arm it, and allow interrupts */
/* ... */
irqDisable();                    /* both timers off */
irqClear();                      /* back to the default handler */
```

Set the handler and the timer values before `irqEnable()`. The
vertical target runs 0-261 on NTSC and 0-311 on PAL; the horizontal one
(`irqSetHTimer()`) 0-339. A value past the end of the line or frame is
never reached, so the IRQ never fires (snesdev-wiki, `HTIME`/`VTIME`).
`irqDisable()` clears both timer bits and leaves the NMI and the joypad
reading alone.

## The handler is assembly

The IRQ vector jumps straight into your handler, with nothing saved: an
H-timer IRQ fires about 15 700 times a second and cannot pay for a C
function's setup. **Passing a C function to `irqSet()` corrupts the main
program.** The handler saves what it touches, acknowledges the IRQ by
reading `$4211` (the timer keeps its IRQ output active until that read —
anomie's timing doc, S-CPU interrupts) and returns with `rti`:

```asm
.SECTION ".my_irq" SEMIFREE
my_irq:
    rep #$20
    .ACCU 16
    pha
    phb
    pea $0000
    plb
    plb                     ; data bank $00 for irq_ticks and $4211
    inc.w irq_ticks
    sep #$20
    .ACCU 8
    lda.w $4211             ; acknowledge
    rep #$20
    .ACCU 16
    plb
    pla
    rti
.ENDS
```

This is the V-timer handler of `examples/chips/superfx_3d`
(`gsu_loader.asm`). `examples/color/hicolor_1792` (`irq_stream.asm`)
is the heavy case: an H-timer handler that streams palette data into
CGRAM on every line. A handler that starts a DMA owns that channel:
do not call the library's DMA helpers from the main loop while it is
armed.

## See also

- @ref tutorial_hdma — per-line register writes without any CPU time,
  and when to prefer an IRQ.
- @ref tutorial_profiling — measuring what the NMI handler costs.
