// GNU attribute cleanup: unlock(&x) runs when x goes out of scope. Ignored,
// it was never called.
void unlock(int *p);
int main(void) { int x __attribute__((cleanup(unlock))) = 1; return x; }
