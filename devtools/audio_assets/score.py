"""A small notation for the songs: one token per row.

    "A-4 . C-5 . E-5 ^ . ."

`A-4` plays a note, `.` leaves the row empty, `^` is note off, `=` note cut.
A token may carry a volume after a colon (`A-4:40`) and an effect after a
slash (`A-4/H24`: vibrato speed 2, depth 4; `./A04`: speed 4). Bars are
written on several lines; whitespace is free.
"""
import itwriter as it


def parse(text):
    return text.split()


def put(pattern, ch, smp, text, start=0, vol=None):
    """Write one track's tokens into channel `ch` of `pattern` from `start`."""
    for i, tok in enumerate(parse(text)):
        row = start + i
        fx = None
        if "/" in tok:
            tok, f = tok.split("/")
            fx = it.cmd(f[0], int(f[1:], 16))
        v = vol
        if ":" in tok:
            tok, vs = tok.split(":")
            v = int(vs)
        if tok == ".":
            if fx:
                pattern.set(row, ch, fx=fx)
            continue
        if tok == "^":
            pattern.set(row, ch, note=it.NOTE_OFF)
            continue
        if tok == "=":
            pattern.set(row, ch, note=it.NOTE_CUT)
            continue
        pattern.set(row, ch, note=it.note(tok), smp=smp, vol=v, fx=fx)


def repeat(text, n):
    return " ".join([text] * n)
