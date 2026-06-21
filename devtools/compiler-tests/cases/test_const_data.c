/* Test: const data arrays should be placed in ROM (SUPERFREE sections),
 * not in RAMSECTION. This is critical for large const tables that would
 * overflow bank 0 RAM if placed there.
 */
static const unsigned char const_arr[] = { 1, 2, 3, 4, 5 };
static unsigned char mut_arr[] = { 10, 20, 30 };

/* Test with typedef */
typedef unsigned char u8;
static const u8 const_u8_arr[] = { 0xFF, 0x00, 0x81 };

int main(void) {
    return const_arr[0] + mut_arr[0] + const_u8_arr[0];
}
