// A10: an address above $FFFF cast to a plain pointer. The store at a
// constant index kept the bank by folding (`sta.l $708045`); the store at a
// variable index went to $00:8040+i, in ROM, without a word. Found writing
// Game Pak RAM from C (chips/superfx_game_skeleton, 2026-10-24).
#include <snes/types.h>
#define EDGE_TABLE ((u8 *)0x708040)
void put(u16 i, u8 v) { EDGE_TABLE[i] = v; }
