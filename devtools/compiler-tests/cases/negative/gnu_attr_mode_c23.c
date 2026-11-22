// The same attribute in the C23 spelling, prefix and name with underscores:
// the refusal does not depend on how the attribute is written.
[[__gnu__::__mode__(__QI__)]] typedef unsigned int u8q;
u8q next(u8q a) { return a + 1; }
