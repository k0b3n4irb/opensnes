/* Test word extension operations (extsw/extuw) */

/* This tests array indexing which triggers word-to-long extension
 * when calculating array offsets */

static const unsigned char data[] = {1, 2, 3, 4, 5, 6, 7, 8};

unsigned char get_element(unsigned short index) {
    return data[index];
}

int main(void) {
    unsigned char val = 0;
    unsigned short i;

    for (i = 0; i < 8; i++) {
        val = val + get_element(i);
    }

    return val;
}
