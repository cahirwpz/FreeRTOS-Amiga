#include <stdlib.h>

/*
 * Compute x = (7^5 * x) mod (2^31 - 1)
 * without overflowing 31 bits:
 *      (2^31 - 1) = 127773 * (7^5) + 2836
 * From "Random number generators: good ones are hard to find",
 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
 * October 1988, p. 1195.
 */
int rand_r(unsigned *seed) {
	/* Transform to [1, 0x7ffffffe] range. */
	long x = (*seed % 0x7ffffffe) + 1;
	long hi = x / 127773;
	long lo = x % 127773;
	x = 16807 * lo - 2836 * hi;
	if (x < 0)
		x += 0x7fffffff;
	/* Transform to [0, 0x7ffffffd] range. */
	x--;
	*seed = x;
	return x;
}
