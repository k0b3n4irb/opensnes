// A read through a string literal. `*"a"` stopped the compiler on
// "unimplemented expression"; `"abc"[i]` compiled to a read that ignored the
// literal's bank (lda.w sym,x), which the link then refused. A literal is
// ROM data in a bank the linker picks: every read of it is a far read.
char first(void) { return *"a"; }
char nth(int i) { return *("abc" + i); }
char idx(int i) { return "abc"[i]; }
char swapped(int i) { return (i + "abc")[1]; }
