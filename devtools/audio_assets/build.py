"""Write the examples' sounds: `make audio-assets` (contributor-only).

Every output is committed; this script is how it was made, and how to make
it again. It needs ffmpeg (the Kenney OGG files) and bin/opensnes-sample.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)

import itwriter as it   # noqa: E402
import songs            # noqa: E402
import synth as sy      # noqa: E402

MODULES = {
    "examples/audio/snesmod_music/music/nocturne.it": songs.nocturne,
    "examples/audio/snesmod_music/sfx/neo_lumen_sfx.it": songs.neo_lumen_sfx,
    "examples/games/blocks/music/blocks_theme.it": songs.korobeiniki,
    "examples/games/likemario/res/rooftops.it": songs.rooftops,
    "examples/games/likemario/res/jump_sfx.it": songs.rooftops_sfx,
    "testing/fixtures/retired_examples/snesmod_music_large/music/nocturne_large.it": songs.nocturne_large,
}

# The cello of apu_switch and soundboard: played at pitch $08BB (17 463 Hz
# of output), a 100-sample period sounds 174.6 Hz, the F3 the examples were
# written for. 273 blocks of attack, then 8 periods = 800 samples of loop.
CELLO_RATE, CELLO_PERIOD = 17463, 100
CELLO = ["examples/audio/apu_switch/res/cello", "examples/audio/soundboard/res/cello"]


def cello():
    """Encode the cello once in a scratch directory, copy the .brr where the
    examples .incbin it (the WAV and the generated .h are not kept)."""
    xs, ls, le = sy.bowed_note(CELLO_PERIOD, CELLO_RATE, attack_blocks=273, loop_periods=8)
    # 60 %: the level of the cello it replaced (luna diff --audio: the full-
    # scale tone was 75 % louder, and the long echo took it near clipping)
    xs = [x * 0.6 for x in xs]
    with tempfile.TemporaryDirectory() as tmp:
        wav = os.path.join(tmp, "cello.wav")
        sy.write_wav(wav, xs, CELLO_RATE)
        subprocess.run([os.path.join(ROOT, "bin", "opensnes-sample"), "encode", "-q",
                        wav, "--loop", str(ls), str(le)], check=True)
        for base in CELLO:
            shutil.copyfile(os.path.join(tmp, "cello.brr"), os.path.join(ROOT, base + ".brr"))
    print("cello: loop at sample %d, byte %d of the BRR" % (ls, ls // 16 * 9))


def main():
    for rel, song in MODULES.items():
        path = os.path.join(ROOT, rel)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        it.write(path, **song())
        print("wrote", rel)
    cello()


def check():
    """Write everything into a copy of the outputs and compare: exit 1 when a
    committed file is not what this script writes."""
    rels = list(MODULES) + [b + ".brr" for b in CELLO]
    before = {r: open(os.path.join(ROOT, r), "rb").read() for r in rels}
    main()
    bad = [r for r in rels if open(os.path.join(ROOT, r), "rb").read() != before[r]]
    for r in rels:
        with open(os.path.join(ROOT, r), "wb") as f:
            f.write(before[r])
    for r in bad:
        print("differs:", r)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(check() if "--check" in sys.argv[1:] else main())
