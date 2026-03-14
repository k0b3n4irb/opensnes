#ifndef HUD_H
#define HUD_H

#include <snes.h>

void hudInit(void);
void hudUpdateScore(u16 score);
void hudUpdateLevel(u16 level);
void hudUpdateLines(u16 lines);
void hudShowMessage(const char *str);
void hudClearMessage(void);

#endif
