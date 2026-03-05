/*
 * Mario object management
 * Based on PVSnesLib mapandobjects by Alekmaul
 *
 * Uses the workspace pattern: objWorkspace is populated before each callback.
 */
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>

#define MARIO_MAXACCEL  0x0140
#define MARIO_ACCEL     0x0038
#define MARIO_JUMPING   0x0394
#define MARIO_HIJUMPING 0x0594

extern u8 sprmario;
extern u16 nbobjects;

u16 pad0;
u16 marioid;
u16 mariox, marioy;
u8 mariofidx, marioflp, flip;

static void mariowalk(void) {
    flip++;
    if (flip & 1) {
        mariofidx++;
        mariofidx = mariofidx & 3;
        objWorkspace.sprframe = mariofidx;
        oambuffer[0].oamframeid = mariofidx;
        oambuffer[0].oamrefresh = 1;
    }

    if (objWorkspace.yvel != 0)
        objWorkspace.action = ACT_FALL;
    else if ((objWorkspace.xvel == 0) && (objWorkspace.yvel == 0))
        objWorkspace.action = ACT_STAND;
}

static void mariofall(void) {
    if (objWorkspace.yvel == 0) {
        objWorkspace.action = ACT_STAND;
        oambuffer[0].oamframeid = 0;
        oambuffer[0].oamrefresh = 1;
    }
}

static void mariojump(void) {
    oambuffer[0].oamframeid = 4;
    oambuffer[0].oamrefresh = 1;

    if (objWorkspace.yvel >= 0)
        objWorkspace.action = ACT_FALL;
}

void marioinit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx) {
    if (objNew(type, xp, yp) == 0)
        return;

    /* objNew copies the new object to objWorkspace — set fields here */
    objGetPointer(objgetid);
    marioid = objgetid;
    objWorkspace.width = 16;
    objWorkspace.height = 16;

    mariofidx = 0;
    marioflp = 0;
    objWorkspace.action = ACT_WALK;

    /* Set up dynamic sprite */
    oambuffer[0].oamx = xp;
    oambuffer[0].oamy = yp;
    oambuffer[0].oamframeid = 0;
    oambuffer[0].oamrefresh = 1;
    oambuffer[0].oamattribute = 0x20 | (0 << 1);
    OAM_SET_GFX(0, &sprmario);
}

void marioupdate(u16 idx) {
    /* objWorkspace is pre-populated by the engine */

    pad0 = padHeld(0);

    if (pad0) {
        if (pad0 & KEY_LEFT) {
            marioflp = 1;
            objWorkspace.action = ACT_WALK;
            objWorkspace.xvel -= MARIO_ACCEL;
            if (objWorkspace.xvel <= (s16)(-MARIO_MAXACCEL))
                objWorkspace.xvel = (s16)(-MARIO_MAXACCEL);
        }
        if (pad0 & KEY_RIGHT) {
            marioflp = 0;
            objWorkspace.action = ACT_WALK;
            objWorkspace.xvel += MARIO_ACCEL;
            if (objWorkspace.xvel >= (s16)(MARIO_MAXACCEL))
                objWorkspace.xvel = (s16)(MARIO_MAXACCEL);
        }
        if (pad0 & KEY_A) {
            if (objWorkspace.tilestand != 0) {
                objWorkspace.action = ACT_JUMP;
                if (pad0 & KEY_UP)
                    objWorkspace.yvel = (s16)(-MARIO_HIJUMPING);
                else
                    objWorkspace.yvel = (s16)(-MARIO_JUMPING);
            }
        }
    }

    /* Collision with map tiles (syncs workspace automatically) */
    objCollidMap(idx);

    /* Animation state machine */
    if (objWorkspace.action == ACT_WALK)
        mariowalk();
    else if (objWorkspace.action == ACT_FALL)
        mariofall();
    else if (objWorkspace.action == ACT_JUMP)
        mariojump();

    /* Update position from velocity (syncs workspace automatically) */
    objUpdateXY(idx);

    /* Read updated position from workspace (24-bit: xpos[0]=frac, [1]=low, [2]=high) */
    mariox = objWorkspace.xpos[1] | (objWorkspace.xpos[2] << 8);
    marioy = objWorkspace.ypos[1] | (objWorkspace.ypos[2] << 8);

    oambuffer[0].oamx = mariox - x_pos;
    oambuffer[0].oamy = marioy - y_pos;
    oamDynamic16Draw(0);
    mapUpdateCamera(mariox, marioy);
}
