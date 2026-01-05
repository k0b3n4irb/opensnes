#include <assert.h>
#include <stdio.h>

typedef unsigned long long ullong;

char seen[64];
ullong rbg = 0x1e0298f7a7e;

int
bit()
{
	int bit;

	bit = rbg & 1;
	rbg >>= 1;
	return bit;
}

int
search(ullong n, int b, ullong *out)
{
	int i, x;
	ullong y, z;

	if (b == 64) {
		*out = n;
		return 1;
	}

	x = 63 & ((n << (63 - b)) >> 58);
	assert(!(x & 0) && x <= 62);
	y = bit();

	for (i=0; i<2; i++) {
		z = x | (y << 5);
		if (!seen[z]) {
			seen[z] = (63-b)+1;
			if (search(n | (y << b), b+1, out))
				return 1;
			seen[z] = 0;
		}
		y ^= 1;
	}

	return 0;
}

int
main()
{
	ullong out;
	int i;

	if (search(0, 0, &out)) {
		printf("0x%llx\n", out);
		for (i=0; i<64; i++) {
			printf((i&7) == 0 ? "\t" : " ");
			printf("%2d,", seen[i]-1);
			if ((i&7) == 7)
				printf("\n");
		}
	} else
		puts("not found");
}
