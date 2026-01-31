/**
 * @file entity.c
 * @brief OpenSNES Entity System Implementation
 *
 * Game object management system.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/entity.h>

/*============================================================================
 * Entity Pool
 *============================================================================*/

static Entity entities[ENTITY_MAX];

/*============================================================================
 * Entity Management Functions
 *============================================================================*/

void entityInit(void) {
    u8 i;
    Entity *e;

    for (i = 0; i < ENTITY_MAX; i++) {
        e = &entities[i];
        e->active = 0;
        e->type = ENT_NONE;
        e->flags = 0;
        e->state = 0;
        e->x = 0;
        e->y = 0;
        e->vx = 0;
        e->vy = 0;
        e->width = 8;
        e->height = 8;
        e->spriteId = i;
        e->tile = 0;
        e->palette = 0;
        e->priority = 2;
        e->health = 1;
        e->timer = 0;
    }
}

Entity *entitySpawn(u8 type, s16 x, s16 y) {
    u8 i;
    Entity *e;

    for (i = 0; i < ENTITY_MAX; i++) {
        if (!entities[i].active) {
            e = &entities[i];
            e->active = 1;
            e->type = type;
            e->flags = ENT_FLAG_VISIBLE;
            e->state = 0;
            e->x = x;
            e->y = y;
            e->vx = 0;
            e->vy = 0;
            e->width = 8;
            e->height = 8;
            e->spriteId = i;
            e->tile = 0;
            e->palette = 0;
            e->priority = 2;
            e->health = 1;
            e->timer = 0;
            return e;
        }
    }
    return 0;
}

void entityDestroy(Entity *e) {
    if (e) {
        e->active = 0;
        e->type = ENT_NONE;
    }
}

Entity *entityGet(u8 index) {
    if (index >= ENTITY_MAX) return 0;
    return &entities[index];
}

Entity *entityFindType(u8 type) {
    u8 i;
    for (i = 0; i < ENTITY_MAX; i++) {
        if (entities[i].active && entities[i].type == type) {
            return &entities[i];
        }
    }
    return 0;
}

u8 entityCount(void) {
    u8 i, count;
    count = 0;
    for (i = 0; i < ENTITY_MAX; i++) {
        if (entities[i].active) count = count + 1;
    }
    return count;
}

u8 entityCountType(u8 type) {
    u8 i, count;
    count = 0;
    for (i = 0; i < ENTITY_MAX; i++) {
        if (entities[i].active && entities[i].type == type) {
            count = count + 1;
        }
    }
    return count;
}

/*============================================================================
 * Entity Update Functions
 *============================================================================*/

void entityUpdateAll(void) {
    u8 i;
    Entity *e;

    for (i = 0; i < ENTITY_MAX; i++) {
        e = &entities[i];
        if (!e->active) continue;

        e->x = e->x + e->vx;
        e->y = e->y + e->vy;

        if (e->timer > 0) {
            e->timer = e->timer - 1;
        }
    }
}

void entityMoveAll(void) {
    u8 i;
    Entity *e;

    for (i = 0; i < ENTITY_MAX; i++) {
        e = &entities[i];
        if (!e->active) continue;
        e->x = e->x + e->vx;
        e->y = e->y + e->vy;
    }
}

/*============================================================================
 * Entity Drawing Functions
 *============================================================================*/

void entityDrawAll(void) {
    u8 i;
    Entity *e;
    u16 sx;
    u8 sy;
    u8 flipFlags;

    for (i = 0; i < ENTITY_MAX; i++) {
        e = &entities[i];

        if (!e->active || !(e->flags & ENT_FLAG_VISIBLE)) {
            oamSetY(e->spriteId, 240);
            continue;
        }

        sx = (u16)(e->x >> 8);
        sy = (u8)(e->y >> 8);

        flipFlags = 0;
        if (e->flags & ENT_FLAG_FLIP_X) flipFlags = flipFlags | 0x40;
        if (e->flags & ENT_FLAG_FLIP_Y) flipFlags = flipFlags | 0x80;

        oamSet(e->spriteId, sx, sy, (u16)e->tile, e->palette, e->priority, flipFlags);
    }
}

void entityHideAll(void) {
    u8 i;
    for (i = 0; i < ENTITY_MAX; i++) {
        oamSetY(entities[i].spriteId, 240);
    }
}

/*============================================================================
 * Entity Collision Functions
 *============================================================================*/

u8 entityCollide(Entity *a, Entity *b) {
    s16 ax2, ay2, bx2, by2;

    if (!a || !b || !a->active || !b->active) return 0;

    ax2 = a->x + (a->width << 8);
    ay2 = a->y + (a->height << 8);
    bx2 = b->x + (b->width << 8);
    by2 = b->y + (b->height << 8);

    if (a->x >= bx2) return 0;
    if (ax2 <= b->x) return 0;
    if (a->y >= by2) return 0;
    if (ay2 <= b->y) return 0;

    return 1;
}

Entity *entityCollideType(Entity *e, u8 type) {
    u8 i;
    Entity *other;

    if (!e || !e->active) return 0;

    for (i = 0; i < ENTITY_MAX; i++) {
        other = &entities[i];
        if (other == e) continue;
        if (!other->active) continue;
        if (other->type != type) continue;
        if (entityCollide(e, other)) return other;
    }
    return 0;
}

u8 entityContainsPoint(Entity *e, s16 px, s16 py) {
    s16 ex2, ey2;

    if (!e || !e->active) return 0;

    ex2 = e->x + (e->width << 8);
    ey2 = e->y + (e->height << 8);

    if (px < e->x || px >= ex2) return 0;
    if (py < e->y || py >= ey2) return 0;

    return 1;
}

/*============================================================================
 * Entity Helper Functions
 *============================================================================*/

void entitySetPos(Entity *e, s16 x, s16 y) {
    if (e) { e->x = x; e->y = y; }
}

void entitySetVel(Entity *e, s16 vx, s16 vy) {
    if (e) { e->vx = vx; e->vy = vy; }
}

s16 entityScreenX(Entity *e) {
    if (!e) return 0;
    return e->x >> 8;
}

s16 entityScreenY(Entity *e) {
    if (!e) return 0;
    return e->y >> 8;
}

void entitySetSprite(Entity *e, u8 spriteId, u8 tile, u8 palette) {
    if (e) {
        e->spriteId = spriteId;
        e->tile = tile;
        e->palette = palette;
    }
}
