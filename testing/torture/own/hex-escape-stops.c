/* A hex escape stops at the first character that is not a hex digit (ee5b8636).
 * cproc's hexval() used to take every letter a-z as a digit: "\x7Fw" was one byte, 0xC0.
 * Ours, in the format of the suite: abort() when wrong, exit(0) when right. */
void abort(void);
void exit(int);

int main(void)
{
    static const unsigned char s[] = "\x7Fw";
    static const unsigned char t[] = "\x41zz\x42";   /* 'A' 'z' 'z' 'B' */
    if (sizeof s != 3) abort();
    if (s[0] != 0x7F) abort();
    if (s[1] != 'w') abort();
    if (s[2] != 0) abort();
    if (sizeof t != 5) abort();
    if (t[0] != 'A' || t[1] != 'z' || t[2] != 'z' || t[3] != 'B') abort();
    exit(0);
    return 0;
}
