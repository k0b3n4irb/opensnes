// A switch over consecutive values. The case search is a binary tree of
// "equal? below?" tests; a case whose neighbours in the tree are the values
// just below and just above it can only be that value, and needs no test
// (upstream cproc 863e1e7). Here case 0, reached only when v is below 1.
int dense(unsigned int v) {
    switch (v) {
    case 0: return 11;
    case 1: return 22;
    case 2: return 33;
    case 3: return 44;
    default: return 0;
    }
}
