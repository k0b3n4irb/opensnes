#ifndef INSECT_H
#define INSECT_H

#include <snes.h>

/** @brief Create a robot insect (object type 1) patrolling between minx and maxx */
void insectinit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx);
/** @brief The insect's per-frame update: patrol, animation, sprite position */
void insectupdate(u16 idx);

#endif
