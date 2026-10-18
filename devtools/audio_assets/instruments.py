"""The instrument set every song shares — the sound of Néo-Lumen.

Looped single cycles for the sustained voices (64 or 128 bytes each: the
SPC700's RAM stays free for the effects and the echo), short one-shots for
the plucked and struck ones. `c5` is the playback rate that makes note C-5
sound C5 (523.25 Hz): a cycle of n samples needs n x 523.25.
"""
import itwriter as it
import synth as sy

C5 = 523.25


def lead():
    """Pulse 25 %, a little of the octave below: the singing lead."""
    c = sy.mix_cycles((sy.cycle("pulse", 64, duty=0.25, harmonics=20), 0.8),
                      (sy.cycle("sine", 64), 0.35))
    return it.Sample("lead", sy.to16(c), 64 * C5, loop=(0, 64), volume=56)


def pad():
    """Soft saw, 128 samples: the chords under everything."""
    c = sy.mix_cycles((sy.cycle("saw", 128, harmonics=12), 0.7),
                      (sy.cycle("triangle", 128), 0.5))
    return it.Sample("pad", sy.to16(c), 128 * C5, loop=(0, 128), volume=40)


def bass():
    """Triangle with a hint of square: round, it does not mask the kick."""
    c = sy.mix_cycles((sy.cycle("triangle", 64), 0.9),
                      (sy.cycle("square", 64, harmonics=7), 0.25))
    return it.Sample("bass", sy.to16(c), 64 * C5, loop=(0, 64), volume=60)


def pluck(rate=16000):
    """A Karplus-Strong string at C4, half a second: the arpeggios."""
    f0 = C5 / 2
    return it.Sample("pluck", sy.to16(sy.pluck(f0, rate, 0.5, damping=0.994)),
                     rate * C5 / sy.pluck_pitch(f0, rate), volume=48)


def bell(rate=16000):
    """An FM bell at C5, the sparkle of the neon signs."""
    return it.Sample("bell", sy.to16(sy.fm_bell(C5, rate, 0.7)), rate, volume=40)


def kick(rate=16000):
    return it.Sample("kick", sy.to16(sy.kick(rate)), rate, volume=64)


def snare(rate=16000):
    return it.Sample("snare", sy.to16(sy.snare(rate)), rate, volume=48)


def hat(rate=16000):
    return it.Sample("hat", sy.to16(sy.hat(rate)), rate, volume=36)
