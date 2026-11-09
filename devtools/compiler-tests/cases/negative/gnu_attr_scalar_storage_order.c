// GNU attribute scalar_storage_order: the members are stored big-endian, as
// in a file from a big-endian machine. Ignored, 0x1234 was stored 34 12 and
// read back as written: data in the other order read wrong without a word.
struct __attribute__((scalar_storage_order("big-endian"))) be { unsigned short w; };
struct be x = { 0x1234 };
