# OpenSNES → luna: `profile` folds a bank-$00 PC onto a RAM label (2026-10-30)

Pin: luna v1.41.0 (`testing/luna.version`). Status: sent as is, the same day
it was found. One request, `léger`, not blocking.

## What luna made possible first

The porting kit reads `luna profile` on a game built with this SDK to
measure its load per routine; this report comes from that reading. The
symptom it saw first was ours: wlalink's RAM-usage markers,
`RAM_USAGE_SLOT_2_BANK_126_START` written `00:2000` in the `.sym` for a
label at `$7E:2000`, took 1,177 instructions and 39 PCs of a 200-frame
window. The markers are gone since `develop` `4dca0011`
(`opensnes-rom check` removes them after every link); the cause under
them is the one below, and it is still there.

## The fact, reproduced on our ROM

```sh
luna profile examples/sprites/dynamic_sprite/dynamic_sprite.sfc --from-frame 60 --until-frame 260 --out prof.json
```

gives, among the 64 rows:

```
symbol         addr      instructions  mclk  pcs
oamQueueEntry  0x001520  200           6400  1
```

`oamQueueEntry` is a RAM variable (`00:1520` in the `.sym`, the dynamic
sprites' queue). The one PC credited to it is `$00:8248`: `NmiEntry`, the
NMI vector's target, which runs its single `jml.l NmiHandler` in bank $00
once a frame (200 frames, 200 instructions). The `.sym` names it — but as
`80:8248`: every ROM unit of the SDK is assembled under `.BASE $80` (`$C0`
for HiROM), the mirror the vectors leave for, so every ROM label is in
`$80-$BF` while the vectors' first instructions execute in `$00-$3F`.

The fold takes the nearest **preceding** label in 24-bit order: below
`$00:8248` there is no `00:8xxx` label, so it lands on the highest RAM
label (`00:1520`), 28 KB away. On the game the kit profiles, the same fold
put 39 PCs of `$00:8000-$824D` (its vectors' bank-$00 code) under the RAM
marker; with the marker gone they will land on its highest RAM variable
instead. A reader of the profile sees a RAM variable, or a RAM marker,
executing code.

## What we ask

1. **Fold the mirror when matching a PC to a label**: a PC in `$00-$3F`
   matches a label in `$80-$BF` at the same offset (LoROM / FastROM
   mirror), and a PC in `$40-$7D` a label in `$C0-$FF` (HiROM), when its
   own bank has no label at or before it in the same 32 KB half. Our own
   `.sym` reader does exactly this (`sym_rom_bank` in
   `tools/opensnes-rom/src/sym.c`: `$C0+` → `-$C0`, `$80-$BF` → `-$80`),
   so does `devtools/symmap/symmap.py`. With it, the row above reads
   `NmiEntry  200  6400  1`.
2. **Never credit a ROM PC to a RAM label**: a label whose address is
   under `$8000` in banks `$00-$3F` / `$80-$BF`, or in `$7E`/`$7F`, is
   data; a ROM PC with no code label before it in its bank should go to
   the "256-byte page without a label" row `profile` already has, not to
   the last RAM label. The marker case would then have read `page
   $00:8200` — true, and a question — instead of a RAM marker executing.

Contract we would use: unchanged command line; the row's `symbol` is the
mirrored label's name, `addr` stays the PC's bank (`0x008248`) or the
label's — either, said in the JSON. We replay with the command above: the
`oamQueueEntry` row disappears, `NmiEntry` appears with 200 instructions.

## Observation, no ask

The 39 PCs the kit saw span `$00:8000-$824D` on its game, more than one
`jml`: our `crt0.asm` keeps `NmiEntry`, `IrqHandler`'s stub and the boot
code in that bank-$00 range, and the game's window (frames 200-400) may
also run its IRQ entry there. Once the fold is in, the rows will say which.
