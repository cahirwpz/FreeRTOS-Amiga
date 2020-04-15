static unsigned randseed = 1;

/*
 * Compute x[n + 1] = (7^5 * x[n]) mod (2^31 - 1).
 * From "Random number generators: good ones are hard to find",
 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
 * October 1988, p. 1195.
 */
int rand(void) {
  int x = randseed;
  int hi = x / 127773;
  int lo = x % 127773;
  int t = 16807 * lo - 2836 * hi;
  if (t < 0)
    t += 0x7fffffff;
  randseed = t;
  return t;
}
