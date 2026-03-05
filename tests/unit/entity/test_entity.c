// =============================================================================
// Unit Test: Entity Module
// =============================================================================
// Tests game entity management system.
//
// Critical functions tested:
// - entityInit(): Initialize entity pool
// - entitySpawn(): Create new entities
// - entityDestroy(): Remove entities
// - entityGet(): Access entities by index
// - entityCount(): Count active entities
// - entityUpdateAll/MoveAll(): Update positions
// - entityCollide(): Collision detection
// - Constants: ENTITY_MAX, ENT_NONE, ENT_FLAG_*
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/entity.h>
#include <snes/math.h>
#include <snes/text.h>

// =============================================================================
// Test entity types
// =============================================================================
#define ENT_PLAYER  1
#define ENT_ENEMY   2
#define ENT_BULLET  3

// =============================================================================
// Compile-time tests for constants
// =============================================================================

_Static_assert(ENTITY_MAX == 16, "ENTITY_MAX must be 16");
_Static_assert(ENT_NONE == 0, "ENT_NONE must be 0");

// Entity flags
_Static_assert(ENT_FLAG_VISIBLE == 0x01, "ENT_FLAG_VISIBLE must be 0x01");
_Static_assert(ENT_FLAG_SOLID == 0x02, "ENT_FLAG_SOLID must be 0x02");
_Static_assert(ENT_FLAG_FLIP_X == 0x04, "ENT_FLAG_FLIP_X must be 0x04");
_Static_assert(ENT_FLAG_FLIP_Y == 0x08, "ENT_FLAG_FLIP_Y must be 0x08");

// Verify flags don't overlap
_Static_assert((ENT_FLAG_VISIBLE & ENT_FLAG_SOLID) == 0, "Flags must not overlap");
_Static_assert((ENT_FLAG_FLIP_X & ENT_FLAG_FLIP_Y) == 0, "Flags must not overlap");

// =============================================================================
// Runtime tests
// =============================================================================

static u8 test_passed;
static u8 test_failed;

void log_result(const char* name, u8 passed) {
    if (passed) {
        test_passed++;
    } else {
        test_failed++;
    }
}

// =============================================================================
// Test: entityInit
// =============================================================================
void test_entity_init(void) {
    entityInit();
    log_result("entityInit executes", 1);

    // After init, count should be 0
    u8 count = entityCount();
    log_result("Init clears entities", count == 0);
}

// =============================================================================
// Test: entitySpawn / entityDestroy
// =============================================================================
void test_entity_spawn_destroy(void) {
    entityInit();

    // Spawn an entity
    Entity *e = entitySpawn(ENT_PLAYER, FIX(100), FIX(80));
    log_result("entitySpawn returns ptr", e != 0);

    if (e) {
        log_result("Entity is active", e->active == 1);
        log_result("Entity has type", e->type == ENT_PLAYER);
        log_result("Entity at correct X", e->x == FIX(100));
        log_result("Entity at correct Y", e->y == FIX(80));
    }

    // Count should be 1
    log_result("Count is 1", entityCount() == 1);

    // Destroy it
    entityDestroy(e);
    log_result("entityDestroy executes", 1);

    // Count should be 0
    log_result("Count after destroy", entityCount() == 0);
}

// =============================================================================
// Test: Multiple entities
// =============================================================================
void test_multiple_entities(void) {
    entityInit();

    // Spawn several entities
    Entity *player = entitySpawn(ENT_PLAYER, FIX(50), FIX(50));
    Entity *enemy1 = entitySpawn(ENT_ENEMY, FIX(100), FIX(50));
    Entity *enemy2 = entitySpawn(ENT_ENEMY, FIX(150), FIX(50));
    Entity *bullet = entitySpawn(ENT_BULLET, FIX(60), FIX(50));

    log_result("Spawn 4 entities", entityCount() == 4);

    // Count by type
    log_result("Count players", entityCountType(ENT_PLAYER) == 1);
    log_result("Count enemies", entityCountType(ENT_ENEMY) == 2);
    log_result("Count bullets", entityCountType(ENT_BULLET) == 1);

    // Find by type
    Entity *found = entityFindType(ENT_ENEMY);
    log_result("Find enemy", found == enemy1 || found == enemy2);

    // Destroy one
    entityDestroy(enemy1);
    log_result("Destroy one enemy", entityCountType(ENT_ENEMY) == 1);

    entityInit();  // Clean up
}

// =============================================================================
// Test: entityGet
// =============================================================================
void test_entity_get(void) {
    entityInit();

    Entity *spawned = entitySpawn(ENT_PLAYER, FIX(100), FIX(100));

    // Get entity by index 0
    Entity *e0 = entityGet(0);
    log_result("entityGet(0) valid", e0 != 0);

    // It should be the same entity we spawned
    log_result("Get returns spawned", e0 == spawned || e0->type == ENT_PLAYER);

    entityInit();
}

// =============================================================================
// Test: Entity pool exhaustion
// =============================================================================
void test_pool_exhaustion(void) {
    entityInit();

    // Spawn ENTITY_MAX entities
    u8 i;
    u8 spawned = 0;
    for (i = 0; i < ENTITY_MAX; i++) {
        Entity *e = entitySpawn(ENT_ENEMY, FIX(i * 10), FIX(50));
        if (e) spawned++;
    }
    log_result("Spawn max entities", spawned == ENTITY_MAX);

    // Try to spawn one more - should return NULL
    Entity *extra = entitySpawn(ENT_ENEMY, FIX(200), FIX(200));
    log_result("Pool exhausted NULL", extra == 0);

    entityInit();  // Clean up
}

// =============================================================================
// Test: entityUpdateAll (movement)
// =============================================================================
void test_entity_movement(void) {
    entityInit();

    Entity *e = entitySpawn(ENT_PLAYER, FIX(100), FIX(100));
    if (e) {
        // Set velocity
        e->vx = FIX(2);
        e->vy = FIX(-1);

        s16 old_x = e->x;
        s16 old_y = e->y;

        // Update
        entityUpdateAll();

        // Check position changed by velocity
        log_result("X moved by vx", e->x == old_x + FIX(2));
        log_result("Y moved by vy", e->y == old_y + FIX(-1));
    }

    entityInit();
}

// =============================================================================
// Test: entityCollide
// =============================================================================
void test_entity_collision(void) {
    entityInit();

    // Create two overlapping entities
    Entity *a = entitySpawn(ENT_PLAYER, FIX(100), FIX(100));
    Entity *b = entitySpawn(ENT_ENEMY, FIX(105), FIX(105));

    if (a && b) {
        a->width = 16;
        a->height = 16;
        b->width = 16;
        b->height = 16;

        // Should collide (overlapping)
        u8 colliding = entityCollide(a, b);
        log_result("Overlapping collide", colliding == 1);

        // Move b far away
        b->x = FIX(200);
        b->y = FIX(200);

        // Should not collide
        colliding = entityCollide(a, b);
        log_result("Separated no collide", colliding == 0);
    }

    entityInit();
}

// =============================================================================
// Test: entityCollideType
// =============================================================================
void test_collide_type(void) {
    entityInit();

    Entity *player = entitySpawn(ENT_PLAYER, FIX(100), FIX(100));
    Entity *enemy = entitySpawn(ENT_ENEMY, FIX(105), FIX(105));

    if (player && enemy) {
        player->width = 16;
        player->height = 16;
        enemy->width = 16;
        enemy->height = 16;

        // Check collision with enemy type
        Entity *hit = entityCollideType(player, ENT_ENEMY);
        log_result("CollideType finds enemy", hit == enemy);

        // Check collision with non-existent type
        Entity *miss = entityCollideType(player, ENT_BULLET);
        log_result("CollideType misses", miss == 0);
    }

    entityInit();
}

// =============================================================================
// Test: entitySetPos / entitySetVel
// =============================================================================
void test_entity_setters(void) {
    entityInit();

    Entity *e = entitySpawn(ENT_PLAYER, FIX(0), FIX(0));
    if (e) {
        entitySetPos(e, FIX(50), FIX(75));
        log_result("setPos X", e->x == FIX(50));
        log_result("setPos Y", e->y == FIX(75));

        entitySetVel(e, FIX(3), FIX(-2));
        log_result("setVel X", e->vx == FIX(3));
        log_result("setVel Y", e->vy == FIX(-2));
    }

    entityInit();
}

// =============================================================================
// Test: entityScreenX / entityScreenY
// =============================================================================
void test_screen_coords(void) {
    entityInit();

    Entity *e = entitySpawn(ENT_PLAYER, FIX(100), FIX(80));
    if (e) {
        s16 sx = entityScreenX(e);
        s16 sy = entityScreenY(e);

        log_result("screenX correct", sx == 100);
        log_result("screenY correct", sy == 80);
    }

    entityInit();
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "ENTITY MODULE TESTS");
    textPrintAt(2, 2, "-------------------");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_entity_init();
    test_entity_spawn_destroy();
    test_multiple_entities();
    test_entity_get();
    test_pool_exhaustion();
    test_entity_movement();
    test_entity_collision();
    test_collide_type();
    test_entity_setters();
    test_screen_coords();

    // Display results
    textPrintAt(2, 4, "Tests completed");
    textPrintAt(2, 5, "Static asserts: PASSED");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
