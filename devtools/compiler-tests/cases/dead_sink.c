// The inner loop of an insertion sort. QBE hoists the address of arr[j - 1]
// to the loop header and copies it back next to each use; until 2026-10-08
// the hoisted originals and half of the copies were left without a use and
// emitted all the same: 32-bit index and address values (a seven-step shift
// for the high word, an add of the array's bank) stored and never read —
// 258 of the 496 cycles of an iteration.
unsigned short arr[64];

void sort_step(unsigned short key, unsigned short j) {
    while (j > 0 && arr[j - 1] > key) {
        arr[j] = arr[j - 1];
        j--;
    }
    arr[j] = key;
}
