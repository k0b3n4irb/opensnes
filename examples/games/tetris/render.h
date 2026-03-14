#ifndef RENDER_H
#define RENDER_H

#include <snes.h>
#include "board.h"

/* Dirty flags — set by render/hud functions, cleared by renderFlush */
extern u8 bg2_dirty, bg3_dirty;

void renderInit(void);
void renderBoard(void);
void renderPiece(u8 type, u8 rot, s8 row, s8 col);
void renderErasePiece(u8 type, u8 rot, s8 row, s8 col);
void renderNextPiece(u8 type);
void renderClearNextPiece(void);
void renderLineClearFlash(LineClearResult *result, u8 frame);
void renderEnableGradient(void);
void renderSetMsgColor(u16 color);
void renderShake(s8 dx, s8 dy);
void renderFlush(void);

#endif
