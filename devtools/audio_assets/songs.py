"""The songs of the examples, composed for them (CC0, like everything this
directory writes). Each function returns the arguments of itwriter.write."""
import itwriter as it
import instruments as ins
import synth as sy
from score import put, repeat

# Instrument numbers, in the order of SET below
LEAD, PAD, BASS, PLUCK, BELL, KICK, SNARE, HAT = range(1, 9)
SET = [ins.lead, ins.pad, ins.bass, ins.pluck, ins.bell, ins.kick, ins.snare, ins.hat]

# Chords of a 16-row bar: arpeggio, pad note, bass root
AM = ("A-4 C-5 E-5 C-5", "E-4", "A")
FM = ("F-4 A-4 C-5 A-4", "F-4", "F")
CM = ("G-4 C-5 E-5 C-5", "E-4", "C")
GM = ("G-4 B-4 D-5 B-4", "D-4", "G")
DM = ("F-4 A-4 D-5 A-4", "D-4", "D")
EM = ("G#4 B-4 E-5 B-4", "E-4", "E")


def _bar(p, at, chord, arp=True, pad=True, bass=True, drums=0, oct_=2):
    arpeggio, padnote, root = chord
    if arp:
        put(p, 1, PLUCK, repeat(arpeggio, 4), at, vol=30)
    if pad:
        put(p, 2, PAD, padnote + " " + repeat(".", 15), at, vol=26)
    if bass:
        r2, r3 = "%s-%d" % (root, oct_), "%s-%d" % (root, oct_ + 1)
        put(p, 3, BASS, "%s . . %s . . %s . %s . . %s . . %s ." % (r2, r2, r3, r2, r2, r3), at, vol=44)
    if drums:
        put(p, 4, KICK, "C-5 . . . . . . . C-5 . . C-5 . . . .", at)
        if drums > 1:
            put(p, 4, SNARE, ". . . . C-5 . . . . . . . C-5 . . .", at)
            put(p, 5, HAT, repeat(". . C-5:28 .", 4), at)


def nocturne():
    """Néo-Lumen nights: the rooftop city after rain. A minor, i-VI-III-VII,
    a pulse lead over a plucked arpeggio, a pad, a bass and light drums;
    a long echo on the lead and the arpeggio. Channel 8 (and 7) stay free:
    SNESMOD plays its sound effects there."""
    prog = [AM, FM, CM, GM]
    pats = []
    # 0: intro — arpeggio and pad only
    p = it.Pattern(64)
    for b, ch in enumerate(prog):
        _bar(p, 16 * b, ch, bass=b >= 2)
    p.set(0, 0, fx=it.cmd("A", 6))
    p.set(0, 1, fx=it.cmd("T", 104))
    pats.append(p)
    # 1: groove — the drums come in
    p = it.Pattern(64)
    for b, ch in enumerate(prog):
        _bar(p, 16 * b, ch, drums=1 if b < 2 else 2)
    pats.append(p)
    # 2, 3: the theme, twice; the second answers the first
    theme = [
        "E-5:48/H24 . . . D-5 . C-5 . D-5 . E-5 . . . A-4 .",
        "C-5/H24 . . . A-4 . . . C-5 . D-5 . C-5 . A-4 .",
        "G-4/H24 . . . C-5 . E-5 . G-5/H24 . . . E-5 . D-5 .",
        "D-5/H24 . . . . . B-4 . D-5 . . . G-5/H34 . . .",
    ]
    answer = theme[:3] + ["D-5 . E-5 . D-5 . B-4 . A-4/H34 . . . . . ^ ."]
    for melody in (theme, answer):
        p = it.Pattern(64)
        for b, ch in enumerate(prog):
            _bar(p, 16 * b, ch, drums=2)
            put(p, 0, LEAD, melody[b], 16 * b, vol=44)
        pats.append(p)
    # 4: the bridge — F, G, Em, Am, with a bell on the off-beats
    p = it.Pattern(64)
    for b, ch in enumerate([FM, GM, EM, AM]):
        _bar(p, 16 * b, ch, drums=2)
        put(p, 0, BELL, ". . . . %s . . . . . . . %s . . ." % (ch[1].replace("4", "6"), ch[1].replace("4", "6")), 16 * b, vol=30)
    pats.append(p)
    samples = [f() for f in SET]
    message = "[[SNESMOD]]\nedl 5\nefb 48\nevol 28\nefir 127 0 0 0 0 0 0 0\neon 1 2\n"
    pan = [36, 22, 32, 32, 32, 44, 32, 32]
    return dict(title="Daveo Nocturne", samples=samples, patterns=pats,
                orders=[0, 1, 2, 3, 4, 2, 3], speed=6, tempo=104,
                message=message, pan=pan)


def seq(notes, vol=None):
    """[(note, rows), ...] to a token string; '.' as a note is a rest."""
    out = []
    for n, rows in notes:
        head = n if n == "." else (n + (":%d" % vol if vol else ""))
        out += [head] + ["."] * (rows - 1)
    return " ".join(out)


def _bass8(root, oct_=2):
    lo, hi = "%s-%d" % (root, oct_), "%s-%d" % (root, oct_ + 1)
    return " ".join([lo, ".", hi, "."] * 4)


def korobeiniki():
    """Korobeiniki, the 19th-century Russian song (public domain), arranged
    here for the falling-blocks game: a dry pulse lead, a pad on the chord,
    an octave bass and a brisk beat. Part A twice, then the slow part B."""
    # quarter note = 4 rows, 16 rows a bar
    a1 = [("E-5", 4), ("B-4", 2), ("C-5", 2), ("D-5", 4), ("C-5", 2), ("B-4", 2),
          ("A-4", 4), ("A-4", 2), ("C-5", 2), ("E-5", 4), ("D-5", 2), ("C-5", 2),
          ("B-4", 6), ("C-5", 2), ("D-5", 4), ("E-5", 4),
          ("C-5", 4), ("A-4", 4), ("A-4", 4), (".", 4)]
    a2 = [(".", 2), ("D-5", 4), ("F-5", 2), ("A-5", 4), ("G-5", 2), ("F-5", 2),
          ("E-5", 6), ("C-5", 2), ("E-5", 4), ("D-5", 2), ("C-5", 2),
          ("B-4", 4), ("B-4", 2), ("C-5", 2), ("D-5", 4), ("E-5", 4),
          ("C-5", 4), ("A-4", 4), ("A-4", 4), (".", 4)]
    b = [("E-5", 8), ("C-5", 8), ("D-5", 8), ("B-4", 8), ("C-5", 8), ("A-4", 8),
         ("G#4", 8), ("B-4", 4), (".", 4),
         ("E-5", 8), ("C-5", 8), ("D-5", 8), ("B-4", 8), ("C-5", 4), ("E-5", 4),
         ("A-5", 8), ("G#5", 16)]
    chords_a1 = [("A", "C-5"), ("A", "C-5"), ("E", "B-4"), ("A", "C-5")]
    chords_a2 = [("D", "F-4"), ("A", "E-4"), ("E", "G#4"), ("A", "E-4")]
    chords_b = [("A", "E-4"), ("E", "G#4"), ("A", "E-4"), ("E", "G#4")] * 2

    def pattern(melody, chords, drums=True, lead_vol=46):
        p = it.Pattern(16 * len(chords))
        put(p, 0, LEAD, seq(melody, lead_vol), 0)
        for i, (root, padnote) in enumerate(chords):
            at = 16 * i
            put(p, 2, PAD, padnote + " " + repeat(".", 15), at, vol=22)
            put(p, 3, BASS, _bass8(root), at, vol=42)
            if drums:
                put(p, 4, KICK, "C-5 . . . . . . . C-5 . . . . . . .", at)
                put(p, 4, SNARE, ". . . . C-5 . . . . . . . C-5 . . .", at)
                put(p, 5, HAT, repeat("C-5:22 . C-5:14 .", 4), at)
        return p

    pats = [pattern(a1, chords_a1), pattern(a2, chords_a2),
            pattern(b[:9], chords_b[:4], lead_vol=40), pattern(b[9:], chords_b[4:], lead_vol=40)]
    pats[0].set(0, 1, fx=it.cmd("A", 5))
    pats[0].set(0, 6, fx=it.cmd("T", 140))
    samples = [f() for f in SET]
    message = "[[SNESMOD]]\nedl 2\nefb 24\nevol 16\nefir 127 0 0 0 0 0 0 0\neon 1\n"
    return dict(title="Korobeiniki arr. OpenSNES", samples=samples, patterns=pats,
                orders=[0, 1, 0, 1, 2, 3, 0, 1], speed=5, tempo=140,
                message=message, pan=[32, 32, 30, 32, 32, 40, 32, 32])


def rooftops():
    """The rooftops of Néo-Lumen, for the platformer: bright and quick, C
    major with a turn to A minor, the lead jumping by fifths like Kit from
    roof to roof, the arpeggio running underneath."""
    C = ("C-5 E-5 G-5 E-5", "E-4", "C")
    G = ("B-4 D-5 G-5 D-5", "D-4", "G")
    A = ("A-4 C-5 E-5 C-5", "E-4", "A")
    F = ("A-4 C-5 F-5 C-5", "F-4", "F")
    D = ("A-4 D-5 F-5 D-5", "D-4", "D")
    verse = [C, G, A, F]
    chorus = [F, G, C, A, D, G, C, C]
    m_verse = [
        [("C-5", 2), ("G-5", 2), (".", 2), ("E-5", 2), ("G-5", 4), ("A-5", 2), ("G-5", 2)],
        [("D-5", 2), ("G-5", 2), (".", 2), ("B-4", 2), ("D-5", 6), (".", 2)],
        [("E-5", 2), ("A-5", 2), (".", 2), ("C-6", 2), ("B-5", 2), ("A-5", 2), ("G-5", 2), ("E-5", 2)],
        [("F-5", 4), ("E-5", 2), ("D-5", 2), ("C-5", 4), ("D-5", 4)],
    ]
    m_chorus = [
        [("A-5", 4), ("G-5", 2), ("F-5", 2), ("A-5", 4), ("C-6", 4)],
        [("B-5", 4), ("A-5", 2), ("G-5", 2), ("D-5", 4), ("G-5", 4)],
        [("E-5", 2), ("G-5", 2), ("C-6", 4), ("B-5", 2), ("C-6", 2), ("G-5", 4)],
        [("A-5", 6), ("E-5", 2), ("A-5", 4), ("B-5", 4)],
        [("C-6", 4), ("A-5", 2), ("F-5", 2), ("D-5", 4), ("F-5", 4)],
        [("G-5", 4), ("F-5", 2), ("D-5", 2), ("B-4", 4), ("D-5", 4)],
        [("C-5", 2), ("E-5", 2), ("G-5", 2), ("C-6", 2), ("E-6", 4), ("D-6", 4)],
        [("C-6", 8), (".", 4), ("G-5", 2), ("B-5", 2)],
    ]

    def bars(chords, melodies, lead=True, drums=2):
        p = it.Pattern(16 * len(chords))
        for i, ch in enumerate(chords):
            at = 16 * i
            _bar(p, at, ch, drums=drums)
            if lead:
                put(p, 0, LEAD, seq(melodies[i], 46), at)
        return p

    intro = bars(verse, m_verse, lead=False, drums=1)
    intro.set(0, 6, fx=it.cmd("A", 5))
    intro.set(0, 7, fx=it.cmd("T", 128))
    pats = [intro, bars(verse, m_verse), bars(chorus[:4], m_chorus[:4]), bars(chorus[4:], m_chorus[4:])]
    samples = [f() for f in SET]
    message = "[[SNESMOD]]\nedl 3\nefb 32\nevol 22\nefir 127 0 0 0 0 0 0 0\neon 1 2\n"
    return dict(title="Daveo Rooftops", samples=samples, patterns=pats,
                orders=[0, 1, 1, 2, 3, 1, 2, 3], speed=5, tempo=128,
                message=message, pan=[34, 24, 32, 32, 32, 42, 32, 32])


def neo_lumen_sfx():
    """The effects of snesmod_music: four Kenney sounds (CC0) as the samples
    of a module that is only an instrument list. Cut to fit beside the
    nocturne and its echo."""
    import kenney
    rate = 16000
    picks = [("jump", "phaseJump1", None), ("powerup", "powerUp1", 0.8),
             ("laser", "laser1", 0.5), ("confirm", "confirmation_001", None)]
    samples = [it.Sample(n, sy.to16(kenney.load(f, rate, sec)), rate, env=[(0, 64), (2, 64)])
               for n, f, sec in picks]
    p = it.Pattern(16)
    for i in range(len(samples)):
        p.set(4 * i, 0, note=it.note("C-5"), smp=i + 1)
    return dict(title="Neo-Lumen effects", samples=samples, patterns=[p], orders=[0])


def rooftops_sfx():
    """rooftop_run's effects (Kenney, CC0): Kit's jump, a shard picked up, a machine
    stomped, Kit hurt, the roof cleared. Each sample's name gives its SFX_ name in
    soundbank.h (SFX_JUMP, SFX_SHARD, SFX_STOMP, SFX_HURT, SFX_CLEAR), the names
    the game uses; they are cut short to leave the music its SPC700 RAM."""
    import kenney
    rate = 16000
    samples = [it.Sample("jump", sy.to16(kenney.load("phaseJump1", rate)), rate),
               it.Sample("shard", sy.to16(kenney.load("pepSound1", rate, seconds=0.25)), rate),
               it.Sample("stomp", sy.to16(kenney.load("zapTwoTone", rate, seconds=0.2)), rate),
               it.Sample("hurt", sy.to16(kenney.load("error_004", rate, seconds=0.35)), rate),
               it.Sample("clear", sy.to16(kenney.load("powerUp1", rate, seconds=0.6)), rate)]
    p = it.Pattern(4)
    p.set(0, 0, note=it.note("C-5"), smp=1)
    return dict(title="Neo-Lumen rooftop effects", samples=samples, patterns=[p], orders=[0])


def nocturne_large():
    """The nocturne with long, 32 kHz instruments and no echo: a soundbank
    over 32 KB, so the test fixture checks a module that spans two ROM
    banks (it replaced a cover of a song we had no right to)."""
    import instruments as ins2
    d = nocturne()
    big = [ins2.lead(), ins2.pad(), ins2.bass(),
           it.Sample("pluck", sy.to16(sy.pluck(ins2.C5 / 2, 32000, 1.0, damping=0.998)),
                     32000 * ins2.C5 / sy.pluck_pitch(ins2.C5 / 2, 32000), volume=48),
           it.Sample("bell", sy.to16(sy.fm_bell(ins2.C5, 32000, 1.0, decay=2.0)), 32000, volume=40),
           ins2.kick(32000), ins2.snare(32000), ins2.hat(32000)]
    d.update(title="Nocturne (large)", samples=big, message="")
    return d
