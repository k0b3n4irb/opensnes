// GNU attribute noinit: an object the startup code leaves alone (one that
// survives a reset). Ignored, it was an ordinary object, cleared at startup.
int boots __attribute__((noinit));
