// Qualifiers between the brackets belong to the outermost array of a
// parameter only (C11 6.7.6.2p1); the inner one was accepted.
void f(int a[const 3][const 4]);
int main(void) { return 0; }
