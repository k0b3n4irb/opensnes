/* templates.h — the files `opensnes init` writes. @LEAF@ is replaced by the
 * project's leaf name (the ROM is <leaf>.sfc). Kept byte for byte what the
 * shell CLI wrote until 2026-10-06 (tests/golden holds its output). */
#ifndef OPENSNES_TEMPLATES_H
#define OPENSNES_TEMPLATES_H

static const char TEMPLATE_BLANK_MAIN[] =
    "#include <snes.h>\n"
    "\n"
    "int main(void) {\n"
    "    /* One call sets up the PPU, the text engine, and the default font. */\n"
    "    textModeInit();\n"
    "\n"
    "    /* The NMI handler flushes printed text to VRAM automatically. */\n"
    "    textPrintAt(9, 14, \"HELLO SNES!\");\n"
    "\n"
    "    setScreenOn();\n"
    "\n"
    "    while (1) {\n"
    "        WaitForVBlank();\n"
    "    }\n"
    "\n"
    "    return 0;\n"
    "}\n"
    ;

#endif
