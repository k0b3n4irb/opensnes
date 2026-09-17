// Inline assembly: cproc has no asm statement. SNES asm lives in .asm files
// assembled by wla-65816 and called through the C ABI (compiler/ABI.md).
int main(void) { __asm__("nop"); return 0; }
