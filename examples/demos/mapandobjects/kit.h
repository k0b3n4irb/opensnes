#ifndef KIT_H
#define KIT_H

#include <snes.h>

/** @brief Create the player object (object type 0) at a map position */
void kitinit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx);
/** @brief The player's per-frame update: input, collision with slopes, animation, camera */
void kitupdate(u16 idx);

#endif
