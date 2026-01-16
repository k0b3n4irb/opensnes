--[[
  snesdbg.lua - Symbol-Aware Debugging Library for Mesen2

  A comprehensive debugging library for OpenSNES development that provides:
  - Symbol-aware memory access (read/write by variable name)
  - Variable watching with callbacks
  - Breakpoints at symbol addresses
  - Test assertions
  - OAM comparison (shadow buffer vs hardware)
  - Execution tracing with symbol annotations

  Usage:
    local dbg = require("snesdbg")

    -- Load symbols from .sym file
    dbg.loadSymbols("game.sym")

    -- Read/write by symbol name
    local x = dbg.read("monster_x")
    dbg.write("monster_x", 120)

    -- Watch variable changes
    dbg.watch("monster_x", function(old, new)
        print("monster_x: " .. old .. " -> " .. new)
    end)

    -- Assertions
    dbg.assertEqual("monster_x", 120, "X should be 120")

  Requires: Mesen2 emulator with Lua scripting support

  Part of OpenSNES SDK - https://github.com/xxx/opensnes
]]

-- Get the directory of this script for relative requires
local scriptDir = debug.getinfo(1, "S").source:match("@?(.*/)")
if scriptDir then
    package.path = scriptDir .. "?.lua;" .. package.path
end

local M = {}

-- Load submodules
local symbols = require("symbols")
local memory = require("memory")
local assertions = require("assertions")
local oam = require("oam")

-- Current symbol table
local currentSymbols = nil

-- Watch callbacks: {symbolName -> {lastValue, callback}}
local watches = {}

-- Struct definitions
local structDefs = {}

-- ============================================================================
-- Symbol Management
-- ============================================================================

--- Load symbols from a WLA-DX .sym file
-- @param path Path to the .sym file
function M.loadSymbols(path)
    currentSymbols = symbols.load(path)
    memory.setSymbols(currentSymbols)
    oam.init(memory, currentSymbols)
    print(string.format("[snesdbg] Loaded %d symbols from %s", currentSymbols:count(), path))
end

--- Check if a symbol exists
-- @param name Symbol name
-- @return true if symbol exists
function M.symbolExists(name)
    return currentSymbols and currentSymbols:exists(name)
end

--- Get address of a symbol
-- @param name Symbol name
-- @return full 24-bit address, or nil
function M.getAddress(name)
    if not currentSymbols then
        error("Symbols not loaded. Call loadSymbols() first.")
    end
    return currentSymbols:resolve(name)
end

--- Format an address with symbol annotation
-- @param addr Full 24-bit address
-- @return formatted string like "$008100 [update_monster+$0F]"
function M.formatAddr(addr)
    if currentSymbols then
        return currentSymbols:formatAddr(addr)
    else
        return string.format("$%06X", addr)
    end
end

-- ============================================================================
-- Memory Access
-- ============================================================================

--- Read a byte from a symbol or address
-- @param nameOrAddr Symbol name or 24-bit address
-- @return byte value
function M.readByte(nameOrAddr)
    return memory.readByte(nameOrAddr)
end

--- Read a 16-bit word from a symbol or address
-- @param nameOrAddr Symbol name or 24-bit address
-- @return 16-bit value
function M.read(nameOrAddr)
    return memory.readWord(nameOrAddr)
end

--- Read a signed 16-bit word
-- @param nameOrAddr Symbol name or 24-bit address
-- @return signed 16-bit value
function M.readSigned(nameOrAddr)
    return memory.readWordSigned(nameOrAddr)
end

--- Read multiple bytes
-- @param nameOrAddr Symbol name or 24-bit address
-- @param count Number of bytes
-- @return array of bytes
function M.readBytes(nameOrAddr, count)
    return memory.readBytes(nameOrAddr, count)
end

--- Write a byte to a symbol or address
-- @param nameOrAddr Symbol name or 24-bit address
-- @param value Byte value
function M.writeByte(nameOrAddr, value)
    memory.writeByte(nameOrAddr, value)
end

--- Write a 16-bit word to a symbol or address
-- @param nameOrAddr Symbol name or 24-bit address
-- @param value 16-bit value
function M.write(nameOrAddr, value)
    memory.writeWord(nameOrAddr, value)
end

--- Hex dump memory with symbol annotations
-- @param nameOrAddr Symbol name or 24-bit address
-- @param count Number of bytes (default 64)
function M.hexdump(nameOrAddr, count)
    count = count or 64
    print(memory.hexdump(nameOrAddr, count))
end

-- ============================================================================
-- Variable Watching
-- ============================================================================

--- Watch a variable for changes
-- @param name Symbol name
-- @param callback Function called with (oldValue, newValue)
function M.watch(name, callback)
    if not currentSymbols then
        error("Symbols not loaded. Call loadSymbols() first.")
    end

    local addr = currentSymbols:resolve(name)
    if not addr then
        error("Unknown symbol: " .. name)
    end

    watches[name] = {
        addr = addr,
        lastValue = memory.readWord(addr),
        callback = callback
    }

    print(string.format("[snesdbg] Watching '%s' at $%06X", name, addr))
end

--- Stop watching a variable
-- @param name Symbol name
function M.unwatch(name)
    watches[name] = nil
end

--- Clear all watches
function M.clearWatches()
    watches = {}
end

--- Internal: Check all watches (call every frame)
local function checkWatches()
    for name, watch in pairs(watches) do
        local current = memory.readWord(watch.addr)
        if current ~= watch.lastValue then
            local old = watch.lastValue
            watch.lastValue = current
            if watch.callback then
                watch.callback(old, current)
            end
        end
    end
end

-- ============================================================================
-- Breakpoints
-- ============================================================================

--- Set a breakpoint at a symbol
-- @param name Symbol name
-- @param callback Function called when breakpoint hit
function M.breakAtSymbol(name, callback)
    if not currentSymbols then
        error("Symbols not loaded. Call loadSymbols() first.")
    end

    local addr = currentSymbols:resolve(name)
    if not addr then
        error("Unknown symbol: " .. name)
    end

    -- Mesen2 requires 24-bit addresses for execution callbacks
    emu.addMemoryCallback(callback, emu.callbackType.exec, addr)
    print(string.format("[snesdbg] Breakpoint set at '%s' ($%06X)", name, addr))
end

--- Set a breakpoint at an address
-- @param addr 24-bit address
-- @param callback Function called when breakpoint hit
function M.breakAt(addr, callback)
    emu.addMemoryCallback(callback, emu.callbackType.exec, addr)
    print(string.format("[snesdbg] Breakpoint set at %s", M.formatAddr(addr)))
end

-- ============================================================================
-- Assertions
-- ============================================================================

-- Pass through assertion functions with memory module pre-configured
assertions.setMemory(memory)

--- Assert variable equals expected value
-- @param nameOrAddr Symbol name or address
-- @param expected Expected value
-- @param msg Optional message
function M.assertEqual(nameOrAddr, expected, msg)
    return assertions.assertEqual(nameOrAddr, expected, msg)
end

--- Assert variable does not equal value
-- @param nameOrAddr Symbol name or address
-- @param unexpected Value that should not match
-- @param msg Optional message
function M.assertNotEqual(nameOrAddr, unexpected, msg)
    return assertions.assertNotEqual(nameOrAddr, unexpected, msg)
end

--- Assert variable is greater than threshold
-- @param nameOrAddr Symbol name or address
-- @param threshold Comparison value
-- @param msg Optional message
function M.assertGreaterThan(nameOrAddr, threshold, msg)
    return assertions.assertGreaterThan(nameOrAddr, threshold, msg)
end

--- Assert variable is less than threshold
-- @param nameOrAddr Symbol name or address
-- @param threshold Comparison value
-- @param msg Optional message
function M.assertLessThan(nameOrAddr, threshold, msg)
    return assertions.assertLessThan(nameOrAddr, threshold, msg)
end

--- Assert variable is in range
-- @param nameOrAddr Symbol name or address
-- @param min Minimum (inclusive)
-- @param max Maximum (inclusive)
-- @param msg Optional message
function M.assertInRange(nameOrAddr, min, max, msg)
    return assertions.assertInRange(nameOrAddr, min, max, msg)
end

--- Assert condition is true
-- @param condition Boolean
-- @param msg Message
function M.assertTrue(condition, msg)
    return assertions.assertTrue(condition, msg)
end

--- Assert condition is false
-- @param condition Boolean
-- @param msg Message
function M.assertFalse(condition, msg)
    return assertions.assertFalse(condition, msg)
end

--- Get assertion test results
function M.getTestResults()
    return assertions.getResults()
end

--- Reset assertion counters
function M.resetTests()
    assertions.reset()
end

--- Print test summary
function M.printTestSummary()
    return assertions.printSummary()
end

-- ============================================================================
-- OAM Helpers
-- ============================================================================

--- Set OAM shadow buffer symbol
-- @param symbolName Name of the OAM buffer (e.g., "oamMemory")
function M.setOAMBuffer(symbolName)
    oam.setBufferSymbol(symbolName)
end

--- Read sprite from shadow buffer
-- @param index Sprite index (0-127)
-- @return sprite data table
function M.readSprite(index)
    return oam.readSprite(index)
end

--- Read sprite from hardware OAM
-- @param index Sprite index (0-127)
-- @return sprite data table
function M.readHardwareSprite(index)
    return oam.readHardwareSprite(index)
end

--- Compare shadow buffer to hardware OAM
-- @param maxSprites Maximum sprites to check (default 128)
-- @return number of mismatches
function M.compareOAM(maxSprites)
    return oam.compareToHardware(maxSprites)
end

--- Assert sprite matches expected values
-- @param index Sprite index
-- @param expected Table {x=, y=, tile=, ...}
-- @param msg Optional message
function M.assertOAM(index, expected, msg)
    return oam.assertSprite(index, expected, msg)
end

--- Assert OAM was transferred to hardware
-- @param msg Optional message
function M.assertOAMTransferred(msg)
    return oam.assertTransferred(msg)
end

--- Print sprite information
-- @param index Sprite index
-- @param source "buffer", "hardware", or "both"
function M.printSprite(index, source)
    oam.printSprite(index, source)
end

--- Print all visible sprites
function M.printVisibleSprites()
    oam.printVisibleSprites()
end

-- ============================================================================
-- Struct Support
-- ============================================================================

--- Define a struct layout
-- @param name Struct name
-- @param fields Table of field definitions
function M.defineStruct(name, fields)
    structDefs[name] = fields
end

--- Read a struct by symbol and struct type
-- @param nameOrAddr Symbol name or address of struct instance
-- @param structName Name of struct definition
-- @return table with field values
function M.readStruct(nameOrAddr, structName)
    local def = structDefs[structName]
    if not def then
        error("Unknown struct: " .. structName)
    end
    return memory.readStruct(nameOrAddr, def)
end

-- ============================================================================
-- Frame Callbacks
-- ============================================================================

local frameCallbacks = {}

--- Run function every frame
-- @param callback Function to call
-- @return callback ID
function M.everyFrame(callback)
    local id = #frameCallbacks + 1
    frameCallbacks[id] = callback
    return id
end

--- Stop a frame callback
-- @param id Callback ID
function M.stopEveryFrame(id)
    frameCallbacks[id] = nil
end

--- Internal frame handler
local function onFrame()
    checkWatches()
    for _, callback in pairs(frameCallbacks) do
        callback()
    end
end

-- ============================================================================
-- Utilities
-- ============================================================================

--- Wait for a number of frames
-- @param frames Number of frames to wait
function M.waitFrames(frames)
    local count = 0
    local waiting = true

    local function onFrameWait()
        count = count + 1
        if count >= frames then
            waiting = false
        end
    end

    emu.addEventCallback(onFrameWait, emu.eventType.startFrame)

    while waiting do
        emu.frameAdvance()
    end

    emu.removeEventCallback(onFrameWait, emu.eventType.startFrame)
end

--- Wait until a symbol address is reached (execution)
-- @param name Symbol name
-- @param timeout Maximum frames to wait (default 600 = 10 seconds)
-- @return true if reached, false if timeout
function M.waitUntilSymbol(name, timeout)
    timeout = timeout or 600
    local reached = false

    local function onHit()
        reached = true
    end

    M.breakAtSymbol(name, onHit)

    local frames = 0
    while not reached and frames < timeout do
        emu.frameAdvance()
        frames = frames + 1
    end

    if not reached then
        print(string.format("[snesdbg] TIMEOUT waiting for '%s' after %d frames", name, timeout))
    end

    return reached
end

--- Print current state (useful for debugging)
function M.printState()
    print("=== snesdbg State ===")
    print(string.format("Symbols loaded: %s", currentSymbols and "yes" or "no"))
    if currentSymbols then
        print(string.format("  Count: %d", currentSymbols:count()))
    end
    print(string.format("Watches: %d", 0))
    for name, _ in pairs(watches) do
        print(string.format("  - %s", name))
    end
    print(string.format("Struct definitions: %d", 0))
    for name, _ in pairs(structDefs) do
        print(string.format("  - %s", name))
    end
end

-- ============================================================================
-- Initialization
-- ============================================================================

--- Initialize the debug library (call once at start)
function M.init()
    -- Register frame callback
    emu.addEventCallback(onFrame, emu.eventType.startFrame)
    print("[snesdbg] Initialized")
end

--- Stop the debug library
function M.shutdown()
    emu.removeEventCallback(onFrame, emu.eventType.startFrame)
    M.clearWatches()
    frameCallbacks = {}
    print("[snesdbg] Shutdown")
end

-- Auto-initialize if emu is available
if emu then
    M.init()
end

return M
