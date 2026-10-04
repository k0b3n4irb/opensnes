TITLE: Panning set from the volume column does not clear surround

Hi,

This one is not my finding. It is a fix by KungFuFurby, dated 12/20/15, that
PVSnesLib carries in its copy of the driver and that never made it back here.
I noticed it while diffing their sm_spc source against yours.

S91 (SCommand_SoundControl) sets CF_SURROUND on the channel, and the right
volume is then negated where the panning is applied. Command_SetPanning (Xxx)
clears the flag again:

```
	mov	a, ch_flags+x
	and	a, #~CF_SURROUND
	mov	ch_flags+x, a
```

but vcmd_pan, the panning from the volume column (128-192), only stores
ch_panning:

```
vcmd_pan:
	cmp	mod_tick, #0		; set panning
	bne	exit_vcmd		;
	push	a			;
	mov	a, y			;
	sbc	a, #128			;
	mov	ch_panning+x, a		;
	pop	a			;
	ret				;
```

So after an S91, a pan set in the volume column leaves the channel in
surround, while the same pan set with Xxx does not.

The PVSnesLib version (pvsneslib/snesmod/sm_spc.as7) adds the same three
instructions after the store:

```
	mov	ch_panning+x, a		;

	mov	a, ch_flags+x		; Bugfix by KungFuFurby 12/20/15
	and	a, #~CF_SURROUND	; Surround should be disabled
	mov	ch_flags+x, a		; when panning is set via volume

	pop	a			;
	ret				;
```

To be clear, I read this in the two sources, I did not measure it on a module.
The build I ship comes from PVSnesLib, so it already has the fix.

While I was there: their copy also drops the TEST command and adds a pause
(0Ah) and a resume (0Bh) of the module, with SSIZE moved to 09h, in case that
is of any use to you.
