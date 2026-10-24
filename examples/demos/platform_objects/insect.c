/*
 * Insect: the robot insect: a walker that patrols between two x and turns at each end
 * Began as PVSnesLib mapandobjects' enemies by Alekmaul
 *
 * Uses the workspace pattern: objWorkspace is populated before each callback.
 * One 32x32 sprite around a 16x16 collision box, in the machines' palette
 * (OBJ palette 1); its sheet faces right, so it is mirrored walking left.
 */
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>

#define INSECT_LEFT    1
#define INSECT_RIGHT   2
#define INSECT_XVELOC  0x0040    /* 1/4 px a frame */
/* four walk frames in the sheet, one every 6 frames */
#define INSECT_FRAMES  4
#define INSECT_RATE    6
/* where the frame is drawn from the collision box */
#define INSECT_DRAW_DX (-8)
#define INSECT_DRAW_DY (-14)

#include "res/insect.inc"        /* generated: insect_tiles[], insect_pal[] */
extern u16 nbobjects;

u16 insectnum;
s16 insectx, insecty;

void insectinit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx) {
    if (objNew(type, xp, yp) == 0)
        return;
    /* objNew copies the new object to objWorkspace — set fields here */
    objGetPointer(objGetCurrentId());

    objWorkspace.width = 16;
    objWorkspace.height = 16;
    objWorkspace.sprframe = 0;
    objWorkspace.count = 0;
    objWorkspace.dir = INSECT_LEFT;
    objWorkspace.xvel = -INSECT_XVELOC;
    objWorkspace.xmin = minx;
    objWorkspace.xmax = maxx;
    objWorkspace.sprnum = nbobjects;

    /* Set up dynamic sprite: a large (32x32) slot, palette 1, facing left */
    oambuffer[nbobjects].oamx = xp + INSECT_DRAW_DX;
    oambuffer[nbobjects].oamy = yp + INSECT_DRAW_DY;
    oambuffer[nbobjects].oamframeid = 0;
    oambuffer[nbobjects].oamrefresh = 1;
    oambuffer[nbobjects].oamattribute = OBJ_PAL(1) | OBJ_PRIO(2) | OBJ_FLIPX;
    OAM_SET_GFX(nbobjects, insect_tiles);

    nbobjects++;
}

void insectupdate(u16 idx) {
    /* objWorkspace is pre-populated by the engine */
    insectx = objWorkspace.xpos[1] | (objWorkspace.xpos[2] << 8);
    insectnum = objWorkspace.sprnum;

    objWorkspace.count++;
    if (objWorkspace.count >= INSECT_RATE) {
        objWorkspace.count = 0;
        objWorkspace.sprframe = (objWorkspace.sprframe + 1) & (INSECT_FRAMES - 1);
        oambuffer[insectnum].oamframeid = objWorkspace.sprframe;
        oambuffer[insectnum].oamrefresh = 1;
    }

    if (objWorkspace.dir == INSECT_LEFT) {
        if (insectx <= (s16)objWorkspace.xmin) {
            objWorkspace.dir = INSECT_RIGHT;
            objWorkspace.xvel = +INSECT_XVELOC;
            oambuffer[insectnum].oamattribute = OBJ_PAL(1) | OBJ_PRIO(2);
        }
    } else {
        if (insectx >= (s16)objWorkspace.xmax) {
            objWorkspace.dir = INSECT_LEFT;
            objWorkspace.xvel = -INSECT_XVELOC;
            oambuffer[insectnum].oamattribute = OBJ_PAL(1) | OBJ_PRIO(2) | OBJ_FLIPX;
        }
    }
    objUpdateXY(idx);

    /* Update sprite screen position */
    insecty = (objWorkspace.ypos[1] | (objWorkspace.ypos[2] << 8)) - mapGetCameraY();
    insectx = insectx - mapGetCameraX();
    oambuffer[insectnum].oamx = insectx + INSECT_DRAW_DX;
    oambuffer[insectnum].oamy = insecty + INSECT_DRAW_DY;
    oamDynamicDraw(insectnum);
}
