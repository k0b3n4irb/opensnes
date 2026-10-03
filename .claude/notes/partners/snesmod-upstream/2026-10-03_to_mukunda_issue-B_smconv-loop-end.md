TITLE: smconv (Go): a loop that ends before the end of the sample data is not honoured

Hi,

I compared the new Go smconv with the old C++ one on a module of mine and one
sample came out very different: pollen8.it, sample 17, which has 1878 samples
and a ping-pong loop from 317 to 969. The cause seems to be in createSource
(smconv/smconv/source.go): the data is never cut at the loop end.

To see what happens I fed it ramps (sample i has the value i*8), so that a
decoded sample tells which input sample it came from. The test is at the end.

1. Data after the loop end is kept, and becomes part of the loop.

400 samples, forward loop 96-256.
Expected: 256 samples out, loop 96-256.
Got: 400 samples out (225 BRR bytes), the loop runs from 96 to 400.

"length" is trimmed to the loop end:

```
	length = min(length, loopStart+loopLength)
```

but sampleData is not, and the codec is given the whole slice. SetLoop then
takes everything after the loop start as the loop. The C++ version only
copied "length" samples into cdata, so the tail was dropped there.

2. The ping-pong unroll is appended after that tail.

400 samples, ping-pong loop 96-176.
Expected: 0..175, then 175..96, 256 samples in total.
Got: 480 samples: 0..399, then 175..96. So the loop plays 96 to 399 forward
before it goes backward.

3. When a ping-pong loop has to be resampled, the backward half is lost.

1878 samples, ping-pong loop 317-969 (the pollen8 sample). The unrolled loop
is 1304 samples, which is over kMaxUnrollThreshold once aligned, so it goes to
resampleLoop. But "length" was not updated after the unroll, it is still 969,
so resampleLoop only sees the forward half.

A loop that starts on a block boundary and runs to the end of the data comes
out fine (256 samples, loop 96-256: 256 samples out, Loop 54).

Something like this near the top of createSource seems enough for the three:

```
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
```

The test I used. Drop it in smconv/smconv/ as zz_loops_test.go and run
"go test ./smconv -run TestLoopShapes -v". It only prints, it asserts nothing.

```go
package smconv

import (
	"fmt"
	"testing"

	"go.mukunda.com/modlib/common"
	"go.mukunda.com/snesbrr/v2/brr"
)

// ramp: sample i has value i*8, so a decoded value tells which input sample it came from.
func ramp(n int) []int16 {
	d := make([]int16, n)
	for i := range d {
		d[i] = int16(i * 8)
	}
	return d
}

func decode(data []byte) []int16 {
	c := brr.NewCodec()
	c.BrrData = data
	c.Decode()
	return c.PcmData
}

func show(t *testing.T, name string, s common.Sample) {
	src, err := createSource(s)
	if err != nil {
		t.Fatal(err)
	}
	pcm := decode(src.Data)
	idx := func(i int) int { return int(pcm[i]+4) / 8 }
	ls := src.Loop / 9 * 16
	fmt.Printf("%s\n  input: %d samples, loop %d-%d pingpong=%v\n", name, len(s.Data.Data[0].([]int16)), s.LoopStart, s.LoopEnd, s.PingPong)
	fmt.Printf("  output: %d BRR bytes = %d samples, Loop field %d (sample %d), tuning %.5f\n", len(src.Data), len(pcm), src.Loop, ls, src.TuningFactor)
	n := len(pcm)
	fmt.Printf("  source index of decoded samples: [0]=%d [%d]=%d ... last four: %d %d %d %d\n", idx(0), ls, idx(ls), idx(n-4), idx(n-3), idx(n-2), idx(n-1))
	if n > 300 {
		fmt.Printf("  around 250..262: ")
		for i := 250; i < 262; i++ {
			fmt.Printf("%d ", idx(i))
		}
		fmt.Println()
	}
}

func mk(n, ls, le int, pp bool) common.Sample {
	return common.Sample{Loop: true, PingPong: pp, LoopStart: ls, LoopEnd: le, C5: 8363,
		Data: common.SampleData{Channels: 1, Bits: 16, Data: []any{ramp(n)}}}
}

func TestLoopShapes(t *testing.T) {
	show(t, "1. forward loop ending before the data end (400 samples, loop 96-256)", mk(400, 96, 256, false))
	show(t, "2. ping-pong loop, short (400 samples, loop 96-176)", mk(400, 96, 176, true))
	show(t, "3. ping-pong loop that needs resampling (1878 samples, loop 317-969)", mk(1878, 317, 969, true))
	show(t, "4. forward loop to the end, start not on a block (256 samples, loop 100-256)", mk(256, 100, 256, false))
	show(t, "5. control: forward loop to the end, aligned (256 samples, loop 96-256)", mk(256, 96, 256, false))
}
```

What it prints here, on main (3e4990a):

```
1. forward loop ending before the data end (400 samples, loop 96-256)
  input: 400 samples, loop 96-256 pingpong=false
  output: 225 BRR bytes = 400 samples, Loop field 54 (sample 96), tuning 1.00000
  source index of decoded samples: [0]=0 [96]=96 ... last four: 397 397 399 399
  around 250..262: 250 251 252 253 254 255 256 257 258 259 260 261
2. ping-pong loop, short (400 samples, loop 96-176)
  input: 400 samples, loop 96-176 pingpong=true
  output: 270 BRR bytes = 480 samples, Loop field 54 (sample 96), tuning 1.00000
  source index of decoded samples: [0]=0 [96]=96 ... last four: 99 98 97 96
  around 250..262: 250 251 252 253 254 255 256 257 258 259 260 261
3. ping-pong loop that needs resampling (1878 samples, loop 317-969)
  input: 1878 samples, loop 317-969 pingpong=true
  output: 1674 BRR bytes = 2976 samples, Loop field 180 (sample 320), tuning 0.98788
  source index of decoded samples: [0]=0 [320]=316 ... last four: 306 351 329 308
  around 250..262: 247 248 249 250 251 252 253 254 255 256 257 258
4. forward loop to the end, start not on a block (256 samples, loop 100-256)
  input: 256 samples, loop 100-256 pingpong=false
  output: 414 BRR bytes = 736 samples, Loop field 54 (sample 96), tuning 1.00000
  source index of decoded samples: [0]=0 [96]=96 ... last four: 104 113 106 115
  around 250..262: 250 251 252 253 254 255 96 96 96 96 112 112
5. control: forward loop to the end, aligned (256 samples, loop 96-256)
  input: 256 samples, loop 96-256 pingpong=false
  output: 144 BRR bytes = 256 samples, Loop field 54 (sample 96), tuning 1.00000
  source index of decoded samples: [0]=0 [96]=96 ... last four: 252 253 254 255
```

Cases 3 and 4 also show two other things, which I am filing separately so
they can be looked at one by one.
