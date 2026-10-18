# wav2brr — library, not a tool

The converter of the SDK's first WAV converter. Its binary is no longer built or shipped: the
code is linked into `opensnes-sample`, which write the same bytes.

What is here: no source (the encoder is smconv's `brr.c`); `tests/golden` is what opensnes-sample's suite compares against.

A project that used the `wav2brr` command: `docs/tools/legacy.md` maps its
options to the tool that replaced it.
