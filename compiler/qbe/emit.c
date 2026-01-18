#include "all.h"

enum {
	SecText,
	SecData,
	SecBss,
};

static int datasec_counter = 0;

/* Emit a string with proper handling of null terminators for WLA-DX.
 * WLA-DX doesn't support \000 escape sequences, so we convert them to:
 * "Hello\000World" -> "Hello", 0, "World"
 */
static void
emit_wladx_string(char *str, FILE *f)
{
	char *p = str;
	int in_string = 0;
	int need_comma = 0;

	/* Skip opening quote if present */
	if (*p == '"')
		p++;

	fputs("\t.ASC ", f);

	while (*p) {
		/* Check for \000 escape sequence (4 chars) */
		if (p[0] == '\\' && p[1] == '0' && p[2] == '0' && p[3] == '0') {
			if (in_string) {
				fputc('"', f);
				in_string = 0;
			}
			if (need_comma)
				fputs(", ", f);
			fputs("0", f);
			need_comma = 1;
			p += 4;
			continue;
		}
		/* Check for closing quote */
		if (*p == '"' && p[1] == '\0') {
			if (in_string)
				fputc('"', f);
			break;
		}
		/* Regular character */
		if (!in_string) {
			if (need_comma)
				fputs(", ", f);
			fputc('"', f);
			in_string = 1;
			need_comma = 1;
		}
		fputc(*p, f);
		p++;
	}

	/* Close string if still open and not ended by closing quote */
	if (in_string && *p != '"')
		fputc('"', f);

	fputc('\n', f);
}

void
emitlnk(char *n, Lnk *l, int s, FILE *f)
{
	char *pfx;
	char *name;
	(void)l;  /* WLA-DX doesn't use linkage flags */

	pfx = n[0] == '"' ? "" : T.assym;

	/* WLA-DX: strip .L prefix from local labels (e.g., .Lstring.1 -> string.1)
	 * WLA-DX doesn't support labels starting with '.' */
	name = n;
	if (n[0] == '.' && n[1] == 'L')
		name = n + 2;

	/* WLA-DX compatible section syntax for w65816 */
	switch (s) {
	case SecText:
		fprintf(f, ".SECTION \".text.%s\" SUPERFREE\n", name);
		break;
	case SecData:
		fprintf(f, ".SECTION \".rodata.%d\" SUPERFREE\n", ++datasec_counter);
		break;
	case SecBss:
		/* BSS goes to RAM (SLOT 1) not ROM */
		fprintf(f, ".RAMSECTION \".bss.%d\" BANK 0 SLOT 1\n", ++datasec_counter);
		break;
	}
	/* WLA-DX: skip .globl since we compile as single file.
	 * Symbols are visible within the same compilation unit. */
	fprintf(f, "%s%s:\n", pfx, name);
}

void
emitfnlnk(char *n, Lnk *l, FILE *f)
{
	emitlnk(n, l, SecText, f);
}

void
emitdat(Dat *d, FILE *f)
{
	/* Use WLA-DX compatible directives for w65816 target */
	static char *dtoa[] = {
		[DB] = "\t.db",
		[DH] = "\t.dw",
		[DW] = "\t.dl",  /* WLA-DX uses .dl for 32-bit (long) */
		[DL] = "\t.dl"   /* 8-byte emitted as two 4-byte .dl */
	};
	static int64_t zero;
	char *p;

	switch (d->type) {
	case DStart:
		zero = 0;
		break;
	case DEnd:
		if (d->lnk->common) {
			if (zero == -1)
				die("invalid common data definition");
			p = d->name[0] == '"' ? "" : T.assym;
			fprintf(f, ".comm %s%s,%"PRId64,
				p, d->name, zero);
			if (d->lnk->align)
				fprintf(f, ",%d", d->lnk->align);
			fputc('\n', f);
		}
		else if (zero != -1) {
			emitlnk(d->name, d->lnk, SecBss, f);
			/* RAMSECTION uses 'dsb' without dot and without fill value */
			fprintf(f, "\tdsb %"PRId64"\n", zero);
			fputs(".ENDS\n", f);
		} else {
			/* Data section was emitted, close it */
			fputs(".ENDS\n", f);
		}
		break;
	case DZ:
		if (zero != -1)
			zero += d->u.num;
		else
			fprintf(f, "\t.dsb %"PRId64", 0\n", d->u.num);  /* WLA-DX syntax */
		break;
	default:
		if (zero != -1) {
			emitlnk(d->name, d->lnk, SecData, f);
			if (zero > 0)
				fprintf(f, "\t.dsb %"PRId64", 0\n", zero);  /* WLA-DX syntax */
			zero = -1;
		}
		if (d->isstr) {
			if (d->type != DB)
				err("strings only supported for 'b' currently");
			emit_wladx_string(d->u.str, f);  /* WLA-DX with null handling */
		}
		else if (d->isref) {
			char *refname = d->u.ref.name;
			/* Strip .L prefix from references for WLA-DX */
			if (refname[0] == '.' && refname[1] == 'L')
				refname = refname + 2;
			p = refname[0] == '"' ? "" : T.assym;
			fprintf(f, "%s %s%s%+"PRId64"\n",
				dtoa[d->type], p, refname,
				d->u.ref.off);
		}
		else {
			fprintf(f, "%s %"PRId64"\n",
				dtoa[d->type], d->u.num);
		}
		break;
	}
}

typedef struct Asmbits Asmbits;

struct Asmbits {
	bits n;
	int size;
	Asmbits *link;
};

static Asmbits *stash;

int
stashbits(bits n, int size)
{
	Asmbits **pb, *b;
	int i;

	assert(size == 4 || size == 8 || size == 16);
	for (pb=&stash, i=0; (b=*pb); pb=&b->link, i++)
		if (size <= b->size && b->n == n)
			return i;
	b = emalloc(sizeof *b);
	b->n = n;
	b->size = size;
	b->link = 0;
	*pb = b;
	return i;
}

static void
emitfin(FILE *f, char *sec[3])
{
	Asmbits *b;
	int lg, i;
	union { int32_t i; float f; } u;

	if (!stash)
		return;
	fprintf(f, "/* floating point constants */\n");
	for (lg=4; lg>=2; lg--)
		for (b=stash, i=0; b; b=b->link, i++) {
			if (b->size == (1<<lg)) {
				fprintf(f,
					".section %s\n"
					".p2align %d\n"
					"%sfp%d:",
					sec[lg-2], lg, T.asloc, i
				);
				if (lg == 4)
					fprintf(f,
						"\n\t.quad %"PRId64
						"\n\t.quad 0\n\n",
						(int64_t)b->n);
				else if (lg == 3)
					fprintf(f,
						"\n\t.quad %"PRId64
						" /* %f */\n\n",
						(int64_t)b->n,
						*(double *)&b->n);
				else if (lg == 2) {
					u.i = b->n;
					fprintf(f,
						"\n\t.int %"PRId32
						" /* %f */\n\n",
						u.i, (double)u.f);
				}
			}
		}
	while ((b=stash)) {
		stash = b->link;
		free(b);
	}
}

void
elf_emitfin(FILE *f)
{
	static char *sec[3] = { ".rodata", ".rodata", ".rodata" };

	emitfin(f ,sec);
	fprintf(f, ".section .note.GNU-stack,\"\",@progbits\n");
}

void
elf_emitfnfin(char *fn, FILE *f)
{
	fprintf(f, ".type %s, @function\n", fn);
	fprintf(f, ".size %s, .-%s\n", fn, fn);
}

void
macho_emitfin(FILE *f)
{
	static char *sec[3] = {
		"__TEXT,__literal4,4byte_literals",
		"__TEXT,__literal8,8byte_literals",
		".abort \"unreachable\"",
	};

	emitfin(f, sec);
}

static uint32_t *file;
static uint nfile;
static uint curfile;

void
emitdbgfile(char *fn, FILE *f)
{
	uint32_t id;
	uint n;

	id = intern(fn);
	for (n=0; n<nfile; n++)
		if (file[n] == id) {
			/* gas requires positive
			 * file numbers */
			curfile = n + 1;
			return;
		}
	if (!file)
		file = vnew(0, sizeof *file, PHeap);
	vgrow(&file, ++nfile);
	file[nfile-1] = id;
	curfile = nfile;
	fprintf(f, ".file %u %s\n", curfile, fn);
}

void
emitdbgloc(uint line, uint col, FILE *f)
{
	if (col != 0)
		fprintf(f, "\t.loc %u %u %u\n", curfile, line, col);
	else
		fprintf(f, "\t.loc %u %u\n", curfile, line);
}
