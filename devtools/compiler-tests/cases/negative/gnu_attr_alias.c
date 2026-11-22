// GNU attribute alias: a is another name for b. Ignored, a was an object of
// its own and the store below never reached b.
int b = 5;
int a __attribute__((alias("b")));
int main(void) { a = 7; return b; }
