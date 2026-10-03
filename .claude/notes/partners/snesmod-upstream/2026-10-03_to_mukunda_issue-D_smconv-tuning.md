TITLE: smconv (Go): the tuning factor of a resampled loop is inverted

Hi,

When a loop cannot be unrolled to a multiple of 16 samples, createSource
resamples it and returns a tuning factor that convertSample multiplies into
C5. In the Go version that factor goes the wrong way.

resampleLoop (smconv/smconv/source.go) ends with:

```
	return iResampleFactor, resampledData, newLength, newLoopStart
```

iResampleFactor is old length / new length, so it is below 1. The C++ version
(convert/source/brr.cpp, ResampleLoop) had:

```
	return 1.0/factor; // must scale center freq by this factor!!
```

with factor = length / new_length, so it returned new / old, above 1. That is
the right direction: the sample got longer, so it has to be played faster to
sound the same.

The case where I saw it: pollen8.it, sample 17 (C5 = 8363, ping-pong loop
317-969, which ends up resampled). Converting it with "smconv -s", the pitch
base written for that sample in the soundbank is -14 with the Go smconv and +5
with the old code (I use a C port of your C++ smconv, same ResampleLoop). -14 is round(log2(652/660) * 768), and 0.98788 = 652/660 is
the TuningFactor that createSource returns for it.

Returning resampleFactor instead of iResampleFactor fixes it here.

(This is case 3 of the small test in my issue about the loop end. The lengths
are different there because of that other problem, but the direction of the
factor is the same.)
