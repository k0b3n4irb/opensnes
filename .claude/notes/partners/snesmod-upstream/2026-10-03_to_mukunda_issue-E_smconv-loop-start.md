TITLE: smconv (Go): Loop points one block early when the loop start is not a multiple of 16

Hi,

In createSource (smconv/smconv/source.go) the loop offset written to the
soundbank is:

```
	source.Loop = loopStart / 16 * 9
```

which rounds down. But when the loop start is not on a block boundary the BRR
codec moves it up to the next one (snesbrr, brr/brr-noc.go, "Align loop start
to 16 samples": it appends pcmData[loopPoint] and increments loopPoint until
it is aligned). So the two disagree by one block.

Example, with a ramp as input (sample i has the value i*8):

```
256 samples, forward loop 100-256
output: 414 BRR bytes = 736 samples, Loop field 54
```

736 is 112 + 4 * 156: the codec put the loop start at sample 112 (block 7,
byte 63) and unrolled the 156-sample loop four times. source.Loop is 54,
block 6. So the driver jumps back 16 samples too early on every pass, to a
block that was not encoded as a loop start.

The C++ version padded the start of the data instead (PadData with
16 - (loopstart & 15) samples in front), so the loop start landed on a
boundary and rounding down was right.

I am not sure which way you would prefer to fix it: pad the front as before,
or take the aligned loop start back from the codec.

(This is case 4 of the small test in my issue about the loop end.)
