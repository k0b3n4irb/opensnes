TITLE: smconv (Go): a loop that ends before the end of the sample data is not honoured

Hi,

I compared the new Go smconv with the old C++ one on a module of mine and one
sample came out very different: pollen8.it, sample 17, which has 1878 samples
and a ping-pong loop from 317 to 969. The cause seems to be in createSource
(smconv/smconv/source.go): the data is never cut at the loop end.

To see what happens I fed it ramps (sample i has the value i*8), so that a
decoded sample tells which input sample it came from.

1. Data after the loop end is kept, and becomes part of the loop.

   400 samples, forward loop 96-256.
   Expected: 256 samples out, loop 96-256.
   Got: 400 samples out (225 BRR bytes), the loop runs from 96 to 400.

   "length" is trimmed to the loop end:

	length = min(length, loopStart+loopLength)

   but sampleData is not, and the codec is given the whole slice. SetLoop
   then takes everything after the loop start as the loop. The C++ version
   only copied "length" samples into cdata, so the tail was dropped there.

2. The ping-pong unroll is appended after that tail.

   400 samples, ping-pong loop 96-176.
   Expected: 0..175, then 175..96, 256 samples in total.
   Got: 480 samples: 0..399, then 175..96. So the loop plays 96 to 399
   forward before it goes backward.

3. When a ping-pong loop has to be resampled, the backward half is lost.

   1878 samples, ping-pong loop 317-969 (the pollen8 sample). The unrolled
   loop is 1304 samples, which is over kMaxUnrollThreshold once aligned, so it
   goes to resampleLoop. But "length" was not updated after the unroll, it is
   still 969, so resampleLoop only sees the forward half.

A loop that starts on a block boundary and runs to the end of the data comes
out fine (256 samples, loop 96-256: 256 samples out, Loop 54).

Something like this near the top of createSource seems enough for the three:

	if loopLength > 0 {
		// Discard data after loop end.
		length = min(length, loopStart+loopLength)
		sampleData = sampleData[:length]
	}

	if modsamp.PingPong {
		...
		loopLength *= 2
		length = len(sampleData)
	}

I have the small test I used (it prints the numbers above) and can send it, or
a PR, whichever is easier for you.
