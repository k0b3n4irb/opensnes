#ifndef AUTOMATON_H
#define AUTOMATON_H

#include <snes.h>

/** @brief Create a patrol automaton (object type 2) patrolling between minx and maxx */
void automatoninit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx);
/** @brief The automaton's per-frame update: patrol, animation, sprite position */
void automatonupdate(u16 idx);

#endif
