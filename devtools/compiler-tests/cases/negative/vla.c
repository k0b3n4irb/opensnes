// Variable-length arrays: until 2026-10-17 this compiled without a word and
// the array became a pointer to a fixed 2-byte frame slot, so fill() wrote
// over the return address (audit 04-1). The driver defines __STDC_NO_VLA__
// (C11 6.10.8.3); the #error below fires if it stops doing so.
#ifndef __STDC_NO_VLA__
#error __STDC_NO_VLA__ is not defined
#endif
unsigned fill(unsigned *p, unsigned n);
unsigned vla(unsigned n) { unsigned a[n]; return fill(a, n); }
