#include "all.h"

static void
blit(Ref sd[2], int sz, Fn *fn)
{
	struct { int st, ld, cls, size; } *p, tbl[] = {
		{ Ostorel, Oload,   Kl, 8 },
		{ Ostorew, Oload,   Kw, 4 },
		{ Ostoreh, Oloaduh, Kw, 2 },
		{ Ostoreb, Oloadub, Kw, 1 }
	};
	Ref r, r1, ro;
	int off, fwd, n;

	fwd = sz >= 0;
	sz = abs(sz);
	off = fwd ? sz : 0;
	for (p=tbl; sz; p++)
		for (n=p->size; sz>=n; sz-=n) {
			off -= fwd ? n : 0;
			r = newtmp("blt", Kl, fn);
			r1 = newtmp("blt", Kl, fn);
			ro = getcon(off, fn);
			emit(p->st, 0, R, r, r1);
			emit(Oadd, Kl, r1, sd[1], ro);
			r1 = newtmp("blt", Kl, fn);
			emit(p->ld, p->cls, r, r1, R);
			emit(Oadd, Kl, r1, sd[0], ro);
			off += fwd ? 0 : n;
		}
}

static int
ulog2_tab64[64] = {
	63,  0,  1, 41, 37,  2, 16, 42,
	38, 29, 32,  3, 12, 17, 43, 55,
	39, 35, 30, 53, 33, 21,  4, 23,
	13,  9, 18,  6, 25, 44, 48, 56,
	62, 40, 36, 15, 28, 31, 11, 54,
	34, 52, 20, 22,  8,  5, 24, 47,
	61, 14, 27, 10, 51, 19,  7, 46,
	60, 26, 50, 45, 59, 49, 58, 57,
};

static int
ulog2(uint64_t pow2)
{
	return ulog2_tab64[(pow2 * 0x5b31ab928877a7e) >> 58];
}

static int
ispow2(uint64_t v)
{
	return v && (v & (v - 1)) == 0;
}

static void
ins(Ins **pi, int *new, Blk *b, Fn *fn)
{
	ulong ni;
	Con *c;
	Ins *i;
	Ref r;
	int n;

	i = *pi;
	/* simplify more instructions here;
	 * copy 0 into xor, bit rotations,
	 * etc. */
	switch (i->op) {
	case Oblit1:
		assert(i > b->ins);
		assert((i-1)->op == Oblit0);
		if (!*new) {
			curi = &insb[NIns];
			ni = &b->ins[b->nins] - (i+1);
			curi -= ni;
			icpy(curi, i+1, ni);
			*new = 1;
		}
		blit((i-1)->arg, rsval(i->arg[0]), fn);
		*pi = i-1;
		return;
	case Oudiv:
	case Ourem:
		r = i->arg[1];
		if (KBASE(i->cls) == 0)
		if (rtype(r) == RCon) {
			c = &fn->con[r.val];
			if (c->type == CBits)
			if (ispow2(c->bits.i)) {
				n = ulog2(c->bits.i);
				if (i->op == Ourem) {
					i->op = Oand;
					i->arg[1] = getcon((1ull<<n) - 1, fn);
				} else {
					i->op = Oshr;
					i->arg[1] = getcon(n, fn);
				}
			}
		}
		break;
	}
	if (*new)
		emiti(*i);
}

void
simpl(Fn *fn)
{
	Blk *b;
	Ins *i;
	int new;

	for (b=fn->start; b; b=b->link) {
		new = 0;
		for (i=&b->ins[b->nins]; i!=b->ins;) {
			--i;
			ins(&i, &new, b, fn);
		}
		if (new)
			idup(b, curi, &insb[NIns]-curi);
	}
}
