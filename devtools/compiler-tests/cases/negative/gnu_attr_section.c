// GNU attribute section: place v in .myram. Ignored, v went to the plain RAM
// band with every other object; __far and __ramcode are the placements the
// SDK honours.
int v __attribute__((section(".myram"))) = 3;
