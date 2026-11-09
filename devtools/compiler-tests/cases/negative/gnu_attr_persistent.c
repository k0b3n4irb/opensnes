// GNU attribute persistent: an initialised object the startup code does not
// reinitialise. Ignored, it was an ordinary object, reset at every start.
int boots __attribute__((persistent)) = 1;
