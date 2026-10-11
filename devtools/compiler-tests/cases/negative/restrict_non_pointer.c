// restrict qualifies pointers only (C11 6.7.3p2); it was accepted on an int.
int restrict r;
int main(void) { return r; }
