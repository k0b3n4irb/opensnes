/* Regression: the anonymous string-literal label must be namespaced per-TU
 * (<file>_string.N), never a bare `string.N` that collides across translation
 * units at link. Fix: compiler/cproc/qbe.c emitname + bin/cc65816 CC65816_TU. */
const char *msg(void) { return "anon_marker"; }
