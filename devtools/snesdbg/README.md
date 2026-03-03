# snesdbg - Symbol-Aware Debugging for Mesen2

A comprehensive Lua debugging library for OpenSNES development with Mesen2 emulator.

## Features

- **Symbol-aware memory access** - Read/write variables by name, not magic addresses
- **Variable watching** - Get callbacks when variables change
- **Breakpoints at symbols** - Set breakpoints by function name
- **Test assertions** - assertEqual, assertGreaterThan, assertInRange, etc.
- **OAM comparison** - Compare shadow buffer to hardware OAM
- **Struct support** - Define and read C struct layouts
- **Test framework** - BDD-style test DSL (describe/it) with event-driven execution

## Quick Start

```lua
-- Load the library
local dbg = require("snesdbg")

-- Load symbols from your ROM's .sym file
dbg.loadSymbols("game.sym")

-- Read variables by name!
local x = dbg.read("monster_x")
local y = dbg.read("monster_y")
print(string.format("Monster at (%d, %d)", x, y))

-- Watch for changes
dbg.watch("monster_x", function(old, new)
    print(string.format("monster_x: %d -> %d", old, new))
end)

-- Set breakpoints at functions
dbg.breakAtSymbol("game_init", function()
    print("game_init() called!")
end)

-- Run code after N frames (non-blocking)
dbg.afterFrames(60, function()
    print(string.format("After 1 second: monster_x = %d", dbg.read("monster_x")))
end)
```

## Installation

1. Copy the `snesdbg` directory to your Mesen2 Lua scripts location
2. In your test scripts, add the snesdbg path to package.path:

```lua
package.path = "/path/to/opensnes/devtools/snesdbg/?.lua;" .. package.path
local dbg = require("snesdbg")
```

## API Reference

### Symbol Management

```lua
dbg.loadSymbols("game.sym")     -- Load symbols from WLA-DX .sym file
dbg.symbolExists("my_var")       -- Check if symbol exists
dbg.getAddress("my_var")         -- Get 24-bit address of symbol
dbg.formatAddr(0x7E0300)         -- Format address with symbol annotation
```

### Memory Access

```lua
-- By symbol name
local val = dbg.read("monster_x")        -- Read 16-bit word
local byte = dbg.readByte("flags")       -- Read byte
local signed = dbg.readSigned("delta")   -- Read signed 16-bit

dbg.write("monster_x", 120)              -- Write 16-bit word
dbg.writeByte("flags", 0x01)             -- Write byte

-- By address (still works)
local val = dbg.read(0x7E0022)

-- Hex dump
dbg.hexdump("oamMemory", 64)             -- Dump with symbol annotations
```

### Variable Watching

```lua
-- Watch a variable for changes
dbg.watch("player_health", function(old, new)
    print(string.format("Health: %d -> %d", old, new))
    if new == 0 then
        print("Game over!")
    end
end)

-- Stop watching
dbg.unwatch("player_health")

-- Clear all watches
dbg.clearWatches()
```

### Breakpoints

```lua
-- Break at a symbol (function or label)
dbg.breakAtSymbol("game_loop", function()
    print("Entered game_loop")
    dbg.printVisibleSprites()
end)

-- Break at address
dbg.breakAt(0x008100, function()
    print("Hit $008100")
end)
```

### Assertions (for tests)

```lua
dbg.assertEqual("player_x", 100, "Player should be at X=100")
dbg.assertNotEqual("health", 0, "Player should not be dead")
dbg.assertGreaterThan("score", 0)
dbg.assertLessThan("timer", 1000)
dbg.assertInRange("x", 0, 255)
dbg.assertTrue(condition, "message")
dbg.assertFalse(condition, "message")

-- Get test results
local results = dbg.getTestResults()
dbg.printTestSummary()  -- Print pass/fail summary
dbg.resetTests()        -- Reset counters
```

### OAM (Sprite) Helpers

```lua
-- Set the shadow buffer symbol
dbg.setOAMBuffer("oamMemory")

-- Read sprite data
local sprite = dbg.readSprite(0)
print(sprite.x, sprite.y, sprite.tile)

-- Read from hardware OAM
local hwSprite = dbg.readHardwareSprite(0)

-- Compare shadow buffer to hardware
local mismatches = dbg.compareOAM()

-- Assert sprite values
dbg.assertOAM(0, {x = 120, y = 100, tile = 0})

-- Assert OAM was DMA'd correctly
dbg.assertOAMTransferred()

-- Debug output
dbg.printSprite(0, "both")    -- Print buffer and hardware
dbg.printVisibleSprites()      -- Print all non-hidden sprites
```

### Struct Support

```lua
-- Define a struct layout
dbg.defineStruct("Player", {
    x      = {offset = 0, size = 2, signed = true},
    y      = {offset = 2, size = 2, signed = true},
    health = {offset = 4, size = 1, signed = false},
    state  = {offset = 5, size = 1, signed = false}
})

-- Read entire struct
local player = dbg.readStruct("player_data", "Player")
print(player.x, player.y, player.health)
```

### Frame Callbacks & Timing (Event-Driven)

Mesen2 is event-driven: you register callbacks, return control, and Mesen2
calls your callbacks at the right time. **Never use blocking loops.**

```lua
-- Run code after N frames
dbg.afterFrames(60, function()
    print("One second has passed!")
end)

-- Run code when execution reaches a symbol
dbg.onSymbolReached("game_init", function(reached)
    if reached then
        print("game_init() was called!")
    else
        print("Timed out waiting for game_init()")
    end
end, 600)  -- 600 frame timeout (10 seconds)

-- Run code every frame
local id = dbg.everyFrame(function()
    -- called each frame
end)
dbg.stopEveryFrame(id)

-- Print debug state
dbg.printState()
```

## Test Framework

The test framework provides a BDD-style DSL for writing ROM tests.
Tests are **declared** synchronously, then **executed** asynchronously
via a state machine driven by Mesen2 callbacks.

Each test function receives a `done` callback — call it when the test completes.

```lua
local test = require("snesdbg.test")

test.describe("My Game Tests", function()

    test.beforeAll(function()
        test.loadSymbols("game.sym")
    end)

    test.it("should initialize player at (100, 100)", function(done)
        test.afterFrames(10, function()
            test.assertEqual("player_x", 100)
            test.assertEqual("player_y", 100)
            done()
        end)
    end)

    test.it("should move right when RIGHT pressed", function(done)
        test.afterFrames(10, function()
            local initial = test.read("player_x")
            test.holdButton("right")
            test.afterFrames(10, function()
                test.assertGreaterThan("player_x", initial)
                test.releaseAllButtons()
                done()
            end)
        end)
    end)

    test.it("should have correct OAM data", function(done)
        test.afterFrames(10, function()
            test.assertOAM(0, {x = 100, y = 100, tile = 0})
            done()
        end)
    end)

end)

test.run()  -- Returns immediately, Mesen2 drives execution
```

### Test Framework API

```lua
-- Suite definition
test.describe("Suite Name", fn)
test.it("test name", fn(done))   -- fn receives done() callback
test.beforeAll(fn)
test.beforeEach(fn)
test.afterEach(fn)
test.afterAll(fn)
test.skip("test name", "reason")
test.only("test name", fn)       -- Run only this test

-- Symbol management
test.loadSymbols("game.sym")

-- Frame control (event-driven)
test.afterFrames(10, callback)              -- Non-blocking frame wait
test.onSymbolReached("label", callback)     -- Non-blocking symbol wait

-- Input simulation
test.holdButton("right")
test.releaseButton("right")
test.releaseAllButtons()
test.pressButton("a", 2, callback)  -- Press for 2 frames, then callback

-- Memory access (same as dbg)
test.read("symbol")
test.write("symbol", value)

-- Assertions (same as dbg)
test.assertEqual("symbol", expected)
test.assertOAM(index, {x=, y=, tile=})
test.assertOAMTransferred()

-- Run tests
test.run()  -- Returns immediately, event-driven execution
```

## File Structure

```
snesdbg/
├── snesdbg.lua        # Main entry point
├── symbols.lua        # WLA-DX .sym parser
├── memory.lua         # Memory access helpers
├── assertions.lua     # Test assertions
├── oam.lua            # OAM helpers
├── test.lua           # Test framework DSL (event-driven)
├── README.md          # This file
└── examples/
    ├── watch_variable.lua   # Watch variables for changes
    ├── trace_function.lua   # Trace function execution
    ├── test_sprite.lua      # Test OAM/sprite functionality
    └── debug_breakout.lua   # Debug breakout example
```

## Mesen2 Memory Types

When using raw Mesen2 API alongside snesdbg, remember:

| Type | Use |
|------|-----|
| `emu.memType.snesSpriteRam` | OAM data |
| `emu.memType.snesWorkRam` | WRAM ($7E-$7F) |
| `emu.memType.snesVideoRam` | VRAM |
| `emu.memType.snesCgRam` | CGRAM (palettes) |
| `emu.memType.snesMemory` | Full address space |

## Common Patterns

### Debugging Variable Corruption (WRAM Mirroring)

```lua
-- Watch variables that might be corrupted
dbg.watch("monster_x", function(old, new)
    if new == 0 then
        print("CORRUPTION! monster_x zeroed")
        -- Print who might have done it
        dbg.hexdump("oamMemory", 32)
    end
end)
```

### Verifying OAM Updates

```lua
-- After game initialization
dbg.afterFrames(10, function()
    -- Check shadow buffer
    local sprite = dbg.readSprite(0)
    print(string.format("Buffer: (%d, %d)", sprite.x, sprite.y))

    -- Check hardware
    local hwSprite = dbg.readHardwareSprite(0)
    print(string.format("Hardware: (%d, %d)", hwSprite.x, hwSprite.y))

    -- They should match
    dbg.assertOAMTransferred()
end)
```

### Tracing Function Execution

```lua
dbg.breakAtSymbol("update_enemies", function()
    print("Updating enemies...")
    print(string.format("  enemy_count = %d", dbg.read("enemy_count")))
end)
```

## Troubleshooting

### "Unknown symbol" errors
- Make sure you called `dbg.loadSymbols()` first
- Check that the symbol exists in your .sym file
- Symbol names are case-sensitive

### OAM comparison shows mismatches
- Make sure you're checking after VBlank (OAM is DMA'd during VBlank)
- Use `dbg.afterFrames(1, callback)` to wait a frame before comparing

### Execution callbacks not firing
- Use 24-bit addresses for exec callbacks: `0x008100` not `0x8100`
- Make sure the code path is actually executed

### "waitFrames() is blocking and not supported" error
- `waitFrames()` and `waitUntilSymbol()` have been removed
- Use `afterFrames(n, callback)` and `onSymbolReached(name, callback)` instead
- See the Quick Start section for examples

## See Also

- [OpenSNES SDK Documentation](../../README.md)
