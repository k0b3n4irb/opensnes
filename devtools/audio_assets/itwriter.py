"""Write Impulse Tracker (.it) modules for SNESMOD.

The subset `opensnes-music` reads (tools/lib/smconv/src/itloader.c): one
instrument per sample, its keyboard mapping every note to that sample and
a volume envelope (SNESMOD plays nothing of a module in sample mode: it
reads the instrument column as an instrument, and there are none; and an
instrument without a volume envelope is cut after a few frames, where
libopenmpt holds it — measured on luna), 16-bit signed mono
samples, optional loops, patterns of up to 200 rows, and the song message
whose `[[SNESMOD]]` block carries the echo settings (`edl`, `efb`, `evol`,
`efir`, `eon`). Layout from Jeffrey Lim's ITTECH.TXT.
"""
import struct

NOTE_OFF = 255
NOTE_CUT = 254

_NAMES = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6,
          "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}


def note(name):
    """'C-5' or 'F#3' to an IT note number (C-5 = 60)."""
    name = name.replace("-", "")
    return 12 * int(name[-1]) + _NAMES[name[:-1]]


def cmd(letter, value):
    """An effect column entry: ('A', 6) is set-speed 6."""
    return (ord(letter.upper()) - ord("A") + 1, value)


class Sample:
    """A 16-bit signed mono sample: `data` is a list of ints in -32768..32767
    (the converter makes BRR from 16 bits; 8 would add their noise first)."""

    def __init__(self, name, data, c5speed, loop=None, volume=64, env=None, sustain=1):
        self.name = name
        self.data = data
        self.c5speed = int(c5speed)
        self.loop = loop            # (start, end) in samples, or None
        self.volume = volume
        # volume envelope: [(tick, level 0..64), ...]; held at node `sustain`
        # until note off (None: no sustain). Default: flat, held.
        self.env = env or [(0, 64), (2, 64)]
        self.sustain = sustain


class Pattern:
    """`rows` rows of 8 channels; set(row, ch, note=, smp=, vol=, fx=)."""

    def __init__(self, rows=64, channels=8):
        self.rows = rows
        self.channels = channels
        self.cells = {}

    def set(self, row, ch, note=None, smp=None, vol=None, fx=None):
        cell = self.cells.setdefault((row, ch), {})
        if note is not None:
            cell["note"] = note
        if smp is not None:
            cell["smp"] = smp
        if vol is not None:
            cell["vol"] = vol
        if fx is not None:
            cell["fx"] = fx

    def pack(self):
        out = bytearray()
        for r in range(self.rows):
            for c in range(self.channels):
                cell = self.cells.get((r, c))
                if not cell:
                    continue
                mask = 0
                body = bytearray()
                if "note" in cell:
                    mask |= 1
                    body.append(cell["note"])
                if "smp" in cell:
                    mask |= 2
                    body.append(cell["smp"])
                if "vol" in cell:
                    mask |= 4
                    body.append(cell["vol"])
                if "fx" in cell:
                    mask |= 8
                    body += bytes(cell["fx"])
                out.append((c + 1) | 0x80)
                out.append(mask)
                out += body
            out.append(0)
        return bytes(struct.pack("<HH4x", len(out), self.rows)) + bytes(out)


def _instrument(i, s, fadeout):
    h = bytearray(554)
    h[0:4] = b"IMPI"
    h[0x11] = 3                  # NNA: note fade
    struct.pack_into("<H", h, 0x14, fadeout)
    h[0x18] = 128                # global volume
    h[0x19] = 32 | 128           # default pan unused
    struct.pack_into("<H", h, 0x1C, 0x0214)
    h[0x1E] = 1
    h[0x20:0x20 + 26] = s.name.encode("ascii")[:25].ljust(26, b"\0")
    h[0x3A] = 0x7F               # no filter
    for n in range(120):
        h[0x40 + 2 * n] = n
        h[0x41 + 2 * n] = i + 1
    e = 0x130                    # volume envelope; panning and pitch stay off
    flags = 0x01 | (0x04 if s.sustain is not None else 0)
    h[e] = flags
    h[e + 1] = len(s.env)
    if s.sustain is not None:
        h[e + 4] = h[e + 5] = s.sustain
    for k, (tick, level) in enumerate(s.env):
        h[e + 6 + 3 * k] = level
        struct.pack_into("<H", h, e + 7 + 3 * k, tick)
    return bytes(h)


def write(path, title, samples, patterns, orders, speed=6, tempo=125,
          message="", global_volume=128, mix_volume=48, pan=None, fadeout=256):
    """Write the module. `orders` lists pattern indices; `pan` is a list of
    eight 0..64 channel pans (32 = centre)."""
    pan = pan or [32] * 8
    msg = message.replace("\n", "\r").encode("ascii") + b"\0" if message else b""
    head = bytearray(0xC0)
    head[0:4] = b"IMPM"
    head[4:4 + 26] = title.encode("ascii")[:25].ljust(26, b"\0")
    ordlist = bytes(orders) + bytes([255])
    n = len(samples)
    struct.pack_into("<HHHH", head, 0x20, len(ordlist), n, n, len(patterns))
    struct.pack_into("<HHHH", head, 0x28, 0x0214, 0x0214, 0x01 | 0x04 | 0x08, 1 if msg else 0)
    struct.pack_into("<BBBBBB", head, 0x30, global_volume, mix_volume, speed, tempo, 128, 0)
    for i in range(64):
        head[0x40 + i] = pan[i] if i < len(pan) else 32 | 128
        head[0x80 + i] = 64
    table_at = 0xC0 + len(ordlist)
    offs_size = 4 * (2 * n + len(patterns))
    cursor = table_at + offs_size
    msg_at = cursor
    cursor += len(msg)
    ins_at = []
    for _ in samples:
        ins_at.append(cursor)
        cursor += 554
    smp_hdr_at = []
    for _ in samples:
        smp_hdr_at.append(cursor)
        cursor += 0x50
    pat_at = []
    packed = [p.pack() for p in patterns]
    for p in packed:
        pat_at.append(cursor)
        cursor += len(p)
    data_at = []
    for s in samples:
        data_at.append(cursor)
        cursor += 2 * len(s.data)
    if msg:
        struct.pack_into("<HI", head, 0x36, len(msg), msg_at)
    out = bytearray(head) + ordlist
    for a in ins_at + smp_hdr_at + pat_at:
        out += struct.pack("<I", a)
    out += msg
    for i, s in enumerate(samples):
        out += _instrument(i, s, fadeout)
    for s, at in zip(samples, data_at):
        h = bytearray(0x50)
        h[0:4] = b"IMPS"
        h[0x11] = 64
        flags = 0x01 | 0x02 | (0x10 if s.loop else 0)
        h[0x12] = flags
        h[0x13] = s.volume
        h[0x14:0x14 + 26] = s.name.encode("ascii")[:25].ljust(26, b"\0")
        h[0x2E] = 0x01           # signed samples
        h[0x2F] = 32
        lb, le = s.loop if s.loop else (0, 0)
        struct.pack_into("<IIIIIII", h, 0x30, len(s.data), lb, le, s.c5speed, 0, 0, at)
        out += h
    for p in packed:
        out += p
    for s in samples:
        out += struct.pack("<%dh" % len(s.data), *s.data)
    with open(path, "wb") as f:
        f.write(out)
