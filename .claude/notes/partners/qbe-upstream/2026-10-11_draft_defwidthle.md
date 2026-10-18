# Draft, NOT SENT: a wrong width proof in QBE since e01207f

**2026-10-13: superseded by `2026-10-13_to_qbe_defwidthle.txt`**, the text
ready to send (owner's word that day: send it, in natural and kind
language, no glyphs, no emoticons). QBE has no issue tracker: its page
(c9x.me/compile) sends patches and discussions to the mailing list
`~mpu/qbe@lists.sr.ht`, plain text. Every claim was re-run that day on a
fresh clone of upstream master (`e786f06`, still the tip, no change to
`copy.c` since): the reproducer returns 0x1234 there and 0x34 at
`e01207f~1`; with the mark cleared it returns 0x34 and `make check` says
"All is fine!". NOT SENT by this session: it has no mail tool, and the
sending address is the owner's choice.

**Final, 2026-10-13: not sent, by the owner's decision** — we keep our
fork, take their changes, and do not contribute upstream (`README.md` of
this folder). The text and the reproducer stay as the record.

Status: draft. QBE takes reports by mail (`~mpu/qbe@lists.sr.ht`). Sending
is a public act under the maintainer's name and waits for the owner's word.
Evidence and the table of runs: `.claude/notes/tech/qbe_upstream_review.md`.

---

Subject: defwidthle() keeps the mark of a phi whose proof failed (since e01207f)

Hello,

Since e01207f ("fix exponential behavior of defwidthle()"), a phi visited
by dwl() keeps `visit = w` until the whole query ends, also when its proof
returned 0. A later path of the same query that reaches that phi then
takes it for proven. With `and` (either operand narrow is enough) this
removes an extension that is needed:

    export function w $f(w %a, w %c) {
    @start
    	jnz %c, @l, @r
    @l
    	jmp @j
    @r
    	jmp @j
    @j
    	%p =w phi @l %a, @r 1
    	jnz %c, @m, @n
    @m
    	jmp @k
    @n
    	jmp @k
    @k
    	%s =w phi @m %p, @n 3
    	%q =w and %p, %s
    	%z =w extub %q
    	ret %z
    }

f(0x1234, 1) returns 0x1234 at e01207f and at master (e786f06); it returns
0x34 at e01207f~1. Seen on arm64; `-dG` shows `retw %q` after GVN, the
extub gone.

dwl(%q, 8) tries %p first: %p is marked, its argument %a fails, dwl returns
0 and the mark stays. It then tries %s: its argument %p is "visited at a
width <= 8", so yes; 3 fits; %s is narrow, hence %q.

Clearing the mark of the phi whose proof fails restores the result here
(checked on the tip; `make check` still passes). It is probably not
enough in general: a phi proven narrow while an outer phi was only
assumed narrow keeps its mark when that outer proof fails later. I have
not measured what a complete fix does to the case e01207f was written
for.

Thanks for QBE.
