#!/usr/bin/env python3
"""Generate the paper/hand hybrids of path. flags: P E F B C (see FLAGS)."""
import sys
HEAD = """.SECTION ".gap_pathpp" SUPERFREE
.ACCU 16
.INDEX 16
.DEFINE ACC0L   tcc__lf+0
.DEFINE ACC0H   tcc__lf+2
.DEFINE ACC1L   tcc__lf+4
.DEFINE ACC1H   tcc__lf+6
.DEFINE ACC2L   tcc__lf+8
.DEFINE ACC2H   tcc__lf+10
.DEFINE ACC3L   tcc__lf+12
.DEFINE ACC3H   tcc__lf+14
.DEFINE STEP    tcc__lf+16
.DEFINE IDX     tcc__lf+18
.DEFINE S       tcc__lf+20
.DEFINE M       tcc__lf+22
.DEFINE TL      tcc__lf+24
.DEFINE TH      tcc__lf+26
k_pathpp_asm:
    rep #$20
    .ACCU 16
    .INDEX 16
"""
def gen(P="paper", E="paper", F="paper", B="slot1", C="paper"):
    o = [HEAD]
    # P: prologue composition of the four 32-bit values
    for k in range(4):
        off = "" if k == 0 else f"+{2*k}"
        if P == "paper":
            o.append(f"""    lda.w pa_lo{off}
    sta.b ACC{k}L
    bmi @n{k}
    lda.w pa_hi{off}
    bra @s{k}
@n{k}:
    lda #$FFFF
@s{k}:
    sta.b ACC{k}H
""")
        else:
            o.append(f"""    lda.w pa_lo{off}
    sta.b ACC{k}L
    lda.w pa_hi{off}
    sta.b ACC{k}H
""")
    # B: index/step prologue
    if B == "slot1":
        o.append("    lda.w pa_step\n    sta.b STEP\n    lda.w pa_idx\n    sta.b IDX\n")
    elif B == "slot2":
        o.append("    lda.w pa_step\n    asl a\n    sta.b STEP\n    lda.w pa_idx\n    asl a\n    sta.b IDX\n")
    else:
        o.append("    lda.w pa_step\n    asl a\n    sta.b STEP\n    lda.w pa_idx\n    asl a\n    tax\n")
    # C: loop entry
    if C == "paper":
        o.append("    ldy #$0000\n@for_cond:\n    cpy #$0040\n    bcs @for_join\n@for_body:\n")
    elif C == "rot":
        o.append("    ldy #$0000\n@for_body:\n")
    else:
        o.append("    ldy #$0040\n@for_body:\n")
    # index head
    if B == "slot1":
        o.append("    lda.b IDX\n    asl a\n    tax\n")
    elif B == "slot2":
        o.append("    lda.b IDX\n    tax\n")
    o.append("""    lda.l path_sin,x
    sta.b S
    cmp #$8000
    lda #$0000
    sbc #$0000
    eor #$FFFF
    sta.b M
""")
    # E: acc3 += s sar 1
    if E == "paper":
        o.append("""    cmp #$8000
    ror a
    sta.b TH
    lda.b S
    ror a
    sta.b TL
    clc
    lda.b ACC3L
    adc.b TL
    sta.b ACC3L
    lda.b ACC3H
    adc.b TH
    sta.b ACC3H
""")
    else:
        o.append("""    lda.b S
    cmp #$8000
    ror a
    clc
    adc.b ACC3L
    sta.b ACC3L
    lda.b ACC3H
    adc.b M
    sta.b ACC3H
""")
    # F: acc2 += s*2
    if F == "paper":
        o.append("""    lda.b S
    asl a
    sta.b TL
    lda.b M
    rol a
    sta.b TH
    clc
    lda.b ACC2L
    adc.b TL
    sta.b ACC2L
    lda.b ACC2H
    adc.b TH
    sta.b ACC2H
""")
    else:
        o.append("""    lda.b S
    asl a
    clc
    adc.b ACC2L
    sta.b ACC2L
    lda.b ACC2H
    adc.b M
    sta.b ACC2H
""")
    o.append("""    clc
    lda.b ACC0L
    adc.b S
    sta.b ACC0L
    lda.b ACC0H
    adc.b M
    sta.b ACC0H
    sec
    lda.b ACC1L
    sbc.b S
    sta.b ACC1L
    lda.b ACC1H
    sbc.b M
    sta.b ACC1H
""")
    # index update
    if B == "slot1":
        o.append("    lda.b STEP\n    clc\n    adc.b IDX\n    and #$003F\n    sta.b IDX\n")
    elif B == "slot2":
        o.append("    lda.b STEP\n    clc\n    adc.b IDX\n    and #$007E\n    sta.b IDX\n")
    else:
        o.append("    txa\n    clc\n    adc.b STEP\n    and #$007E\n    tax\n")
    # loop tail
    if C == "paper":
        o.append("    iny\n    bra @for_cond\n@for_join:\n")
    elif C == "rot":
        o.append("    iny\n    cpy #$0040\n    bcc @for_body\n")
    else:
        o.append("    dey\n    bne @for_body\n")
    # epilogue
    for k in range(4):
        off = "" if k == 0 else f"+{2*k}"
        o.append(f"    lda.b ACC{k}H\n    sta.w pa_hi{off}\n    lda.b ACC{k}L\n    sta.w pa_lo{off}\n")
    if B == "slot1":
        o.append("    lda.b IDX\n    sta.w pa_idx\n")
    elif B == "slot2":
        o.append("    lda.b IDX\n    lsr a\n    sta.w pa_idx\n")
    else:
        o.append("    txa\n    lsr a\n    sta.w pa_idx\n")
    o.append("    rtl\n.ENDS\n")
    return "".join(o)

PAPER = dict(P="paper", E="paper", F="paper", B="slot1", C="paper")
HAND = dict(P="hand", E="hand", F="hand", B="X", C="down")
if __name__ == "__main__":
    import os
    d = os.path.dirname(os.path.abspath(__file__))
    V = {}
    # forward cumulative, paper -> hand
    steps = [("P", "hand"), ("E", "hand"), ("F", "hand"), ("B", "slot2"), ("C", "rot"), ("C", "down"), ("B", "X")]
    cur = dict(PAPER); V["f0_paper"] = dict(cur)
    for i, (k, v) in enumerate(steps, 1):
        cur[k] = v; V[f"f{i}_{k}_{v}"] = dict(cur)
    # reverse: from the hand, regress one post at a time (single), plus cumulative reverse
    rev = [("P", "paper"), ("E", "paper"), ("F", "paper"), ("B", "slot2"), ("B", "slot1"), ("C", "rot"), ("C", "paper")]
    for k, v in rev:
        c = dict(HAND); c[k] = v; V[f"r_{k}_{v}"] = c
    # reverse cumulative, opposite order (hand -> paper): B, C, F, E, P
    c = dict(HAND); V["c0_hand"] = dict(c)
    for i, (k, v) in enumerate([("B", "slot2"), ("C", "rot"), ("C", "paper"), ("B", "slot1"), ("F", "paper"), ("E", "paper"), ("P", "paper")], 1):
        c[k] = v; V[f"c{i}_{k}_{v}"] = dict(c)
    for name, fl in V.items():
        open(os.path.join(d, name + ".asm"), "w").write(f"; flags {fl}\n" + gen(**fl))
    print(len(V), "variants")
