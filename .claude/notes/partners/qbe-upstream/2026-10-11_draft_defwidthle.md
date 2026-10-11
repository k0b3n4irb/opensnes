# Draft, NOT SENT: a wrong width proof in QBE since e01207f

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

Clearing the mark when a proof fails (keeping it only for phis proven
narrow) restores the result here; I have not measured what that does to
the case e01207f was written for.

Thanks for QBE.
