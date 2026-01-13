# Game Template

A complete game template with proper architecture for SNES games.

## Features

- **State Management**: Title screen, playing, paused states
- **Sprite System**: OAM buffer management with add/clear/update
- **Input Handling**: Edge detection (pressed vs held)
- **Background**: Tile-based background with scrolling
- **Player**: Movable player with boundary checking

## Quick Start

```bash
# Copy template
cp -r templates/game myproject
cd myproject

# Edit Makefile - set your project name
# Edit main.c - add your game code

# Build
make

# Run
make run
```

## Project Structure

```
game/
├── main.c      # All game code (expand into multiple files as needed)
├── Makefile    # Build configuration
└── README.md   # This file
```

## Architecture

### Game States

```c
#define STATE_TITLE   0
#define STATE_PLAYING 1
#define STATE_PAUSED  2
#define STATE_GAMEOVER 3
```

Each state has its own handler function. Add more states as needed.

### Input System

```c
update_input();              // Call once per frame
u16 pressed = joy_pressed(); // Just pressed this frame
u16 held = joy_held();       // Currently held
```

### OAM (Sprite) System

```c
oam_clear();                          // Start fresh each frame
oam_add_sprite(x, y, tile, attr, 0);  // Add 8x8 sprite
oam_add_sprite(x, y, tile, attr, 1);  // Add 16x16 sprite
oam_update();                         // Write to hardware
```

## Extending the Template

### Adding Player Animation

```c
static u8 player_frame;
static u8 player_anim_timer;

void update_player(void) {
    // Animation
    player_anim_timer++;
    if (player_anim_timer >= 8) {
        player_anim_timer = 0;
        player_frame++;
        if (player_frame >= 4) player_frame = 0;
    }
    // ... movement code
}

void draw_player(void) {
    u8 tile = player_frame * 4;  // 4 tiles per frame
    oam_add_sprite(player_x, player_y, tile, 0x30, 1);
}
```

### Adding Enemies

```c
#define MAX_ENEMIES 8

struct Enemy {
    u16 x, y;
    u8 type;
    u8 active;
};

static struct Enemy enemies[MAX_ENEMIES];

void update_enemies(void) {
    u8 i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        // Update enemy logic
    }
}

void draw_enemies(void) {
    u8 i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        oam_add_sprite(enemies[i].x, enemies[i].y, 8, 0x31, 0);
    }
}
```

### Adding Collision

```c
u8 check_collision(u16 ax, u8 ay, u8 aw, u8 ah,
                   u16 bx, u8 by, u8 bw, u8 bh) {
    if (ax + aw <= bx) return 0;
    if (ax >= bx + bw) return 0;
    if (ay + ah <= by) return 0;
    if (ay >= by + bh) return 0;
    return 1;
}
```

## Memory Usage

| Section | Usage |
|---------|-------|
| ROM | ~2KB (expandable) |
| RAM | ~1KB (variables + OAM buffer) |
| VRAM | BG tiles: $0000, Sprites: $4000 |

## Controls (Default)

| Button | Action |
|--------|--------|
| D-Pad | Move player |
| Start | Pause/Resume, Start game |

## Next Steps

1. **Add graphics**: Replace placeholder tiles with real art
2. **Add sound**: See `examples/audio/` for sound integration
3. **Add levels**: Create level data structures
4. **Add enemies**: Use the enemy pattern above
5. **Split files**: Move player.c, enemy.c, level.c as project grows
