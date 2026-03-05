/*
 * Mario object management — slope collision demo
 * Based on PVSnesLib slopemario by Nub1604
 *
 * Uses the workspace pattern: objWorkspace is populated before each callback.
 * Key difference from mapandobjects: objCollidMapWithSlopes for diagonal terrain.
 */
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>

#define MARIO_MAXACCEL  0x0140
#define MARIO_ACCEL     0x0038
#define MARIO_JUMPING   0x0394
#define MARIO_HIJUMPING 0x0594

extern u8 mariogfx;
extern u8 mariopal;

u16 pad0;
u16 marioid;
u16 mariox, marioy;
u8 mariofidx, marioflp, flip;

static void mariowalk(void) {
    flip++;
    if ((flip & 3) == 3) {
        mariofidx++;
        mariofidx = mariofidx & 1;
        /* Walk frames: marioflp(2) + mariofidx(0/1) → frames 2,3 */
        oambuffer[0].oamframeid = marioflp + mariofidx;
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
        oambuffer[0].oamframeid = 6;
        oambuffer[0].oamrefresh = 1;
    }
}

static void mariojump(void) {
    if (oambuffer[0].oamframeid != 1) {
        oambuffer[0].oamframeid = 1;
        oambuffer[0].oamrefresh = 1;
    }

    if (objWorkspace.yvel >= 0)
        objWorkspace.action = ACT_FALL;
}

void marioinit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx) {
    if (objNew(type, xp, yp) == 0)
        return;

    /* objNew copies the new object to objWorkspace */
    objGetPointer(objgetid);
    marioid = objgetid;

    /* Width 14 + xofs 1 is critical for slope collision (width 16 bugs) */
    objWorkspace.width = 14;
    objWorkspace.xofs = 1;
    objWorkspace.height = 16;

    mariofidx = 0;
    marioflp = 0;
    objWorkspace.action = ACT_STAND;

    /* Dynamic sprite: standing frame (6) */
    oambuffer[0].oamx = xp;
    oambuffer[0].oamy = yp;
    oambuffer[0].oamframeid = 6;
    oambuffer[0].oamrefresh = 1;
    oambuffer[0].oamattribute = 0x60 | (0 << 1);  /* priority 3, palette 0 */
    OAM_SET_GFX(0, &mariogfx);

    /* Sprite palette at CGRAM 128 (sprite palette 0) */
    dmaCopyCGram(&mariopal, 128, 16 * 2);
}

void marioupdate(u16 idx) {
    pad0 = padHeld(0);

    if (pad0 & (KEY_RIGHT | KEY_LEFT | KEY_A)) {
        if (pad0 & KEY_LEFT) {
            if ((marioflp > 3) || (marioflp < 2))
                marioflp = 2;
            oambuffer[0].oamattribute &= ~0x40;  /* no H-flip */

            objWorkspace.action = ACT_WALK;
            objWorkspace.xvel -= MARIO_ACCEL;
            if (objWorkspace.xvel <= (s16)(-MARIO_MAXACCEL))
                objWorkspace.xvel = (s16)(-MARIO_MAXACCEL);
        }
        if (pad0 & KEY_RIGHT) {
            if ((marioflp > 3) || (marioflp < 2))
                marioflp = 2;
            oambuffer[0].oamattribute |= 0x40;   /* H-flip */

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

    /* Collision with map tiles including slopes */
    objCollidMapWithSlopes(idx);

    /* Animation state machine */
    if (objWorkspace.action == ACT_WALK)
        mariowalk();
    else if (objWorkspace.action == ACT_FALL)
        mariofall();
    else if (objWorkspace.action == ACT_JUMP)
        mariojump();

    /* Update position from velocity */
    objUpdateXY(idx);

    /* Boundary checks */
    mariox = objWorkspace.xpos[1] | (objWorkspace.xpos[2] << 8);
    marioy = objWorkspace.ypos[1] | (objWorkspace.ypos[2] << 8);
    if (mariox == 0) {
        objWorkspace.xpos[1] = 0;
        objWorkspace.xpos[2] = 0;
    }
    if (marioy == 0) {
        objWorkspace.ypos[1] = 0;
        objWorkspace.ypos[2] = 0;
    }

    /* Update screen position relative to camera */
    oambuffer[0].oamx = mariox - x_pos;
    oambuffer[0].oamy = marioy - y_pos;
    oamDynamic16Draw(0);

    mapUpdateCamera(mariox, marioy);
}
