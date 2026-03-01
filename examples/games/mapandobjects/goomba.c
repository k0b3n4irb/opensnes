/*
 * Goomba object management
 * Based on PVSnesLib mapandobjects by Alekmaul
 *
 * Uses the workspace pattern: objWorkspace is populated before each callback.
 */
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>

#define GOOMBA_LEFT    1
#define GOOMBA_RIGHT   2
#define GOOMBA_XVELOC  0x028A

extern u8 sprgoomba;
extern u16 nbobjects;

u16 goombanum;
s16 goombax, goombay;

void goombainit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx) {
    if (objNew(type, xp, yp) == 0)
        return;

    /* objNew copies the new object to objWorkspace — set fields here */
    objGetPointer(objgetid);
    objWorkspace.width = 16;
    objWorkspace.height = 16;
    objWorkspace.sprframe = 0;
    objWorkspace.count = 10;
    objWorkspace.dir = GOOMBA_LEFT;
    objWorkspace.xvel = -GOOMBA_XVELOC;
    objWorkspace.xmin = minx;
    objWorkspace.xmax = maxx;
    objWorkspace.sprnum = nbobjects;

    /* Set up dynamic sprite */
    oambuffer[nbobjects].oamx = xp;
    oambuffer[nbobjects].oamy = yp;
    oambuffer[nbobjects].oamframeid = 0;
    oambuffer[nbobjects].oamrefresh = 1;
    oambuffer[nbobjects].oamattribute = 0x20 | (0 << 1);
    OAM_SET_GFX(nbobjects, &sprgoomba);
    nbobjects++;
}

void goombaupdate(u16 idx) {
    /* objWorkspace is pre-populated by the engine */
    goombax = objWorkspace.xpos[1] | (objWorkspace.xpos[2] << 8);
    goombanum = objWorkspace.sprnum;

    objWorkspace.count++;
    if (objWorkspace.count >= 10) {
        objWorkspace.count = 0;
        objWorkspace.sprframe = (1 - objWorkspace.sprframe);
        oambuffer[goombanum].oamframeid = objWorkspace.sprframe;
        oambuffer[goombanum].oamrefresh = 1;

        if (objWorkspace.dir == GOOMBA_LEFT) {
            if (goombax <= (s16)objWorkspace.xmin) {
                objWorkspace.dir = GOOMBA_RIGHT;
                objWorkspace.xvel = +GOOMBA_XVELOC;
                objWorkspace.yvel = 0;
            } else {
                objWorkspace.xvel = -GOOMBA_XVELOC;
            }
        } else {
            if (goombax >= (s16)objWorkspace.xmax) {
                objWorkspace.dir = GOOMBA_LEFT;
                objWorkspace.xvel = -GOOMBA_XVELOC;
                objWorkspace.yvel = 0;
            } else {
                objWorkspace.xvel = +GOOMBA_XVELOC;
            }
        }

        objUpdateXY(idx);
    }

    /* Update sprite screen position */
    goombay = (objWorkspace.ypos[1] | (objWorkspace.ypos[2] << 8)) - y_pos;
    goombax = goombax - x_pos;
    oambuffer[goombanum].oamx = goombax;
    oambuffer[goombanum].oamy = goombay;
    oamDynamic16Draw(goombanum);
}
