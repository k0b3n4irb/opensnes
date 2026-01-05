#include "all.h"

typedef struct Ext Ext;

struct Ext {
	char zext;
	char nopw; /* is a no-op if arg width is <= nopw */
	char usew; /* uses only the low usew bits of arg */
};

static int
ext(Ins *i, Ext *e)
{
	static Ext tbl[] = {
		/*extsb*/ {0,  7,  8},
		/*extub*/ {1,  8,  8},
		/*extsh*/ {0, 15, 16},
		/*extuh*/ {1, 16, 16},
		/*extsw*/ {0, 31, 32},
		/*extuw*/ {1, 32, 32},
	};

	if (!isext(i->op))
		return 0;
	*e = tbl[i->op - Oextsb];
	return 1;
}

static int
bitwidth(uint64_t v)
{
	int n;

	n = 0;
	if (v >> 32) { n += 32; v >>= 32; }
	if (v >> 16) { n += 16; v >>= 16; }
	if (v >>  8) { n +=  8; v >>=  8; }
	if (v >>  4) { n +=  4; v >>=  4; }
	if (v >>  2) { n +=  2; v >>=  2; }
	if (v >>  1) { n +=  1; v >>=  1; }
	return n+v;
}

/* no more than w bits are used */
static int
usewidthle(Fn *fn, Ref r, int w)
{
	Ext e;
	Tmp *t;
	Use *u;
	Phi *p;
	Ins *i;
	Ref rc;
	int64_t v;
	int b;

	assert(rtype(r) == RTmp);
	t = &fn->tmp[r.val];
	for (u=t->use; u<&t->use[t->nuse]; u++) {
		switch (u->type) {
		case UPhi:
			p = u->u.phi;
			/* during gvn, phi nodes may be
			 * replaced by other temps; in
			 * this case, the replaced phi
			 * uses are added to the
			 * replacement temp uses and
			 * Phi.to is set to R */
			if (p->visit || req(p->to, R))
				continue;
			p->visit = 1;
			b = usewidthle(fn, p->to, w);
			p->visit = 0;
			if (b)
				continue;
			break;
		case UIns:
			i = u->u.ins;
			assert(i != 0);
			if (i->op == Ocopy)
				if (usewidthle(fn, i->to, w))
					continue;
			if (ext(i, &e)) {
				if (e.usew <= w)
					continue;
				if (usewidthle(fn, i->to, w))
					continue;
			}
			if (i->op == Oand) {
				if (req(r, i->arg[0]))
					rc = i->arg[1];
				else {
					assert(req(r, i->arg[1]));
					rc = i->arg[0];
				}
				if (isconbits(fn, rc, &v)
				&& bitwidth(v) <= w)
					continue;
				break;
			}
			break;
		default:
			break;
		}
		return 0;
	}
	return 1;
}

static int
min(int v1, int v2)
{
	return v1 < v2 ? v1 : v2;
}

/* is the ref narrower than w bits */
static int
defwidthle(Fn *fn, Ref r, int w)
{
	Ext e;
	Tmp *t;
	Phi *p;
	Ins *i;
	uint n;
	int64_t v;
	int x;

	if (isconbits(fn, r, &v)
	&& bitwidth(v) <= w)
		return 1;
	if (rtype(r) != RTmp)
		return 0;
	t = &fn->tmp[r.val];
	if (t->cls != Kw)
		return 0;

	if (!t->def) {
		/* phi def */
		for (p=fn->rpo[t->bid]->phi; p; p=p->link)
			if (req(p->to, r))
				break;
		assert(p);
		if (p->visit)
			return 1;
		p->visit = 1;
		for (n=0; n<p->narg; n++)
			if (!defwidthle(fn, p->arg[n], w)) {
				p->visit = 0;
				return 0;
			}
		p->visit = 0;
		return 1;
	}

	i = t->def;
	if (i->op == Ocopy)
                return defwidthle(fn, i->arg[0], w);
	if (i->op == Oshr || i->op == Osar) {
		if (isconbits(fn, i->arg[1], &v))
		if (0 < v && v <= 32) {
			if (i->op == Oshr && w+v >= 32)
				return 1;
			if (w < 32) {
				if (i->op == Osar)
					w = min(31, w+v);
				else
					w = min(32, w+v);
			}
		}
		return defwidthle(fn, i->arg[0], w);
	}
	if (iscmp(i->op, &x, &x))
		return w >= 1;
	if (i->op == Oand) {
		if (defwidthle(fn, i->arg[0], w)
		|| defwidthle(fn, i->arg[1], w))
			return 1;
		return 0;
	}
	if (i->op == Oor || i->op == Oxor) {
		if (defwidthle(fn, i->arg[0], w)
		&& defwidthle(fn, i->arg[1], w))
			return 1;
		return 0;
	}
	if (ext(i, &e)) {
		if (e.zext && e.usew <= w)
			return 1;
		w = min(w, e.nopw);
		return defwidthle(fn, i->arg[0], w);
	}

	return 0;
}

static int
isw1(Fn *fn, Ref r)
{
	return defwidthle(fn, r, 1);
}

/* insert early extub/extuh instructions
 * for pars used only narrowly; this
 * helps factoring extensions out of
 * loops
 *
 * needs use; breaks use
 */
void
narrowpars(Fn *fn)
{
	Blk *b;
	int loop;
	Ins ext, *i, *ins;
	uint npar, nins;
	Ref r;

	/* only useful for functions with loops */
	loop = 0;
	for (b=fn->start; b; b=b->link)
		if (b->loop > 1) {
			loop = 1;
			break;
		}
	if (!loop)
		return;

	b = fn->start;

	npar = 0;
	for (i=b->ins; i<&b->ins[b->nins]; i++) {
		if (!ispar(i->op))
			break;
		npar++;
	}
	if (npar == 0)
		return;

	nins = b->nins + npar;
	ins = vnew(nins, sizeof ins[0], PFn);
	icpy(ins, b->ins, npar);
	icpy(ins + 2*npar, b->ins+npar, b->nins-npar);
	b->ins = ins;
	b->nins = nins;

	for (i=b->ins; i<&b->ins[b->nins]; i++) {
		if (!ispar(i->op))
			break;
		ext = (Ins){.op = Onop};
		if (i->cls == Kw)
		if (usewidthle(fn, i->to, 16)) {
			ext.op = Oextuh;
			if (usewidthle(fn, i->to, 8))
				ext.op = Oextub;
			r = newtmp("vw", i->cls, fn);
			ext.cls = i->cls;
			ext.to = i->to;
			ext.arg[0] = r;
			i->to = r;
		}
		*(i+npar) = ext;
	}
}

Ref
copyref(Fn *fn, Blk *b, Ins *i)
{
	/* which extensions are copies for a given
	 * argument width */
	static bits extcpy[] = {
		[WFull] = 0,
		[Wsb] = BIT(Wsb) | BIT(Wsh) | BIT(Wsw),
		[Wub] = BIT(Wub) | BIT(Wuh) | BIT(Wuw),
		[Wsh] = BIT(Wsh) | BIT(Wsw),
		[Wuh] = BIT(Wuh) | BIT(Wuw),
		[Wsw] = BIT(Wsw),
		[Wuw] = BIT(Wuw),
	};
	Ext e;
	Tmp *t;
	int64_t v;
	int w, z;

	if (i->op == Ocopy)
		return i->arg[0];

	/* op identity value */
	if (optab[i->op].hasid
	&& KBASE(i->cls) == 0 /* integer only - fp NaN! */
	&& req(i->arg[1], con01[optab[i->op].idval])
	&& (!optab[i->op].cmpeqwl || isw1(fn, i->arg[0])))
		return i->arg[0];

	/* idempotent op with identical args */
	if (optab[i->op].idemp
	&& req(i->arg[0], i->arg[1]))
		return i->arg[0];

	/* integer cmp with identical args */
	if ((optab[i->op].cmpeqwl || optab[i->op].cmplgtewl)
	&& req(i->arg[0], i->arg[1]))
		return con01[optab[i->op].eqval];

	/* cmpeq/ne 0 with 0/non-0 inference */
	if (optab[i->op].cmpeqwl
	&& req(i->arg[1], CON_Z)
	&& zeroval(fn, b, i->arg[0], argcls(i, 0), &z))
		return con01[optab[i->op].eqval^z^1];

	/* redundant and mask */
	if (i->op == Oand
	&& isconbits(fn, i->arg[1], &v)
	&& (v > 0 && ((v+1) & v) == 0)
	&& defwidthle(fn, i->arg[0], bitwidth(v)))
		return i->arg[0];

	if (i->cls == Kw
	&& (i->op == Oextsw || i->op == Oextuw))
		return i->arg[0];

	if (ext(i, &e) && rtype(i->arg[0]) == RTmp) {
		t = &fn->tmp[i->arg[0].val];
		assert(KBASE(t->cls) == 0);

		/* do not break typing by returning
		 * a narrower temp */
		if (KWIDE(i->cls) > KWIDE(t->cls))
			return R;

		w  = Wsb + (i->op - Oextsb);
		if (BIT(w) & extcpy[t->width])
			return i->arg[0];

		/* avoid eliding extensions of params
		 * inserted in the start block; their
		 * point is to make further extensions
		 * redundant */
		if ((!t->def || !ispar(t->def->op))
		&& usewidthle(fn, i->to, e.usew))
			return i->arg[0];

		if (defwidthle(fn, i->arg[0], e.nopw))
			return i->arg[0];
	}

	return R;
}

static int
phieq(Phi *pa, Phi *pb)
{
	Ref r;
	uint n;

	assert(pa->narg == pb->narg);
	for (n=0; n<pa->narg; n++) {
		r = phiarg(pb, pa->blk[n]);
		if (!req(pa->arg[n], r))
			return 0;
	}
	return 1;
}

Ref
phicopyref(Fn *fn, Blk *b, Phi *p)
{
	Blk *d, **s;
	Phi *p1;
	uint n, c;

	/* identical args */
	for (n=0; n<p->narg-1; n++)
		if (!req(p->arg[n], p->arg[n+1]))
			break;
	if (n == p->narg-1)
		return p->arg[n];

	/* same as a previous phi */
	for (p1=b->phi; p1!=p; p1=p1->link) {
		assert(p1);
		if (phieq(p1, p))
			return p1->to;
	}

	/* can be replaced by a
	 * dominating jnz arg */
	d = b->idom;
	if (p->narg != 2
	|| d->jmp.type != Jjnz
	|| !isw1(fn, d->jmp.arg))
		return R;

	s = (Blk*[]){0, 0};
	for (n=0; n<2; n++)
		for (c=0; c<2; c++)
			if (req(p->arg[n], con01[c]))
				s[c] = p->blk[n];

	/* if s1 ends with a jnz on either b
	 * or s2; the inference below is wrong
	 * without the jump type checks */
	if (d->s1 == s[1] && d->s2 == s[0]
	&& d->s1->jmp.type == Jjmp
	&& d->s2->jmp.type == Jjmp)
		return d->jmp.arg;

	return R;
}
