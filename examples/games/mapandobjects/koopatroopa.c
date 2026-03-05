/*
 * Koopa Troopa object management (2-sprite composite)
 * Based on PVSnesLib mapandobjects by Alekmaul
 *
 * Uses the workspace pattern: objWorkspace is populated before each callback.
 * Functions split to keep frame sizes under 255 bytes (65816 stack limit).
 */
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>

#define KOOPATROOPA_LEFT    1
#define KOOPATROOPA_RIGHT   2
#define KOOPATROOPA_XVELOC  0x028A

extern u8 sprkoopatroopa;
extern u16 nbobjects;

u16 koopatroopanum;
s16 koopatroopax, koopatroopay;

static void koopatroopa_setup_sprites(u16 xp, u16 yp) {
    u16 n = nbobjects;

    oambuffer[n].oamx = xp;
    oambuffer[n].oamy = yp - 16;
    oambuffer[n].oamframeid = 0;
    oambuffer[n].oamrefresh = 1;
    oambuffer[n].oamattribute = 0x20 | (0 << 1);
    OAM_SET_GFX(n, &sprkoopatroopa);
    nbobjects++;
    n++;

    oambuffer[n].oamx = xp;
    oambuffer[n].oamy = yp;
    oambuffer[n].oamframeid = 1;
    oambuffer[n].oamrefresh = 1;
    oambuffer[n].oamattribute = 0x20 | (0 << 1);
    OAM_SET_GFX(n, &sprkoopatroopa);
    nbobjects++;
}

void koopatroopainit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx) {
    if (objNew(type, xp, yp) == 0)
        return;

    objGetPointer(objgetid);
    objWorkspace.width = 16;
    objWorkspace.height = 16;
    objWorkspace.sprframe = 0;
    objWorkspace.sprflip = 0;
    objWorkspace.count = 10;
    objWorkspace.dir = KOOPATROOPA_LEFT;
    objWorkspace.xvel = -KOOPATROOPA_XVELOC;
    objWorkspace.xmin = minx;
    objWorkspace.xmax = maxx;
    objWorkspace.sprnum = nbobjects;

    koopatroopa_setup_sprites(xp, yp);
}

/* Animation and AI logic — split from update to reduce frame size */
static void koopatroopa_animate(u16 idx) {
    u16 n = koopatroopanum;

    objWorkspace.count = 0;
    objWorkspace.sprframe = (2 - objWorkspace.sprframe);
    oambuffer[n].oamframeid = objWorkspace.sprframe;
    oambuffer[n].oamrefresh = 1;
    oambuffer[n + 1].oamframeid = objWorkspace.sprframe + 1;
    oambuffer[n + 1].oamrefresh = 1;

    if (objWorkspace.dir == KOOPATROOPA_LEFT) {
        if (koopatroopax <= (s16)objWorkspace.xmin) {
            objWorkspace.dir = KOOPATROOPA_RIGHT;
            objWorkspace.sprflip = 1;
            objWorkspace.xvel = +KOOPATROOPA_XVELOC;
            objWorkspace.yvel = 0;
            oambuffer[n].oamattribute |= 0x40;
            oambuffer[n + 1].oamattribute |= 0x40;
        } else {
            objWorkspace.xvel = -KOOPATROOPA_XVELOC;
        }
    } else {
        if (koopatroopax >= (s16)objWorkspace.xmax) {
            objWorkspace.dir = KOOPATROOPA_LEFT;
            objWorkspace.sprflip = 0;
            objWorkspace.xvel = -KOOPATROOPA_XVELOC;
            objWorkspace.yvel = 0;
            oambuffer[n].oamattribute &= ~0x40;
            oambuffer[n + 1].oamattribute &= ~0x40;
        } else {
            objWorkspace.xvel = +KOOPATROOPA_XVELOC;
        }
    }

    objUpdateXY(idx);
}

/* Sprite position update — split from update to reduce frame size */
static void koopatroopa_draw(void) {
    u16 n = koopatroopanum;

    koopatroopay = (objWorkspace.ypos[1] | (objWorkspace.ypos[2] << 8)) - y_pos;
    koopatroopax = koopatroopax - x_pos;

    oambuffer[n].oamx = koopatroopax;
    oambuffer[n].oamy = koopatroopay - 16;
    oamDynamic16Draw(n);

    n++;
    oambuffer[n].oamx = koopatroopax;
    oambuffer[n].oamy = koopatroopay;
    oamDynamic16Draw(n);
}

void koopatroopaupdate(u16 idx) {
    koopatroopax = objWorkspace.xpos[1] | (objWorkspace.xpos[2] << 8);
    koopatroopanum = objWorkspace.sprnum;

    objWorkspace.count++;
    if (objWorkspace.count >= 3) {
        koopatroopa_animate(idx);
    }

    koopatroopa_draw();
}
