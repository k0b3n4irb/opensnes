#include "all.h"
#include <limits.h>

/* For x86_64, do the following:
 *
 * - check that constants are used only in
 *   places allowed
 * - ensure immediates always fit in 32b
 * - expose machine register contraints
 *   on instructions like division.
 * - implement fast locals (the streak of
 *   constant allocX in the first basic block)
 * - recognize complex addressing modes
 *
 * Invariant: the use counts that are used
 *            in sel() must be sound.  This
 *            is not so trivial, maybe the
 *            dce should be moved out...
 */

static int amatch(Addr *, Num *, Ref, Fn *);

static int
noimm(Ref r, Fn *fn)
{
	int64_t val;

	if (rtype(r) != RCon)
		return 0;
	switch (fn->con[r.val].type) {
	case CAddr:
		/* we only support the 'small'
		 * code model of the ABI, this
		 * means that we can always
		 * address data with 32bits
		 */
		return 0;
	case CBits:
		val = fn->con[r.val].bits.i;
		return (val < INT32_MIN || val > INT32_MAX);
	default:
		die("invalid constant");
	}
}

static int
rslot(Ref r, Fn *fn)
{
	if (rtype(r) != RTmp)
		return -1;
	return fn->tmp[r.val].slot;
}

static int
hascon(Ref r, Con **pc, Fn *fn)
{
	switch (rtype(r)) {
	case RCon:
		*pc = &fn->con[r.val];
		return 1;
	case RMem:
		*pc = &fn->mem[r.val].offset;
		return 1;
	default:
		return 0;
	}
}

static void
fixarg(Ref *r, int k, Ins *i, Fn *fn)
{
	char buf[32];
	Addr a, *m;
	Con cc, *c;
	Ref r0, r1, r2, r3;
	int s, n, op;

	r1 = r0 = *r;
	s = rslot(r0, fn);
	op = i ? i->op : Ocopy;
	if (KBASE(k) == 1 && rtype(r0) == RCon) {
		/* load floating points from memory
		 * slots, they can't be used as
		 * immediates
		 */
		r1 = MEM(fn->nmem);
		vgrow(&fn->mem, ++fn->nmem);
		memset(&a, 0, sizeof a);
		a.offset.type = CAddr;
		n = stashbits(fn->con[r0.val].bits.i, KWIDE(k) ? 8 : 4);
		/* quote the name so that we do not
		 * add symbol prefixes on the apple
		 * target variant
		 */
		sprintf(buf, "\"%sfp%d\"", T.asloc, n);
		a.offset.sym.id = intern(buf);
		fn->mem[fn->nmem-1] = a;
	}
	else if (op != Ocopy && k == Kl && noimm(r0, fn)) {
		/* load constants that do not fit in
		 * a 32bit signed integer into a
		 * long temporary
		 */
		r1 = newtmp("isel", Kl, fn);
		emit(Ocopy, Kl, r1, r0, R);
	}
	else if (s != -1) {
		/* load fast locals' addresses into
		 * temporaries right before the
		 * instruction
		 */
		r1 = newtmp("isel", Kl, fn);
		emit(Oaddr, Kl, r1, SLOT(s), R);
	}
	else if (T.apple && hascon(r0, &c, fn)
	&& c->type == CAddr && c->sym.type == SThr) {
		r1 = newtmp("isel", Kl, fn);
		if (c->bits.i) {
			r2 = newtmp("isel", Kl, fn);
			cc = (Con){.type = CBits};
			cc.bits.i = c->bits.i;
			r3 = newcon(&cc, fn);
			emit(Oadd, Kl, r1, r2, r3);
		} else
			r2 = r1;
		emit(Ocopy, Kl, r2, TMP(RAX), R);
		r2 = newtmp("isel", Kl, fn);
		r3 = newtmp("isel", Kl, fn);
		emit(Ocall, 0, R, r3, CALL(17));
		emit(Ocopy, Kl, TMP(RDI), r2, R);
		emit(Oload, Kl, r3, r2, R);
		cc = *c;
		cc.bits.i = 0;
		r3 = newcon(&cc, fn);
		emit(Oload, Kl, r2, r3, R);
		if (rtype(r0) == RMem) {
			m = &fn->mem[r0.val];
			m->offset.type = CUndef;
			m->base = r1;
			r1 = r0;
		}
	}
	else if (!(isstore(op) && r == &i->arg[1])
	&& !isload(op) && op != Ocall && rtype(r0) == RCon
	&& fn->con[r0.val].type == CAddr) {
		/* apple as does not support 32-bit
		 * absolute addressing, use a rip-
		 * relative leaq instead
		 */
		r1 = newtmp("isel", Kl, fn);
		emit(Oaddr, Kl, r1, r0, R);
	}
	else if (rtype(r0) == RMem) {
		/* eliminate memory operands of
		 * the form $foo(%rip, ...)
		 */
		m = &fn->mem[r0.val];
		if (req(m->base, R))
		if (m->offset.type == CAddr) {
			r0 = newtmp("isel", Kl, fn);
			emit(Oaddr, Kl, r0, newcon(&m->offset, fn), R);
			m->offset.type = CUndef;
			m->base = r0;
		}
	}
	*r = r1;
}

static void
seladdr(Ref *r, Num *tn, Fn *fn)
{
	Addr a;
	Ref r0;

	r0 = *r;
	if (rtype(r0) == RTmp) {
		memset(&a, 0, sizeof a);
		if (!amatch(&a, tn, r0, fn))
			return;
		if (!req(a.base, R))
		if (a.offset.type == CAddr) {
			/* apple as does not support
			 * $foo(%r0, %r1, M); try to
			 * rewrite it or bail out if
			 * impossible
			 */
			if (!req(a.index, R) || rtype(a.base) != RTmp)
				return;
			else {
				a.index = a.base;
				a.scale = 1;
				a.base = R;
			}
		}
		chuse(r0, -1, fn);
		vgrow(&fn->mem, ++fn->nmem);
		fn->mem[fn->nmem-1] = a;
		chuse(a.base, +1, fn);
		chuse(a.index, +1, fn);
		*r = MEM(fn->nmem-1);
	}
}

static int
cmpswap(Ref arg[2], int op)
{
	switch (op) {
	case NCmpI+Cflt:
	case NCmpI+Cfle:
		return 1;
	case NCmpI+Cfgt:
	case NCmpI+Cfge:
		return 0;
	}
	return rtype(arg[0]) == RCon;
}

static void
selcmp(Ref arg[2], int k, int swap, Fn *fn)
{
	Ref r;
	Ins *icmp;

	if (swap) {
		r = arg[1];
		arg[1] = arg[0];
		arg[0] = r;
	}
	emit(Oxcmp, k, R, arg[1], arg[0]);
	icmp = curi;
	if (rtype(arg[0]) == RCon) {
		assert(k != Kw);
		icmp->arg[1] = newtmp("isel", k, fn);
		emit(Ocopy, k, icmp->arg[1], arg[0], R);
		fixarg(&curi->arg[0], k, curi, fn);
	}
	fixarg(&icmp->arg[0], k, icmp, fn);
	fixarg(&icmp->arg[1], k, icmp, fn);
}

static void
sel(Ins i, Num *tn, Fn *fn)
{
	Ref r0, r1, tmp[7];
	int x, j, k, kc, sh, swap;
	Ins *i0, *i1;

	if (rtype(i.to) == RTmp)
	if (!isreg(i.to) && !isreg(i.arg[0]) && !isreg(i.arg[1]))
	if (fn->tmp[i.to.val].nuse == 0) {
		chuse(i.arg[0], -1, fn);
		chuse(i.arg[1], -1, fn);
		return;
	}
	i0 = curi;
	k = i.cls;
	switch (i.op) {
	case Odiv:
	case Orem:
	case Oudiv:
	case Ourem:
		if (KBASE(k) == 1)
			goto Emit;
		if (i.op == Odiv || i.op == Oudiv)
			r0 = TMP(RAX), r1 = TMP(RDX);
		else
			r0 = TMP(RDX), r1 = TMP(RAX);
		emit(Ocopy, k, i.to, r0, R);
		emit(Ocopy, k, R, r1, R);
		if (rtype(i.arg[1]) == RCon) {
			/* immediates not allowed for
			 * divisions in x86
			 */
			r0 = newtmp("isel", k, fn);
		} else
			r0 = i.arg[1];
		if (fn->tmp[r0.val].slot != -1)
			err("unlikely argument %%%s in %s",
				fn->tmp[r0.val].name, optab[i.op].name);
		if (i.op == Odiv || i.op == Orem) {
			emit(Oxidiv, k, R, r0, R);
			emit(Osign, k, TMP(RDX), TMP(RAX), R);
		} else {
			emit(Oxdiv, k, R, r0, R);
			emit(Ocopy, k, TMP(RDX), CON_Z, R);
		}
		emit(Ocopy, k, TMP(RAX), i.arg[0], R);
		fixarg(&curi->arg[0], k, curi, fn);
		if (rtype(i.arg[1]) == RCon)
			emit(Ocopy, k, r0, i.arg[1], R);
		break;
	case Osar:
	case Oshr:
	case Oshl:
		r0 = i.arg[1];
		if (rtype(r0) == RCon)
			goto Emit;
		if (fn->tmp[r0.val].slot != -1)
			err("unlikely argument %%%s in %s",
				fn->tmp[r0.val].name, optab[i.op].name);
		i.arg[1] = TMP(RCX);
		emit(Ocopy, Kw, R, TMP(RCX), R);
		emiti(i);
		i1 = curi;
		emit(Ocopy, Kw, TMP(RCX), r0, R);
		fixarg(&i1->arg[0], argcls(&i, 0), i1, fn);
		break;
	case Ouwtof:
		r0 = newtmp("utof", Kl, fn);
		emit(Osltof, k, i.to, r0, R);
		emit(Oextuw, Kl, r0, i.arg[0], R);
		fixarg(&curi->arg[0], k, curi, fn);
		break;
	case Oultof:
		/* %mask =l and %arg.0, 1
		 * %isbig =l shr %arg.0, 63
		 * %divided =l shr %arg.0, %isbig
		 * %or =l or %mask, %divided
		 * %float =d sltof %or
		 * %cast =l cast %float
		 * %addend =l shl %isbig, 52
		 * %sum =l add %cast, %addend
		 * %result =d cast %sum
		 */
		r0 = newtmp("utof", k, fn);
		if (k == Ks)
			kc = Kw, sh = 23;
		else
			kc = Kl, sh = 52;
		for (j=0; j<4; j++)
			tmp[j] = newtmp("utof", Kl, fn);
		for (; j<7; j++)
			tmp[j] = newtmp("utof", kc, fn);
		emit(Ocast, k, i.to, tmp[6], R);
		emit(Oadd, kc, tmp[6], tmp[4], tmp[5]);
		emit(Oshl, kc, tmp[5], tmp[1], getcon(sh, fn));
		emit(Ocast, kc, tmp[4], r0, R);
		emit(Osltof, k, r0, tmp[3], R);
		emit(Oor, Kl, tmp[3], tmp[0], tmp[2]);
		emit(Oshr, Kl, tmp[2], i.arg[0], tmp[1]);
		sel(*curi++, 0, fn);
		emit(Oshr, Kl, tmp[1], i.arg[0], getcon(63, fn));
		fixarg(&curi->arg[0], Kl, curi, fn);
		emit(Oand, Kl, tmp[0], i.arg[0], getcon(1, fn));
		fixarg(&curi->arg[0], Kl, curi, fn);
		break;
	case Ostoui:
		i.op = Ostosi;
		kc = Ks;
		tmp[4] = getcon(0xdf000000, fn);
		goto Oftoui;
	case Odtoui:
		i.op = Odtosi;
		kc = Kd;
		tmp[4] = getcon(0xc3e0000000000000, fn);
	Oftoui:
		if (k == Kw) {
			r0 = newtmp("ftou", Kl, fn);
			emit(Ocopy, Kw, i.to, r0, R);
			i.cls = Kl;
			i.to = r0;
			goto Emit;
		}
		/* %try0 =l {s,d}tosi %fp
		 * %mask =l sar %try0, 63
		 *
		 *    mask is all ones if the first
		 *    try was oob, all zeroes o.w.
		 *
		 * %fps ={s,d} sub %fp, (1<<63)
		 * %try1 =l {s,d}tosi %fps
		 *
		 * %tmp =l and %mask, %try1
		 * %res =l or %tmp, %try0
		 */
		r0 = newtmp("ftou", kc, fn);
		for (j=0; j<4; j++)
			tmp[j] = newtmp("ftou", Kl, fn);
		emit(Oor, Kl, i.to, tmp[0], tmp[3]);
		emit(Oand, Kl, tmp[3], tmp[2], tmp[1]);
		emit(i.op, Kl, tmp[2], r0, R);
		emit(Oadd, kc, r0, tmp[4], i.arg[0]);
		i1 = curi; /* fixarg() can change curi */
		fixarg(&i1->arg[0], kc, i1, fn);
		fixarg(&i1->arg[1], kc, i1, fn);
		emit(Osar, Kl, tmp[1], tmp[0], getcon(63, fn));
		emit(i.op, Kl, tmp[0], i.arg[0], R);
		fixarg(&curi->arg[0], Kl, curi, fn);
		break;
	case Onop:
		break;
	case Ostored:
	case Ostores:
	case Ostorel:
	case Ostorew:
	case Ostoreh:
	case Ostoreb:
		if (rtype(i.arg[0]) == RCon) {
			if (i.op == Ostored)
				i.op = Ostorel;
			if (i.op == Ostores)
				i.op = Ostorew;
		}
		seladdr(&i.arg[1], tn, fn);
		goto Emit;
	case_Oload:
		seladdr(&i.arg[0], tn, fn);
		goto Emit;
	case Odbgloc:
	case Ocall:
	case Osalloc:
	case Ocopy:
	case Oadd:
	case Osub:
	case Oneg:
	case Omul:
	case Oand:
	case Oor:
	case Oxor:
	case Oxtest:
	case Ostosi:
	case Odtosi:
	case Oswtof:
	case Osltof:
	case Oexts:
	case Otruncd:
	case Ocast:
	case_OExt:
Emit:
		emiti(i);
		i1 = curi; /* fixarg() can change curi */
		fixarg(&i1->arg[0], argcls(&i, 0), i1, fn);
		fixarg(&i1->arg[1], argcls(&i, 1), i1, fn);
		break;
	case Oalloc4:
	case Oalloc8:
	case Oalloc16:
		salloc(i.to, i.arg[0], fn);
		break;
	default:
		if (isext(i.op))
			goto case_OExt;
		if (isload(i.op))
			goto case_Oload;
		if (iscmp(i.op, &kc, &x)) {
			switch (x) {
			case NCmpI+Cfeq:
				/* zf is set when operands are
				 * unordered, so we may have to
				 * check pf
				 */
				r0 = newtmp("isel", Kw, fn);
				r1 = newtmp("isel", Kw, fn);
				emit(Oand, Kw, i.to, r0, r1);
				emit(Oflagfo, k, r1, R, R);
				i.to = r0;
				break;
			case NCmpI+Cfne:
				r0 = newtmp("isel", Kw, fn);
				r1 = newtmp("isel", Kw, fn);
				emit(Oor, Kw, i.to, r0, r1);
				emit(Oflagfuo, k, r1, R, R);
				i.to = r0;
				break;
			}
			swap = cmpswap(i.arg, x);
			if (swap)
				x = cmpop(x);
			emit(Oflag+x, k, i.to, R, R);
			selcmp(i.arg, kc, swap, fn);
			break;
		}
		die("unknown instruction %s", optab[i.op].name);
	}

	while (i0>curi && --i0) {
		assert(rslot(i0->arg[0], fn) == -1);
		assert(rslot(i0->arg[1], fn) == -1);
	}
}

static Ins *
flagi(Ins *i0, Ins *i)
{
	while (i>i0) {
		i--;
		if (amd64_op[i->op].zflag)
			return i;
		if (amd64_op[i->op].lflag)
			continue;
		return 0;
	}
	return 0;
}

static void
seljmp(Blk *b, Fn *fn)
{
	Ref r;
	int c, k, swap;
	Ins *fi;
	Tmp *t;

	if (b->jmp.type == Jret0
	|| b->jmp.type == Jjmp
	|| b->jmp.type == Jhlt)
		return;
	assert(b->jmp.type == Jjnz);
	r = b->jmp.arg;
	t = &fn->tmp[r.val];
	b->jmp.arg = R;
	assert(rtype(r) == RTmp);
	if (b->s1 == b->s2) {
		chuse(r, -1, fn);
		b->jmp.type = Jjmp;
		b->s2 = 0;
		return;
	}
	fi = flagi(b->ins, &b->ins[b->nins]);
	if (!fi || !req(fi->to, r)) {
		selcmp((Ref[2]){r, CON_Z}, Kw, 0, fn);
		b->jmp.type = Jjf + Cine;
	}
	else if (iscmp(fi->op, &k, &c)
	     && c != NCmpI+Cfeq /* see sel() */
	     && c != NCmpI+Cfne) {
		swap = cmpswap(fi->arg, c);
		if (swap)
			c = cmpop(c);
		if (t->nuse == 1) {
			selcmp(fi->arg, k, swap, fn);
			*fi = (Ins){.op = Onop};
		}
		b->jmp.type = Jjf + c;
	}
	else if (fi->op == Oand && t->nuse == 1
	     && (rtype(fi->arg[0]) == RTmp ||
	         rtype(fi->arg[1]) == RTmp)) {
		fi->op = Oxtest;
		fi->to = R;
		b->jmp.type = Jjf + Cine;
		if (rtype(fi->arg[1]) == RCon) {
			r = fi->arg[1];
			fi->arg[1] = fi->arg[0];
			fi->arg[0] = r;
		}
	}
	else {
		/* since flags are not tracked in liveness,
		 * the result of the flag-setting instruction
		 * has to be marked as live
		 */
		if (t->nuse == 1)
			emit(Ocopy, Kw, R, r, R);
		b->jmp.type = Jjf + Cine;
	}
}

enum {
	Pob,
	Pbis,
	Pois,
	Pobis,
	Pbi1,
	Pobi1,
};

/* mgen generated code
 *
 * (with-vars (o b i s)
 *   (patterns
 *     (ob   (add (con o) (tmp b)))
 *     (bis  (add (tmp b) (mul (tmp i) (con s 1 2 4 8))))
 *     (ois  (add (con o) (mul (tmp i) (con s 1 2 4 8))))
 *     (obis (add (con o) (tmp b) (mul (tmp i) (con s 1 2 4 8))))
 *     (bi1  (add (tmp b) (tmp i)))
 *     (obi1 (add (con o) (tmp b) (tmp i)))
 * ))
 */

static int
opn(int op, int l, int r)
{
	static uchar Oaddtbl[91] = {
		2,
		2,2,
		4,4,5,
		6,6,8,8,
		4,4,9,10,9,
		7,7,5,8,9,5,
		4,4,12,10,12,12,12,
		4,4,9,10,9,9,12,9,
		11,11,5,8,9,5,12,9,5,
		7,7,5,8,9,5,12,9,5,5,
		11,11,5,8,9,5,12,9,5,5,5,
		4,4,9,10,9,9,12,9,9,9,9,9,
		7,7,5,8,9,5,12,9,5,5,5,9,5,
	};
	int t;

	if (l < r)
		t = l, l = r, r = t;
	switch (op) {
	case Omul:
		if (2 <= l)
		if (r == 0) {
			return 3;
		}
		return 2;
	case Oadd:
		return Oaddtbl[(l + l*l)/2 + r];
	default:
		return 2;
	}
}

static int
refn(Ref r, Num *tn, Con *con)
{
	int64_t n;

	switch (rtype(r)) {
	case RTmp:
		if (!tn[r.val].n)
			tn[r.val].n = 2;
		return tn[r.val].n;
	case RCon:
		if (con[r.val].type != CBits)
			return 1;
		n = con[r.val].bits.i;
		if (n == 8 || n == 4 || n == 2 || n == 1)
			return 0;
		return 1;
	default:
		return INT_MIN;
	}
}

static bits match[13] = {
	[4] = BIT(Pob),
	[5] = BIT(Pbi1),
	[6] = BIT(Pob) | BIT(Pois),
	[7] = BIT(Pob) | BIT(Pobi1),
	[8] = BIT(Pbi1) | BIT(Pbis),
	[9] = BIT(Pbi1) | BIT(Pobi1),
	[10] = BIT(Pbi1) | BIT(Pbis) | BIT(Pobi1) | BIT(Pobis),
	[11] = BIT(Pob) | BIT(Pobi1) | BIT(Pobis),
	[12] = BIT(Pbi1) | BIT(Pobi1) | BIT(Pobis),
};

static uchar *matcher[] = {
	[Pbi1] = (uchar[]){
		1,3,1,3,2,0
	},
	[Pbis] = (uchar[]){
		5,1,8,5,27,1,5,1,2,5,13,3,1,1,3,3,3,2,0,1,
		3,3,3,2,3,1,0,1,29
	},
	[Pob] = (uchar[]){
		1,3,0,3,1,0
	},
	[Pobi1] = (uchar[]){
		5,3,9,9,10,33,12,35,45,1,5,3,11,9,7,9,4,9,
		17,1,3,0,3,1,3,2,0,3,1,1,3,0,34,1,37,1,5,2,
		5,7,2,7,8,37,29,1,3,0,1,32
	},
	[Pobis] = (uchar[]){
		5,2,10,7,11,19,49,1,1,3,3,3,2,1,3,0,3,1,0,
		1,3,0,5,1,8,5,25,1,5,1,2,5,13,3,1,1,3,3,3,
		2,0,1,3,3,3,2,26,1,51,1,5,1,6,5,9,1,3,0,51,
		3,1,1,3,0,45
	},
	[Pois] = (uchar[]){
		1,3,0,1,3,3,3,2,0
	},
};

/* end of generated code */

static void
anumber(Num *tn, Blk *b, Con *con)
{
	Ins *i;
	Num *n;

	for (i=b->ins; i<&b->ins[b->nins]; i++) {
		if (rtype(i->to) != RTmp)
			continue;
		n = &tn[i->to.val];
		n->l = i->arg[0];
		n->r = i->arg[1];
		n->nl = refn(n->l, tn, con);
		n->nr = refn(n->r, tn, con);
		n->n = opn(i->op, n->nl, n->nr);
	}
}

static Ref
adisp(Con *c, Num *tn, Ref r, Fn *fn, int s)
{
	Ref v[2];
	int n;

	while (!req(r, R)) {
		assert(rtype(r) == RTmp);
		n = refn(r, tn, fn->con);
		if (!(match[n] & BIT(Pob)))
			break;
		runmatch(matcher[Pob], tn, r, v);
		assert(rtype(v[0]) == RCon);
		addcon(c, &fn->con[v[0].val], s);
		r = v[1];
	}
	return r;
}

static int
amatch(Addr *a, Num *tn, Ref r, Fn *fn)
{
	static int pat[] = {Pobis, Pobi1, Pbis, Pois, Pbi1, -1};
	Ref ro, rb, ri, rs, v[4];
	Con *c, co;
	int s, n, *p;

	if (rtype(r) != RTmp)
		return 0;

	n = refn(r, tn, fn->con);
	memset(v, 0, sizeof v);
	for (p=pat; *p>=0; p++)
		if (match[n] & BIT(*p)) {
			runmatch(matcher[*p], tn, r, v);
			break;
		}
	if (*p < 0)
		v[1] = r;

	memset(&co, 0, sizeof co);
	ro = v[0];
	rb = adisp(&co, tn, v[1], fn, 1);
	ri = v[2];
	rs = v[3];
	s = 1;

	if (*p < 0 && co.type != CUndef)
	if (amatch(a, tn, rb, fn))
		return addcon(&a->offset, &co, 1);
	if (!req(ro, R)) {
		assert(rtype(ro) == RCon);
		c = &fn->con[ro.val];
		if (!addcon(&co, c, 1))
			return 0;
	}
	if (!req(rs, R)) {
		assert(rtype(rs) == RCon);
		c = &fn->con[rs.val];
		assert(c->type == CBits);
		s = c->bits.i;
	}
	ri = adisp(&co, tn, ri, fn, s);
	*a = (Addr){co, rb, ri, s};

	if (rtype(ri) == RTmp)
	if (fn->tmp[ri.val].slot != -1) {
		if (a->scale != 1
		|| fn->tmp[rb.val].slot != -1)
			return 0;
		a->base = ri;
		a->index = rb;
	}
	if (!req(a->base, R)) {
		assert(rtype(a->base) == RTmp);
		s = fn->tmp[a->base.val].slot;
		if (s != -1)
			a->base = SLOT(s);
	}
	return 1;
}

/* instruction selection
 * requires use counts (as given by parsing)
 */
void
amd64_isel(Fn *fn)
{
	Blk *b, **sb;
	Ins *i;
	Phi *p;
	uint a;
	int n, al;
	int64_t sz;
	Num *num;

	/* assign slots to fast allocs */
	b = fn->start;
	/* specific to NAlign == 3 */ /* or change n=4 and sz /= 4 below */
	for (al=Oalloc, n=4; al<=Oalloc1; al++, n*=2)
		for (i=b->ins; i<&b->ins[b->nins]; i++)
			if (i->op == al) {
				if (rtype(i->arg[0]) != RCon)
					break;
				sz = fn->con[i->arg[0].val].bits.i;
				if (sz < 0 || sz >= INT_MAX-15)
					err("invalid alloc size %"PRId64, sz);
				sz = (sz + n-1) & -n;
				sz /= 4;
				if (sz > INT_MAX - fn->slot)
					die("alloc too large");
				fn->tmp[i->to.val].slot = fn->slot;
				fn->slot += sz;
				fn->salign = 2 + al - Oalloc;
				*i = (Ins){.op = Onop};
			}

	/* process basic blocks */
	n = fn->ntmp;
	num = emalloc(n * sizeof num[0]);
	for (b=fn->start; b; b=b->link) {
		curi = &insb[NIns];
		for (sb=(Blk*[3]){b->s1, b->s2, 0}; *sb; sb++)
			for (p=(*sb)->phi; p; p=p->link) {
				for (a=0; p->blk[a] != b; a++)
					assert(a+1 < p->narg);
				fixarg(&p->arg[a], p->cls, 0, fn);
			}
		memset(num, 0, n * sizeof num[0]);
		anumber(num, b, fn->con);
		seljmp(b, fn);
		for (i=&b->ins[b->nins]; i!=b->ins;)
			sel(*--i, num, fn);
		idup(b, curi, &insb[NIns]-curi);
	}
	free(num);

	if (debug['I']) {
		fprintf(stderr, "\n> After instruction selection:\n");
		printfn(fn, stderr);
	}
}
