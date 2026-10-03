TITLE: ResetSound clears KOF too early, voices can keep playing after a stop

Hi,

I maintain a small SNES SDK that ships SNESMOD, and I ran into something in
the SPC driver that I think has been there since the first commit.

In driver/spc/sm_spc.asm, ResetSound does this:

```
	SETDSP( DSP_KOF, 0FFh );
	SETDSP( DSP_FLG, FLG_ECEN );
	SETDSP( DSP_PMON, 0 );
	SETDSP( DSP_EVOL, 0 );
	SETDSP( DSP_EVOLR, 0 );
	SETDSP( DSP_NON, 00h );
	SETDSP( DSP_KOF, 000h ); this is weird

	mov	sfx_mask, #0
	ret
```

SETDSP is two "mov dp,#imm", 5 cycles each, so KOF goes back to 0 exactly 60
SPC cycles after it was set to $FF. The S-DSP only looks at KON and KOFF every
other sample, which is every 64 cycles. When no poll happens to fall inside
those 60 cycles, the DSP never sees the $FF and the voices are not keyed off.

anomie's S-DSP doc gives this very sequence as an example:

```
These registers seem to be polled only at 16000 Hz, when every other
sample is due to be output. Thus, if you write two values in close
succession, usually but not always only the second value will have an
effect:
  [...]
  mov $f2, #$5c  ; KOFF = $ff then KOFF = 0
  mov $f3, #$ff
  mov $f3, #$00  ; -> *usually* all voices remain playing
```

and the SNESdev wiki lists it under the KOFF errata: "Clearing KOFF too early
can cause the voice to not key-off."

How I found it: one of my tests plays a module, sends a stop, and checks that
ENVX of the eight voices is 0 a couple of seconds later. It started failing
after an emulator update. So I swept the frame on which the stop is sent over
161 consecutive frames (40 to 200). On 8 of them some voices were still
sounding 140 frames after the stop, for example V1 at $7B and V3/V4 at $4E.
Pause gives the same count. The emulator update had only moved which frames
are the unlucky ones. Module_Start goes through ResetSound too, so it is not
only about stopping.

(The build I ship is the one from PVSnesLib, but ResetSound is byte for byte
the same as in your smconv/smconv/sm_spc.bin, at offset $6D.)

What fixed it for me is moving the "mov sfx_mask, #0" between the last two
writes:

```
	SETDSP( DSP_NON, 00h );
	mov	sfx_mask, #0
	SETDSP( DSP_KOF, 000h );
	ret
```

That adds 5 cycles, the gap becomes 65, and a poll always falls inside it. The
code size does not change. With this I get 0 failures out of 161, for stop and
for pause.

In sm_spc.bin it is the 9 bytes at offset $91, nothing else moves:

```
8F 5C F2 8F 00 F3 8F 00 C1   before
8F 00 C1 8F 5C F2 8F 00 F3   after
```

Thanks for SNESMOD, by the way.
