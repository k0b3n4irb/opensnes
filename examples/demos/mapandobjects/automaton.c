/*
 * Automaton: the patrol automaton: the same patrol, slower, a taller sprite
 * Began as PVSnesLib mapandobjects' enemies by Alekmaul
 *
 * Uses the workspace pattern: objWorkspace is populated before each callback.
 * One 32x32 sprite around a 16x16 collision box, in the machines' palette
 * (OBJ palette 1); its sheet faces right, so it is mirrored walking left.
 */
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>

#define AUTOMATON_LEFT    1
#define AUTOMATON_RIGHT   2
#define AUTOMATON_XVELOC  0x0030    /* 3/16 px a frame */
/* four walk frames in the sheet, one every 6 frames */
#define AUTOMATON_FRAMES  4
#define AUTOMATON_RATE    6
/* where the frame is drawn from the collision box */
#define AUTOMATON_DRAW_DX (-10)
#define AUTOMATON_DRAW_DY (-16)

#include "res/automaton.inc"        /* generated: automaton_tiles[], automaton_pal[] */
extern u16 nbobjects;

u16 automatonnum;
s16 automatonx, automatony;

void automatoninit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx) {
    if (objNew(type, xp, yp) == 0)
        return;
    /* objNew copies the new object to objWorkspace — set fields here */
    objGetPointer(objGetCurrentId());

    objWorkspace.width = 16;
    objWorkspace.height = 16;
    objWorkspace.sprframe = 0;
    objWorkspace.count = 0;
    objWorkspace.dir = AUTOMATON_LEFT;
    objWorkspace.xvel = -AUTOMATON_XVELOC;
    objWorkspace.xmin = minx;
    objWorkspace.xmax = maxx;
    objWorkspace.sprnum = nbobjects;

    /* Set up dynamic sprite: a large (32x32) slot, palette 1, facing left */
    oambuffer[nbobjects].oamx = xp + AUTOMATON_DRAW_DX;
    oambuffer[nbobjects].oamy = yp + AUTOMATON_DRAW_DY;
    oambuffer[nbobjects].oamframeid = 0;
    oambuffer[nbobjects].oamrefresh = 1;
    oambuffer[nbobjects].oamattribute = OBJ_PAL(1) | OBJ_PRIO(2) | OBJ_FLIPX;
    OAM_SET_GFX(nbobjects, automaton_tiles);

    nbobjects++;
}

void automatonupdate(u16 idx) {
    /* objWorkspace is pre-populated by the engine */
    automatonx = objWorkspace.xpos[1] | (objWorkspace.xpos[2] << 8);
    automatonnum = objWorkspace.sprnum;

    objWorkspace.count++;
    if (objWorkspace.count >= AUTOMATON_RATE) {
        objWorkspace.count = 0;
        objWorkspace.sprframe = (objWorkspace.sprframe + 1) & (AUTOMATON_FRAMES - 1);
        oambuffer[automatonnum].oamframeid = objWorkspace.sprframe;
        oambuffer[automatonnum].oamrefresh = 1;
    }

    if (objWorkspace.dir == AUTOMATON_LEFT) {
        if (automatonx <= (s16)objWorkspace.xmin) {
            objWorkspace.dir = AUTOMATON_RIGHT;
            objWorkspace.xvel = +AUTOMATON_XVELOC;
            oambuffer[automatonnum].oamattribute = OBJ_PAL(1) | OBJ_PRIO(2);
        }
    } else {
        if (automatonx >= (s16)objWorkspace.xmax) {
            objWorkspace.dir = AUTOMATON_LEFT;
            objWorkspace.xvel = -AUTOMATON_XVELOC;
            oambuffer[automatonnum].oamattribute = OBJ_PAL(1) | OBJ_PRIO(2) | OBJ_FLIPX;
        }
    }
    objUpdateXY(idx);

    /* Update sprite screen position */
    automatony = (objWorkspace.ypos[1] | (objWorkspace.ypos[2] << 8)) - mapGetCameraY();
    automatonx = automatonx - mapGetCameraX();
    oambuffer[automatonnum].oamx = automatonx + AUTOMATON_DRAW_DX;
    oambuffer[automatonnum].oamy = automatony + AUTOMATON_DRAW_DY;
    oamDynamicDraw(automatonnum);
}
