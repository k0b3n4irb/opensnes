/*
 * Kit, the player object
 * Began as PVSnesLib mapandobjects' player by Alekmaul
 *
 * Uses the workspace pattern: objWorkspace is populated before each callback.
 * Kit is a 32x32 sprite around a 16x16 collision box: his feet, on row 30
 * of the frame, are the box's bottom.
 */
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>

#define KIT_MAXACCEL  0x0140
#define KIT_ACCEL     0x0038
#define KIT_JUMPING   0x0394
#define KIT_HIJUMPING 0x0594

/* Kit's sheet (res/kit.png, four 32x32 frames a row): idle 0-3, walk 4-11,
 * run 12-19, jump 20-27 */
#define FRAME_IDLE 0
#define FRAME_WALK 4
#define FRAME_RISE 23
#define FRAME_FALL 25
/* where the frame is drawn from the collision box */
#define KIT_DRAW_DX (-8)
#define KIT_DRAW_DY (-14)

#include "res/kit.inc"           /* generated: kit_tiles[], kit_pal[] */
extern u16 nbobjects;

u16 pad0;
u16 kitid;
u16 kitx, kity;
u8 kitfidx, kitflp, flip;

static void kitframe(u16 f) {
    if (oambuffer[0].oamframeid != f) {
        oambuffer[0].oamframeid = f;
        oambuffer[0].oamrefresh = 1;
    }
}

static void kitwalk(void) {
    if ((++flip & 3) == 0) {
        kitfidx = (kitfidx + 1) & 7;
        objWorkspace.sprframe = kitfidx;
        kitframe(FRAME_WALK + kitfidx);
    }

    if (objWorkspace.yvel != 0)
        objWorkspace.action = ACT_FALL;
    else if ((objWorkspace.xvel == 0) && (objWorkspace.yvel == 0))
        objWorkspace.action = ACT_STAND;
}

static void kitfall(void) {
    kitframe(FRAME_FALL);
    if (objWorkspace.yvel == 0) {
        objWorkspace.action = ACT_STAND;
        kitframe(FRAME_IDLE);
    }
}

static void kitjump(void) {
    kitframe(FRAME_RISE);
    if (objWorkspace.yvel >= 0)
        objWorkspace.action = ACT_FALL;
}

void kitinit(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx) {
    if (objNew(type, xp, yp) == 0)
        return;
    /* objNew copies the new object to objWorkspace — set fields here */
    objGetPointer(objGetCurrentId());
    kitid = objGetCurrentId();

    /* 14 wide from x + 1, as PVSnesLib's slope example does: with a width
     * of 16 the box does not follow a slope correctly (observed; the cause
     * is not traced) */
    objWorkspace.width = 14;
    objWorkspace.xofs = 1;
    objWorkspace.height = 16;

    kitfidx = 0;
    kitflp = 0;
    objWorkspace.action = ACT_WALK;

    /* Set up dynamic sprite: a large (32x32) slot */
    oambuffer[0].oamx = xp + KIT_DRAW_DX;
    oambuffer[0].oamy = yp + KIT_DRAW_DY;
    oambuffer[0].oamframeid = FRAME_IDLE;
    oambuffer[0].oamrefresh = 1;
    oambuffer[0].oamattribute = OBJ_PRIO(2);
    OAM_SET_GFX(0, kit_tiles);
}

void kitupdate(u16 idx) {
    /* objWorkspace is pre-populated by the engine */
    pad0 = padHeld(0);

    if (pad0) {
        if (pad0 & KEY_LEFT) {
            kitflp = 1;
            objWorkspace.action = ACT_WALK;
            objWorkspace.xvel -= KIT_ACCEL;
            if (objWorkspace.xvel <= (s16)(-KIT_MAXACCEL))
                objWorkspace.xvel = (s16)(-KIT_MAXACCEL);
        }
        if (pad0 & KEY_RIGHT) {
            kitflp = 0;
            objWorkspace.action = ACT_WALK;
            objWorkspace.xvel += KIT_ACCEL;
            if (objWorkspace.xvel >= (s16)(KIT_MAXACCEL))
                objWorkspace.xvel = (s16)(KIT_MAXACCEL);
        }
        if (pad0 & KEY_A) {
            if (objWorkspace.tilestand != 0) {
                objWorkspace.action = ACT_JUMP;
                if (pad0 & KEY_UP)
                    objWorkspace.yvel = (s16)(-KIT_HIJUMPING);
                else
                    objWorkspace.yvel = (s16)(-KIT_JUMPING);
            }
        }
    }

    /* Collision with map tiles, slopes included (syncs workspace automatically) */
    objCollidMapWithSlopes(idx);

    /* Animation state machine */
    if (objWorkspace.action == ACT_WALK)
        kitwalk();
    else if (objWorkspace.action == ACT_FALL)
        kitfall();
    else if (objWorkspace.action == ACT_JUMP)
        kitjump();

    /* Update position from velocity (syncs workspace automatically) */
    objUpdateXY(idx);

    /* Read updated position from workspace (24-bit: xpos[0]=frac, [1]=low, [2]=high) */
    kitx = objWorkspace.xpos[1] | (objWorkspace.xpos[2] << 8);
    kity = objWorkspace.ypos[1] | (objWorkspace.ypos[2] << 8);

    /* The sheet faces right: mirror it when Kit walks left */
    oambuffer[0].oamattribute = kitflp ? (OBJ_PRIO(2) | OBJ_FLIPX) : OBJ_PRIO(2);
    oambuffer[0].oamx = kitx - mapGetCameraX() + KIT_DRAW_DX;
    oambuffer[0].oamy = kity - mapGetCameraY() + KIT_DRAW_DY;
    oamDynamicDraw(0);

    mapUpdateCamera(kitx, kity);
}
