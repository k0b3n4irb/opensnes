#include "all.h"

Ref con01[2];

static inline uint
mix(uint x0, uint x1)
{
	return x0 + 17*x1;
}

static inline uint
rhash(Ref r)
{
	return mix(r.type, r.val);
}

static uint
ihash(Ins *i)
{
	uint h;

	h = mix(i->op, i->cls);
	h = mix(h, rhash(i->arg[0]));
	h = mix(h, rhash(i->arg[1]));

	return h;
}

static int
ieq(Ins *ia, Ins *ib)
{
	if (ia->op == ib->op)
	if (ia->cls == ib->cls)
	if (req(ia->arg[0], ib->arg[0]))
	if (req(ia->arg[1], ib->arg[1]))
		return 1;
	return 0;
}

static Ins **gvntbl;
static uint gvntbln;

static Ins *
gvndup(Ins *i, int insert)
{
	uint idx, n;
	Ins *ii;

	idx = ihash(i) % gvntbln;
	for (n=1;; n++) {
		ii = gvntbl[idx];
		if (!ii)
			break;
		if (ieq(i, ii))
			return ii;

		idx++;
		if (gvntbln <= idx)
			idx = 0;
	}
	if (insert)
		gvntbl[idx] = i;
	return 0;
}

static void
replaceuse(Fn *fn, Use *u, Ref r1, Ref r2)
{
	Blk *b;
	Ins *i;
	Phi *p;
	Ref *pr;
	Tmp *t2;
	int n;

	t2 = 0;
	if (rtype(r2) == RTmp)
		t2 = &fn->tmp[r2.val];
	b = fn->rpo[u->bid];
	switch (u->type) {
	case UPhi:
		p = u->u.phi;
		for (pr=p->arg; pr<&p->arg[p->narg]; pr++)
			if (req(*pr, r1))
				*pr = r2;
		if (t2)
			adduse(t2, UPhi, b, p);
		break;
	case UIns:
		i = u->u.ins;
		for (n=0; n<2; n++)
			if (req(i->arg[n], r1))
				i->arg[n] = r2;
		if (t2)
			adduse(t2, UIns, b, i);
		break;
	case UJmp:
		if (req(b->jmp.arg, r1))
			b->jmp.arg = r2;
		if (t2)
			adduse(t2, UJmp, b);
		break;
	case UXXX:
		die("unreachable");
	}
}

static void
replaceuses(Fn *fn, Ref r1, Ref r2)
{
	Tmp *t1;
	Use *u;

	assert(rtype(r1) == RTmp);
	t1 = &fn->tmp[r1.val];
	for (u=t1->use; u<&t1->use[t1->nuse]; u++)
		replaceuse(fn, u, r1, r2);
	t1->nuse = 0;
}

static void
dedupphi(Fn *fn, Blk *b)
{
	Phi *p, **pp;
	Ref r;

	for (pp=&b->phi; (p=*pp);) {
		r = phicopyref(fn, b, p);
		if (!req(r, R)) {
			replaceuses(fn, p->to, r);
			p->to = R;
			*pp = p->link;
		} else
			pp = &p->link;
	}
}

static int
rcmp(Ref a, Ref b)
{
	if (rtype(a) != rtype(b))
		return rtype(a) - rtype(b);
	return a.val - b.val;
}

static void
normins(Fn *fn, Ins *i)
{
	uint n;
	int64_t v;
	Ref r;

	/* truncate constant bits to
	 * 32 bits for s/w uses */
	for (n=0; n<2; n++) {
		if (!KWIDE(argcls(i, n)))
		if (isconbits(fn, i->arg[n], &v))
		if ((v & 0xffffffff) != v)
			i->arg[n] = getcon(v & 0xffffffff, fn);
	}
	/* order arg[0] <= arg[1] for
	 * commutative ops, preferring
	 * RTmp in arg[0] */
	if (optab[i->op].commutes)
	if (rcmp(i->arg[0], i->arg[1]) > 0) {
		r = i->arg[1];
		i->arg[1] = i->arg[0];
		i->arg[0] = r;
	}
}

static int
negcon(int cls, Con *c)
{
	static Con z = {.type = CBits, .bits.i = 0};

	return foldint(c, Osub, cls, &z, c);
}

static void
assoccon(Fn *fn, Blk *b, Ins *i1)
{
	Tmp *t2;
	Ins *i2;
	int op, fail;
	Con c, c1, c2;

	op = i1->op;
	if (op == Osub)
		op = Oadd;

	if (!optab[op].assoc
	|| KBASE(i1->cls) != 0
	|| rtype(i1->arg[0]) != RTmp
	|| rtype(i1->arg[1]) != RCon)
		return;
	c1 = fn->con[i1->arg[1].val];

	t2 = &fn->tmp[i1->arg[0].val];
	if (t2->def == 0)
		return;
	i2 = t2->def;

	if (op != (i2->op == Osub ? Oadd : i2->op)
	|| rtype(i2->arg[1]) != RCon)
		return;
	c2 = fn->con[i2->arg[1].val];

	assert(KBASE(i2->cls) == 0);
	assert(KWIDE(i2->cls) >= KWIDE(i1->cls));

	if (i1->op == Osub && negcon(i1->cls, &c1))
		return;
	if (i2->op == Osub && negcon(i2->cls, &c2))
		return;
	if (foldint(&c, op, i1->cls, &c1, &c2))
		return;

	if (op == Oadd && c.type == CBits)
	if ((i1->cls == Kl  && c.bits.i < 0)
	|| (i1->cls == Kw && (int32_t)c.bits.i < 0)) {
		fail = negcon(i1->cls, &c);
		assert(fail == 0);
		op = Osub;
	}

	i1->op = op;
	i1->arg[0] = i2->arg[0];
	i1->arg[1] = newcon(&c, fn);
	adduse(&fn->tmp[i1->arg[0].val], UIns, b, i1);
}

static void
killins(Fn *fn, Ins *i, Ref r)
{
	replaceuses(fn, i->to, r);
	*i = (Ins){.op = Onop};
}

static void
dedupins(Fn *fn, Blk *b, Ins *i)
{
	Ref r;
	Ins *i1;

	normins(fn, i);
	if (i->op == Onop || pinned(i))
		return;

	assert(!req(i->to, R));
	assoccon(fn, b, i);

	r = copyref(fn, b, i);
	if (!req(r, R)) {
		killins(fn, i, r);
		return;
	}
	r = foldref(fn, i);
	if (!req(r, R)) {
		killins(fn, i, r);
		return;
	}
	i1 = gvndup(i, 1);
	if (i1) {
		killins(fn, i, i1->to);
		return;
	}
}

int
cmpeqz(Fn *fn, Ref r, Ref *arg, int *cls, int *eqval)
{
	Ins *i;

	if (rtype(r) != RTmp)
		return 0;
	i = fn->tmp[r.val].def;
	if (i)
	if (optab[i->op].cmpeqwl)
	if (req(i->arg[1], CON_Z)) {
		*arg = i->arg[0];
		*cls = argcls(i, 0);
		*eqval = optab[i->op].eqval;
		return 1;
	}
	return 0;
}

static int
branchdom(Fn *fn, Blk *bif, Blk *bbr1, Blk *bbr2, Blk *b)
{
	assert(bif->jmp.type == Jjnz);

	if (b != bif
	&& dom(bbr1, b)
	&& !reachesnotvia(fn, bbr2, b, bif))
		return 1;

	return 0;
}

static int
domzero(Fn *fn, Blk *d, Blk *b, int *z)
{
	if (branchdom(fn, d, d->s1, d->s2, b)) {
		*z = 0;
		return 1;
	}
	if (branchdom(fn, d, d->s2, d->s1, b)) {
		*z = 1;
		return 1;
	}
	return 0;
}

/* infer 0/non-0 value from dominating jnz */
int
zeroval(Fn *fn, Blk *b, Ref r, int cls, int *z)
{
	Blk *d;
	Ref arg;
	int cls1, eqval;

	for (d=b->idom; d; d=d->idom) {
		if (d->jmp.type != Jjnz)
			continue;
		if (req(r, d->jmp.arg)
		&& cls == Kw
		&& domzero(fn, d, b, z)) {
			return 1;
		}
		if (cmpeqz(fn, d->jmp.arg, &arg, &cls1, &eqval)
		&& req(r, arg)
		&& cls == cls1
		&& domzero(fn, d, b, z)) {
			*z ^= eqval;
			return 1;
		}
	}
	return 0;
}

static int
usecls(Use *u, Ref r, int cls)
{
	int k;

	switch (u->type) {
	case UIns:
		k = Kx;  /* widest use */
		if (req(u->u.ins->arg[0], r))
			k = argcls(u->u.ins, 0);
		if (req(u->u.ins->arg[1], r))
		if (k == Kx || !KWIDE(k))
			k = argcls(u->u.ins, 1);
		return k == Kx ? cls : k;
	case UPhi:
		if (req(u->u.phi->to, R))
			return cls; /* eliminated */
		return u->u.phi->cls;
	case UJmp:
		return Kw;
	default:
		break;
	}
	die("unreachable");
}

static void
propjnz0(Fn *fn, Blk *bif, Blk *s0, Blk *snon0, Ref r, int cls)
{
	Blk *b;
	Tmp *t;
	Use *u;

	if (s0->npred != 1 || rtype(r) != RTmp)
		return;
	t = &fn->tmp[r.val];
	for (u=t->use; u<&t->use[t->nuse]; u++) {
		b = fn->rpo[u->bid];
		/* we may compare an l temp with a w
		 * comparison; so check that the use
		 * does not involve high bits */
		if (usecls(u, r, cls) == cls)
		if (branchdom(fn, bif, s0, snon0, b))
			replaceuse(fn, u, r, CON_Z);
	}
}

static void
dedupjmp(Fn *fn, Blk *b)
{
	Blk **ps;
	int64_t v;
	Ref arg;
	int cls, eqval, z;

	if (b->jmp.type != Jjnz)
		return;

	/* propagate jmp arg as 0 through s2 */
	propjnz0(fn, b, b->s2, b->s1, b->jmp.arg, Kw);
	/* propagate cmp eq/ne 0 def of jmp arg as 0 */
	if (cmpeqz(fn, b->jmp.arg, &arg, &cls, &eqval)) {
		ps = (Blk*[]){b->s1, b->s2};
		propjnz0(fn, b, ps[eqval^1], ps[eqval], arg, cls);
	}

	/* collapse trivial/constant jnz to jmp */
	v = 1;
	z = 0;
	if (b->s1 == b->s2
	|| isconbits(fn, b->jmp.arg, &v)
	|| zeroval(fn, b, b->jmp.arg, Kw, &z)) {
		if (v == 0 || z)
			b->s1 = b->s2;
		/* we later move active ins out of dead blks */
		b->s2 = 0;
		b->jmp.type = Jjmp;
		b->jmp.arg = R;
	}
}

static void
rebuildcfg(Fn *fn)
{
	uint n, nblk;
	Blk *b, *s, **rpo;
	Ins *i;

	nblk = fn->nblk;
	rpo = emalloc(nblk * sizeof rpo[0]);
	memcpy(rpo, fn->rpo, nblk * sizeof rpo[0]);

	fillcfg(fn);

	/* move instructions that were in
	 * killed blocks and may be active
	 * in the computation in the start
	 * block */
	s = fn->start;
	for (n=0; n<nblk; n++) {
		b = rpo[n];
		if (b->id != -1u)
			continue;
		/* blk unreachable after GVN */
		assert(b != s);
		for (i=b->ins; i<&b->ins[b->nins]; i++)
			if (!optab[i->op].pinned)
			if (gvndup(i, 0) == i)
				addins(&s->ins, &s->nins, i);
	}
	free(rpo);
}

/* requires rpo pred ssa use
 * recreates rpo preds
 * breaks pred use dom ssa (GCM fixes ssa)
 */
void
gvn(Fn *fn)
{
	Blk *b;
	Phi *p;
	Ins *i;
	uint n, nins;

	con01[0] = getcon(0, fn);
	con01[1] = getcon(1, fn);

	/* copy.c uses the visit bit */
	for (b=fn->start; b; b=b->link)
		for (p=b->phi; p; p=p->link)
			p->visit = 0;

	fillloop(fn);
	narrowpars(fn);
	filluse(fn);
	ssacheck(fn);

	nins = 0;
	for (b=fn->start; b; b=b->link) {
		b->visit = 0;
		nins += b->nins;
	}

	gvntbln = nins + nins/2;
	gvntbl = emalloc(gvntbln * sizeof gvntbl[0]);
	for (n=0; n<fn->nblk; n++) {
		b = fn->rpo[n];
		dedupphi(fn, b);
		for (i=b->ins; i<&b->ins[b->nins]; i++)
			dedupins(fn, b, i);
		dedupjmp(fn, b);
	}
	rebuildcfg(fn);
	free(gvntbl);
	gvntbl = 0;

	if (debug['G']) {
		fprintf(stderr, "\n> After GVN:\n");
		printfn(fn, stderr);
	}
}
