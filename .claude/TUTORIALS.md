# OpenSNES Module Tutorials

Practical tutorials for using OpenSNES library modules.

## Table of Contents

1. [Animation System](#animation-system)
2. [Collision Detection](#collision-detection)
3. [Entity Management](#entity-management)
4. [Combining Modules](#combining-modules)

---

## Animation System

The animation module provides frame-based sprite animation with multiple slots.

### Basic Animation

```c
#include <snes.h>

// 1. Define animation frames (tile indices in your tileset)
u8 walkFrames[] = { 0, 1, 2, 3, 2, 1 };

// 2. Create animation definition
Animation walkAnim = {
    .frames = walkFrames,
    .frameCount = 6,
    .frameDelay = 8,    // 8 VBlanks = 7.5 FPS
    .loop = 1           // Loop forever
};

void main(void) {
    consoleInit();

    // Load sprite graphics...
    oamInitGfxSet(spriteData, spriteSize, 0x0000);

    // 3. Initialize animation slot
    animInit(0, &walkAnim);
    animPlay(0);

    while (1) {
        WaitForVBlank();

        // 4. Update animations each frame
        animUpdate();

        // 5. Get current tile and apply to sprite
        u8 tile = animGetFrame(0);
        oamSet(0, 100, 100, tile, 0, 0, 0, OBJ_SIZE16);
    }
}
```

### Multiple Animations

Use different slots for different game objects:

```c
u8 playerWalkFrames[] = { 0, 1, 2, 3 };
u8 playerIdleFrames[] = { 4, 5 };
u8 enemyFrames[] = { 8, 9, 10, 11 };

Animation playerWalk = { playerWalkFrames, 4, 6, 1 };
Animation playerIdle = { playerIdleFrames, 2, 15, 1 };
Animation enemyAnim = { enemyFrames, 4, 8, 1 };

// Slot 0: Player current animation
// Slot 1: Enemy animation
animInit(0, &playerWalk);
animInit(1, &enemyAnim);
animPlay(0);
animPlay(1);

// Switch player animation based on input
if (padHeld(KEY_LEFT) || padHeld(KEY_RIGHT)) {
    animSetAnim(0, &playerWalk);
} else {
    animSetAnim(0, &playerIdle);
}
```

### Animation Speed Control

```c
// Slow down animation when player crouches
if (padHeld(KEY_DOWN)) {
    animSetSpeed(0, 16);  // Half speed
} else {
    animSetSpeed(0, 8);   // Normal speed
}

// One-shot animations (attack, death)
Animation attackAnim = {
    .frames = attackFrames,
    .frameCount = 4,
    .frameDelay = 4,
    .loop = 0           // Play once
};

animInit(2, &attackAnim);
animPlay(2);

// Check when attack finishes
if (animIsFinished(2)) {
    animSetAnim(2, &playerIdle);
    animPlay(2);
}
```

### Frame Delay Reference

| frameDelay | FPS | Use Case |
|------------|-----|----------|
| 1 | 60 | Very fast (effects) |
| 4 | 15 | Fast action |
| 6 | 10 | Standard walk |
| 8 | 7.5 | Relaxed |
| 12 | 5 | Slow idle |
| 30 | 2 | Very slow |

---

## Collision Detection

The collision module provides rectangle and tile-based collision.

### Rectangle Collision (AABB)

```c
#include <snes.h>

void main(void) {
    // Define collision boxes
    Rect player = { 100, 100, 16, 16 };  // x, y, width, height
    Rect enemy = { 150, 100, 16, 16 };

    while (1) {
        WaitForVBlank();
        padUpdate();

        // Move player
        if (padHeld(KEY_LEFT))  player.x--;
        if (padHeld(KEY_RIGHT)) player.x++;
        if (padHeld(KEY_UP))    player.y--;
        if (padHeld(KEY_DOWN))  player.y++;

        // Check collision
        if (collideRect(&player, &enemy)) {
            // Flash screen red on collision
            setColor(0, RGB(31, 0, 0));
        } else {
            setColor(0, RGB(0, 0, 0));
        }
    }
}
```

### Collision Response (Pushing Apart)

```c
Rect player, wall;
s16 overlapX, overlapY;

if (collideRectEx(&player, &wall, &overlapX, &overlapY)) {
    // Push player out of wall using minimum displacement
    if (abs(overlapX) < abs(overlapY)) {
        player.x -= overlapX;  // Push horizontally
    } else {
        player.y -= overlapY;  // Push vertically
    }
}
```

### Point Collision

```c
// Check if cursor/bullet hits target
s16 bulletX = 120, bulletY = 80;
Rect target = { 100, 60, 32, 32 };

if (collidePoint(bulletX, bulletY, &target)) {
    // Bullet hit target
    score += 100;
}
```

### Tile-Based Collision (Platformers)

```c
// Collision map: 0 = empty, non-zero = solid
// 32 tiles wide, any height
u8 collisionMap[32 * 28] = {
    // ... level data ...
};

// Check if player feet touch ground
s16 feetX = player.x + 8;   // Center of sprite
s16 feetY = player.y + 16;  // Bottom of sprite

if (collideTile(feetX, feetY, collisionMap, 32)) {
    onGround = 1;
    player.vy = 0;
} else {
    onGround = 0;
    player.vy += GRAVITY;  // Apply gravity
}

// Check head collision (for jumping)
if (player.vy < 0) {
    s16 headY = player.y;
    if (collideTile(feetX, headY, collisionMap, 32)) {
        player.vy = 0;  // Bonk head
    }
}
```

### Platformer Movement Pattern

```c
void updatePlayer(void) {
    s16 newX = player.x;
    s16 newY = player.y;

    // Horizontal movement
    if (padHeld(KEY_LEFT))  newX -= 2;
    if (padHeld(KEY_RIGHT)) newX += 2;

    // Check horizontal collision
    if (!collideTile(newX + 8, player.y + 8, map, 32)) {
        player.x = newX;
    }

    // Vertical movement (gravity/jump)
    newY += player.vy;

    // Check vertical collision
    if (collideTile(player.x + 8, newY + 16, map, 32)) {
        if (player.vy > 0) {
            // Landing
            player.vy = 0;
            onGround = 1;
            // Snap to tile
            newY = (newY / 8) * 8;
        }
    } else {
        onGround = 0;
    }

    player.y = newY;

    // Jump
    if (onGround && padPressed(KEY_A)) {
        player.vy = -6;  // Jump velocity
    }

    // Apply gravity
    if (player.vy < 8) {
        player.vy++;
    }
}
```

---

## Entity Management

The entity module manages pools of game objects.

### Basic Entity Usage

```c
#include <snes.h>

// Define entity types
#define ENT_PLAYER  1
#define ENT_ENEMY   2
#define ENT_BULLET  3

Entity *player;

void main(void) {
    consoleInit();

    // Initialize entity system
    entityInit();

    // Spawn player
    player = entitySpawn(ENT_PLAYER, FIX(100), FIX(100));
    player->width = 16;
    player->height = 16;
    player->spriteId = 0;
    player->tile = 0;
    player->palette = 0;
    player->flags = ENT_FLAG_VISIBLE;

    while (1) {
        WaitForVBlank();
        padUpdate();

        // Update all entities (applies velocity)
        entityUpdateAll();

        // Draw all entities to OAM
        entityDrawAll();
        oamUpdate();
    }
}
```

### Fixed-Point Positions

Entity positions use 8.8 fixed-point for sub-pixel movement:

```c
// FIX() converts integer to fixed-point
entity->x = FIX(100);     // X = 100.0
entity->vx = FIX(1);      // Move 1 pixel/frame = 60 pixels/sec
entity->vx = FIX(1) / 2;  // Move 0.5 pixel/frame = 30 pixels/sec

// UNFIX() converts fixed-point to integer (for display)
s16 screenX = UNFIX(entity->x);

// Or use helper
s16 screenX = entityScreenX(entity);
```

### Spawning Enemies

```c
void spawnEnemy(s16 x, s16 y) {
    Entity *e = entitySpawn(ENT_ENEMY, FIX(x), FIX(y));
    if (e) {
        e->width = 16;
        e->height = 16;
        e->spriteId = entityCount();  // Auto-assign sprite
        e->tile = 8;                   // Enemy tile
        e->palette = 1;
        e->health = 3;
        e->vx = FIX(-1);              // Move left
        e->flags = ENT_FLAG_VISIBLE;
    }
}

// Spawn enemies at intervals
u8 spawnTimer = 0;

void updateSpawner(void) {
    spawnTimer++;
    if (spawnTimer >= 60) {  // Every second
        spawnTimer = 0;
        spawnEnemy(256, 100);  // Spawn off-screen right
    }
}
```

### Entity Collision

```c
void checkCollisions(void) {
    // Check player vs all enemies
    Entity *hit = entityCollideType(player, ENT_ENEMY);
    if (hit) {
        player->health--;
        // Knockback
        player->vx = (player->x < hit->x) ? FIX(-4) : FIX(4);
    }

    // Check all bullets vs all enemies
    for (u8 i = 0; i < ENTITY_MAX; i++) {
        Entity *bullet = entityGet(i);
        if (bullet->active && bullet->type == ENT_BULLET) {
            Entity *enemy = entityCollideType(bullet, ENT_ENEMY);
            if (enemy) {
                enemy->health--;
                if (enemy->health == 0) {
                    entityDestroy(enemy);
                    score += 100;
                }
                entityDestroy(bullet);
            }
        }
    }
}
```

### Shooting Bullets

```c
void playerShoot(void) {
    Entity *bullet = entitySpawn(ENT_BULLET, player->x, player->y);
    if (bullet) {
        bullet->width = 4;
        bullet->height = 4;
        bullet->tile = 16;
        bullet->vx = (player->flags & ENT_FLAG_FLIP_X) ? FIX(-8) : FIX(8);
        bullet->timer = 60;  // Destroy after 1 second
        bullet->flags = ENT_FLAG_VISIBLE;
    }
}

// In update loop
void updateBullets(void) {
    for (u8 i = 0; i < ENTITY_MAX; i++) {
        Entity *e = entityGet(i);
        if (e->active && e->type == ENT_BULLET) {
            // Timer counts down automatically
            if (e->timer == 0) {
                entityDestroy(e);
            }
            // Or destroy if off-screen
            s16 sx = entityScreenX(e);
            if (sx < -8 || sx > 264) {
                entityDestroy(e);
            }
        }
    }
}
```

### Entity State Machine

```c
#define STATE_IDLE    0
#define STATE_WALK    1
#define STATE_ATTACK  2
#define STATE_HURT    3

void updateEnemy(Entity *e) {
    switch (e->state) {
        case STATE_IDLE:
            e->vx = 0;
            e->timer--;
            if (e->timer == 0) {
                e->state = STATE_WALK;
                e->vx = FIX(1);
                e->timer = 120;
            }
            break;

        case STATE_WALK:
            e->timer--;
            if (e->timer == 0) {
                e->state = STATE_IDLE;
                e->timer = 60;
            }
            // Turn around at edges
            s16 sx = entityScreenX(e);
            if (sx < 16 || sx > 224) {
                e->vx = -e->vx;
                e->flags ^= ENT_FLAG_FLIP_X;
            }
            break;

        case STATE_HURT:
            e->timer--;
            if (e->timer == 0) {
                e->state = STATE_IDLE;
            }
            break;
    }
}
```

---

## Combining Modules

### Complete Game Object

Combine animation, collision, and entity for a full game object:

```c
#include <snes.h>

#define ENT_PLAYER 1
#define ENT_ENEMY  2

// Animations
u8 playerWalkFrames[] = { 0, 1, 2, 3 };
u8 playerIdleFrames[] = { 4, 5 };
u8 enemyWalkFrames[] = { 8, 9, 10, 11 };

Animation playerWalk = { playerWalkFrames, 4, 6, 1 };
Animation playerIdle = { playerIdleFrames, 2, 12, 1 };
Animation enemyWalk = { enemyWalkFrames, 4, 8, 1 };

Entity *player;

void main(void) {
    consoleInit();

    // Init systems
    entityInit();
    oamInitGfxSet(spriteData, spriteSize, 0x0000);

    // Create player
    player = entitySpawn(ENT_PLAYER, FIX(100), FIX(100));
    player->width = 16;
    player->height = 16;
    player->spriteId = 0;
    player->palette = 0;
    player->health = 5;
    player->flags = ENT_FLAG_VISIBLE;

    // Player uses animation slot 0
    animInit(0, &playerIdle);
    animPlay(0);

    // Spawn initial enemies
    for (u8 i = 0; i < 3; i++) {
        Entity *e = entitySpawn(ENT_ENEMY, FIX(200 + i * 40), FIX(100));
        if (e) {
            e->width = 16;
            e->height = 16;
            e->spriteId = 1 + i;
            e->palette = 1;
            e->vx = FIX(-1);
            e->flags = ENT_FLAG_VISIBLE;
            // Enemy uses animation slot 1+i
            animInit(1 + i, &enemyWalk);
            animPlay(1 + i);
        }
    }

    while (1) {
        WaitForVBlank();
        padUpdate();

        // Update player movement
        updatePlayerInput();

        // Update all entities
        entityUpdateAll();

        // Update all animations
        animUpdate();

        // Apply animation frames to entities
        player->tile = animGetFrame(0);
        for (u8 i = 0; i < ENTITY_MAX; i++) {
            Entity *e = entityGet(i);
            if (e->active && e->type == ENT_ENEMY) {
                e->tile = animGetFrame(1 + i);  // Each enemy has its slot
            }
        }

        // Check collisions
        checkPlayerCollisions();

        // Draw
        entityDrawAll();
        oamUpdate();
    }
}

void updatePlayerInput(void) {
    u8 moving = 0;

    if (padHeld(KEY_LEFT)) {
        player->vx = FIX(-2);
        player->flags |= ENT_FLAG_FLIP_X;
        moving = 1;
    } else if (padHeld(KEY_RIGHT)) {
        player->vx = FIX(2);
        player->flags &= ~ENT_FLAG_FLIP_X;
        moving = 1;
    } else {
        player->vx = 0;
    }

    // Switch animation based on movement
    if (moving) {
        animSetAnim(0, &playerWalk);
    } else {
        animSetAnim(0, &playerIdle);
    }
}

void checkPlayerCollisions(void) {
    Entity *hit = entityCollideType(player, ENT_ENEMY);
    if (hit && player->timer == 0) {
        player->health--;
        player->timer = 60;  // Invincibility frames

        // Knockback
        if (player->x < hit->x) {
            player->vx = FIX(-4);
        } else {
            player->vx = FIX(4);
        }
    }
}
```

### Memory Considerations

- **Animation slots**: 32 available, use sparingly
- **Entity pool**: 16 entities (can increase ENTITY_MAX)
- **Sprite limit**: 128 OAM entries, 32 per scanline
- **Fixed-point**: Use `s16` for positions, stays within 16-bit range

### Performance Tips

1. **Batch operations**: `entityUpdateAll()` is faster than individual updates
2. **Early out**: Check `e->active` before processing entities
3. **Limit collision checks**: Only check relevant types
4. **Reuse slots**: Destroy old entities before spawning new ones

---

## See Also

- `lib/include/snes/animation.h` - Full API reference
- `lib/include/snes/collision.h` - Full API reference
- `lib/include/snes/entity.h` - Full API reference
- `examples/graphics/13_animation_system/` - Animation example
- `examples/basics/3_collision_demo/` - Collision example
- `examples/game/1_entity_demo/` - Entity example
