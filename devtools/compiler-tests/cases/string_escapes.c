// Every byte of a string literal that is not printable, and the quote and
// the backslash, reaches the assembler as a number: WLA-DX's .ASC knows no
// octal escape. Until 2026-10-09 only the terminating zero was converted;
// "\n" came out as the three bytes 0, '1', '2' and "\xF0" as a backslash and
// three digits.
const char newline[] = "AB\nC";
const char high[] = "\xF0";
const char quote[] = "say \"hi\"";
const char backslash[] = "a\\b";
const char tab_first[] = "\tX";
