# OpenSNES Development Roadmap

## Vision Statement

Create the definitive open source SNES development SDK with:
- A clean, modern C compiler (QBE-based)
- Real game templates that developers can build upon
- Professional-grade tooling and documentation

## Phase 1: Foundation (Months 1-3)

### 1.1 Minimal Viable SDK

**Goal**: Get ONE complete game template building and running

#### Tasks:
- [ ] Port essential library routines from PVSnesLib
  - [ ] sprites.asm (sprite management)
  - [ ] backgrounds.asm (tilemap handling)
  - [ ] dma.asm (DMA transfers)
  - [ ] input.asm (controller reading)
  - [ ] crt0.asm (startup code)
- [ ] Set up build system
  - [ ] Use 816-tcc temporarily
  - [ ] Port wla-dx build integration
  - [ ] Create template Makefile
- [ ] Create platformer template
  - [ ] Player movement (run, jump)
  - [ ] Collision detection
  - [ ] Scrolling background
  - [ ] Simple enemy AI
  - [ ] Coin/collectible system

#### Deliverable:
A playable platformer demo that compiles with OpenSNES

### 1.2 Test Infrastructure

**Goal**: Automated testing from day 1

#### Tasks:
- [ ] Set up Mesen2 Lua test runner
- [ ] Create test categories:
  - [ ] Unit tests (arithmetic, memory)
  - [ ] Hardware tests (sprites, backgrounds, audio)
  - [ ] Template smoke tests
- [ ] Build validation script
- [ ] CI/CD pipeline (GitHub Actions)

### 1.3 Documentation

**Goal**: Developers can get started without reading source code

#### Tasks:
- [ ] Getting Started guide
- [ ] API reference (auto-generated from headers)
- [ ] Hardware overview for beginners
- [ ] Template walkthroughs

---

## Phase 2: QBE Compiler (Months 3-9)

### 2.1 QBE Study Phase

**Goal**: Understand QBE internals

#### Tasks:
- [ ] Read QBE source code (~8000 lines)
- [ ] Document QBE IR format
- [ ] Study existing backends (AMD64, ARM64, RISC-V)
- [ ] Document 65816 instruction set mapping

#### Resources:
- QBE source: https://c9x.me/compile/
- QBE paper: https://c9x.me/compile/doc/llvm.html
- 65816 reference: Western Design Center datasheets

### 2.2 65816 Backend - MVP

**Goal**: Compile "Hello World" with QBE

#### Tasks:
- [ ] Define 65816 target in QBE
- [ ] Implement basic instruction selection
  - [ ] Load/store
  - [ ] Arithmetic (add, sub, and, or)
  - [ ] Branches
  - [ ] Function calls
- [ ] Register allocation (A, X, Y, direct page)
- [ ] Generate WLA-DX compatible assembly

#### Technical Challenges:
- 65816 has very few registers
- 16-bit vs 8-bit mode switching
- 24-bit addresses vs 16-bit pointers
- Bank boundary handling

### 2.3 Optimization Passes

**Goal**: Generate code better than 816-tcc

#### Tasks:
- [ ] Direct page allocation for locals
- [ ] Strength reduction (multiply → shifts)
- [ ] Peephole optimization
  - [ ] Eliminate redundant rep/sep
  - [ ] Combine load-modify-store sequences
  - [ ] Remove dead stores
- [ ] Tail call optimization
- [ ] Inline small functions

### 2.4 Integration

**Goal**: Replace 816-tcc with QBE backend

#### Tasks:
- [ ] Create qbe-65816 frontend driver
- [ ] Update build system
- [ ] Run full test suite
- [ ] Benchmark against 816-tcc
- [ ] Fix regressions

---

## Phase 3: Game Engine (Months 9-15)

### 3.1 Component System

**Goal**: Entity-Component architecture for games

```c
// Example API
Entity player = entity_create();
entity_add(player, Position, (Position){100, 100});
entity_add(player, Velocity, (Velocity){0, 0});
entity_add(player, Sprite, (Sprite){0, 0, 16, 16});

void system_movement(void) {
    for_each(Position, Velocity) {
        pos->x += vel->x;
        pos->y += vel->y;
    }
}
```

#### Tasks:
- [ ] Design component memory layout
- [ ] Implement entity manager
- [ ] Create common systems:
  - [ ] Movement
  - [ ] Collision
  - [ ] Animation
  - [ ] Camera
- [ ] Profile and optimize

### 3.2 Asset Pipeline

**Goal**: YAML-based asset definitions

```yaml
# assets.yaml
sprites:
  player:
    source: player.png
    size: 16x16
    animations:
      idle: [0, 1, 2, 1]
      run: [3, 4, 5, 6, 7, 8]
      jump: [9]

tilemaps:
  level1:
    source: level1.tmx
    collision_layer: collision
```

#### Tasks:
- [ ] YAML parser for asset manifest
- [ ] Auto-generate .asm data files
- [ ] Hot reload during development
- [ ] Compression for release builds

### 3.3 Additional Templates

**Goal**: Three complete game templates

#### Platformer (from Phase 1):
- [ ] Polish existing template
- [ ] Add level editor integration
- [ ] Multiple enemies
- [ ] Boss fight example

#### RPG Template:
- [ ] Tile-based movement
- [ ] Dialog system
- [ ] Inventory
- [ ] Turn-based battle
- [ ] Save/load (SRAM)

#### Shmup Template:
- [ ] Smooth scrolling
- [ ] Bullet patterns
- [ ] Power-up system
- [ ] Scoring
- [ ] 2-player support

---

## Phase 4: Ecosystem (Months 15+)

### 4.1 VSCode Extension

**Goal**: First-class IDE support

#### Features:
- [ ] Syntax highlighting (65816 ASM, OpenSNES C)
- [ ] Build integration
- [ ] Mesen2 debugger integration
- [ ] Memory viewer
- [ ] VRAM/OAM viewer

### 4.2 Community Templates

**Goal**: Template repository for community contributions

#### Tasks:
- [ ] Template submission guidelines
- [ ] Template registry
- [ ] Quality standards
- [ ] Example categories:
  - [ ] Puzzle games
  - [ ] Sports games
  - [ ] Adventure games

### 4.3 Documentation Site

**Goal**: Professional documentation website

#### Tasks:
- [ ] Docusaurus or similar SSG
- [ ] Tutorial series
- [ ] Video tutorials
- [ ] API reference
- [ ] Hardware deep-dives

---

## Success Metrics

### Phase 1 Complete When:
- Platformer template builds and runs
- 50+ automated tests pass
- Getting Started guide exists

### Phase 2 Complete When:
- QBE compiles all templates
- Code size within 20% of 816-tcc
- All tests pass with QBE

### Phase 3 Complete When:
- 3 game templates complete
- Component system benchmarked
- Asset pipeline documented

### Phase 4 Complete When:
- VSCode extension published
- 10+ community templates
- 1000+ GitHub stars (vanity metric, but indicates adoption)

---

## Technical Debt Rules

1. **No code without tests**
2. **No features without documentation**
3. **Attribution for all external code**
4. **Weekly builds must pass**

## Decision Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2025-01-01 | Fork approach over clean break | Better attribution, MIT license inherited |
| 2025-01-01 | QBE over LLVM | QBE is smaller (~8K lines), easier to modify |
| 2025-01-01 | Parallel development (SDK + compiler) | Avoid burnout, have usable tools while building compiler |
