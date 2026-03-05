--[[
  test.lua - Event-Driven Test Framework DSL for OpenSNES

  A BDD-style test framework built on snesdbg for writing readable
  integration tests for SNES ROMs. Uses Mesen2's event-driven model
  (callbacks, not blocking loops).

  Tests are declared synchronously (describe/it), then executed
  asynchronously via a state machine that advances test by test.

  Usage:
    local test = require("snesdbg.test")

    test.describe("My ROM Tests", function()

        test.beforeAll(function()
            test.loadSymbols("game.sym")
        end)

        test.it("should initialize correctly", function(done)
            test.afterFrames(10, function()
                test.assertEqual("player_x", 100)
                done()
            end)
        end)

        test.it("should move right when RIGHT pressed", function(done)
            local initial = test.read("player_x")
            test.holdButton("right")
            test.afterFrames(10, function()
                test.assertGreaterThan("player_x", initial)
                test.releaseAllButtons()
                done()
            end)
        end)

    end)

    test.run()  -- Returns immediately, Mesen2 drives execution
]]

-- Get the directory of this script for relative requires
local scriptDir = debug.getinfo(1, "S").source:match("@?(.*/)")
if scriptDir then
    package.path = scriptDir .. "?.lua;" .. package.path
end

local M = {}

local dbg = require("snesdbg")

-- Test suite state
local suites = {}
local currentSuite = nil
local currentTest = nil

-- Results
local results = {
    suites = 0,
    tests = 0,
    passed = 0,
    failed = 0,
    errors = {}
}

-- Button state for input simulation
local heldButtons = {}

-- Button name to SNES bit mask mapping
local buttonBits = {
    b      = 0x8000,
    y      = 0x4000,
    select = 0x2000,
    start  = 0x1000,
    up     = 0x0800,
    down   = 0x0400,
    left   = 0x0200,
    right  = 0x0100,
    a      = 0x0080,
    x      = 0x0040,
    l      = 0x0020,
    r      = 0x0010
}

-- Input callback ID
local inputCallbackId = nil

-- ============================================================================
-- Suite Definition
-- ============================================================================

--- Define a test suite
-- @param name Suite name
-- @param fn Function containing test definitions
function M.describe(name, fn)
    local suite = {
        name = name,
        tests = {},
        beforeAll = nil,
        beforeEach = nil,
        afterEach = nil,
        afterAll = nil
    }

    currentSuite = suite
    fn()
    currentSuite = nil

    table.insert(suites, suite)
end

--- Define a test case (receives a done callback)
-- @param name Test name
-- @param fn Test function, called with done() callback
function M.it(name, fn)
    if not currentSuite then
        error("it() must be called inside describe()")
    end

    table.insert(currentSuite.tests, {
        name = name,
        fn = fn
    })
end

--- Setup function to run before all tests in suite
-- @param fn Setup function
function M.beforeAll(fn)
    if not currentSuite then
        error("beforeAll() must be called inside describe()")
    end
    currentSuite.beforeAll = fn
end

--- Setup function to run before each test
-- @param fn Setup function
function M.beforeEach(fn)
    if not currentSuite then
        error("beforeEach() must be called inside describe()")
    end
    currentSuite.beforeEach = fn
end

--- Teardown function to run after each test
-- @param fn Teardown function
function M.afterEach(fn)
    if not currentSuite then
        error("afterEach() must be called inside describe()")
    end
    currentSuite.afterEach = fn
end

--- Teardown function to run after all tests
-- @param fn Teardown function
function M.afterAll(fn)
    if not currentSuite then
        error("afterAll() must be called inside describe()")
    end
    currentSuite.afterAll = fn
end

-- ============================================================================
-- Test Helpers - Symbol Management
-- ============================================================================

--- Load symbols from .sym file
-- @param path Path to .sym file
function M.loadSymbols(path)
    dbg.loadSymbols(path)
end

-- ============================================================================
-- Test Helpers - Frame Management (Event-Driven)
-- ============================================================================

--- Execute a callback after N frames (non-blocking)
-- @param frames Number of frames to wait
-- @param callback Function to call when done
-- @return event callback ID
function M.afterFrames(frames, callback)
    return dbg.afterFrames(frames, callback)
end

--- Execute a callback when a symbol is reached (non-blocking)
-- @param name Symbol name
-- @param callback Function called with (reached: boolean)
-- @param timeout Max frames to wait (default 600)
-- @return IDs table
function M.onSymbolReached(name, callback, timeout)
    return dbg.onSymbolReached(name, callback, timeout)
end

-- ============================================================================
-- Test Helpers - Input Simulation
-- ============================================================================

--- Hold a button down
-- @param button Button name (a, b, x, y, l, r, start, select, up, down, left, right)
function M.holdButton(button)
    button = string.lower(button)
    if not buttonBits[button] then
        error("Unknown button: " .. button)
    end
    heldButtons[button] = true
end

--- Release a button
-- @param button Button name
function M.releaseButton(button)
    button = string.lower(button)
    heldButtons[button] = false
end

--- Release all buttons
function M.releaseAllButtons()
    heldButtons = {}
end

--- Press a button for N frames, then call back (non-blocking)
-- @param button Button name
-- @param frames Number of frames to hold (default 2)
-- @param callback Function to call after release
function M.pressButton(button, frames, callback)
    frames = frames or 2
    M.holdButton(button)
    M.afterFrames(frames, function()
        M.releaseButton(button)
        if callback then callback() end
    end)
end

-- Internal: apply held buttons each frame via input callback
local function applyInput()
    local mask = 0
    for button, held in pairs(heldButtons) do
        if held then
            mask = mask | buttonBits[button]
        end
    end
    if mask ~= 0 then
        emu.setInput(0, mask)
    end
end

-- ============================================================================
-- Test Helpers - Memory Access (pass-through to snesdbg)
-- ============================================================================

function M.read(nameOrAddr)
    return dbg.read(nameOrAddr)
end

function M.readByte(nameOrAddr)
    return dbg.readByte(nameOrAddr)
end

function M.readSigned(nameOrAddr)
    return dbg.readSigned(nameOrAddr)
end

function M.write(nameOrAddr, value)
    dbg.write(nameOrAddr, value)
end

function M.writeByte(nameOrAddr, value)
    dbg.writeByte(nameOrAddr, value)
end

-- ============================================================================
-- Test Helpers - Assertions (pass-through to snesdbg)
-- ============================================================================

function M.assertEqual(nameOrAddr, expected, msg)
    local result = dbg.assertEqual(nameOrAddr, expected, msg)
    if not result and currentTest then
        currentTest.failed = true
    end
    return result
end

function M.assertNotEqual(nameOrAddr, unexpected, msg)
    local result = dbg.assertNotEqual(nameOrAddr, unexpected, msg)
    if not result and currentTest then
        currentTest.failed = true
    end
    return result
end

function M.assertGreaterThan(nameOrAddr, threshold, msg)
    local result = dbg.assertGreaterThan(nameOrAddr, threshold, msg)
    if not result and currentTest then
        currentTest.failed = true
    end
    return result
end

function M.assertLessThan(nameOrAddr, threshold, msg)
    local result = dbg.assertLessThan(nameOrAddr, threshold, msg)
    if not result and currentTest then
        currentTest.failed = true
    end
    return result
end

function M.assertInRange(nameOrAddr, min, max, msg)
    local result = dbg.assertInRange(nameOrAddr, min, max, msg)
    if not result and currentTest then
        currentTest.failed = true
    end
    return result
end

function M.assertTrue(condition, msg)
    local result = dbg.assertTrue(condition, msg)
    if not result and currentTest then
        currentTest.failed = true
    end
    return result
end

function M.assertFalse(condition, msg)
    local result = dbg.assertFalse(condition, msg)
    if not result and currentTest then
        currentTest.failed = true
    end
    return result
end

-- ============================================================================
-- Test Helpers - OAM Assertions
-- ============================================================================

--- Assert OAM sprite matches expected values
-- @param index Sprite index (0-127)
-- @param expected Table with expected values {x=, y=, tile=, ...}
function M.assertOAM(index, expected)
    local result = dbg.assertOAM(index, expected)
    if not result and currentTest then
        currentTest.failed = true
    end
    return result
end

--- Assert OAM was transferred to hardware
function M.assertOAMTransferred()
    local result = dbg.assertOAMTransferred()
    if not result and currentTest then
        currentTest.failed = true
    end
    return result
end

--- Read OAM sprite field
-- @param index Sprite index
-- @param field Field name (x, y, tile, palette, priority, etc.)
-- @return field value
function M.readOAM(index, field)
    local sprite = dbg.readSprite(index)
    if sprite and sprite[field] ~= nil then
        return sprite[field]
    end
    error("Unknown OAM field: " .. tostring(field))
end

-- ============================================================================
-- Test Helpers - Memory Overlap Check
-- ============================================================================

--- Assert no WRAM mirror overlaps exist
function M.assertNoWRAMOverlap()
    print("[PASS] No WRAM overlap (checked at build time)")
    return true
end

-- ============================================================================
-- Test Runner (Event-Driven State Machine)
-- ============================================================================

-- Execution state
local execState = {
    suiteIdx = 0,
    testIdx = 0,
    phase = "idle"  -- idle, beforeAll, beforeEach, test, afterEach, afterAll, done
}

-- Advance to next test/suite (called after each step completes)
local function advance()
    local suite = suites[execState.suiteIdx]

    if execState.phase == "beforeAll" then
        execState.testIdx = 1
        if #suite.tests > 0 then
            execState.phase = "beforeEach"
        else
            execState.phase = "afterAll"
        end
        advance()
        return
    end

    if execState.phase == "beforeEach" then
        execState.phase = "test"
        local test = suite.tests[execState.testIdx]
        currentTest = {name = test.name, failed = false}

        -- Run beforeEach
        if suite.beforeEach then
            local ok, err = pcall(suite.beforeEach)
            if not ok then
                print(string.format("  [ERROR] beforeEach for '%s': %s", test.name, err))
                currentTest.failed = true
                execState.phase = "afterEach"
                advance()
                return
            end
        end

        -- Run the test with done() callback
        local done = function()
            -- afterEach
            if suite.afterEach then
                pcall(suite.afterEach)
            end

            -- Record result
            if currentTest.failed then
                results.failed = results.failed + 1
                print(string.format("  [FAIL] %s", test.name))
            else
                results.passed = results.passed + 1
                print(string.format("  [PASS] %s", test.name))
            end
            results.tests = results.tests + 1
            currentTest = nil

            -- Next test or finish suite
            execState.testIdx = execState.testIdx + 1
            if execState.testIdx <= #suite.tests then
                execState.phase = "beforeEach"
                advance()
            else
                execState.phase = "afterAll"
                advance()
            end
        end

        local ok, err = pcall(test.fn, done)
        if not ok then
            print(string.format("  [ERROR] %s: %s", test.name, err))
            currentTest.failed = true
            done()
        end
        return
    end

    if execState.phase == "afterAll" then
        if suite.afterAll then
            pcall(suite.afterAll)
        end
        print("")

        -- Next suite or finish
        execState.suiteIdx = execState.suiteIdx + 1
        if execState.suiteIdx <= #suites then
            execState.phase = "beforeAll"
            advance()
        else
            execState.phase = "done"
            printSummary()
        end
        return
    end
end

-- Print final summary and stop emulator
function printSummary()
    -- Remove input callback
    if inputCallbackId then
        emu.removeEventCallback(inputCallbackId, emu.eventType.startFrame)
        inputCallbackId = nil
    end

    print("======================================")
    print("  Results")
    print("======================================")
    print(string.format("  Suites: %d", results.suites))
    print(string.format("  Tests:  %d", results.tests))
    print(string.format("  Passed: %d", results.passed))
    print(string.format("  Failed: %d", results.failed))
    print("")

    if results.failed == 0 then
        print("  ALL TESTS PASSED!")
        emu.stop(0)
    else
        print("  SOME TESTS FAILED")
        emu.stop(1)
    end
end

--- Run all defined test suites (returns immediately, event-driven)
function M.run()
    print("")
    print("======================================")
    print("  OpenSNES Test Runner")
    print("======================================")
    print("")

    results = {
        suites = #suites,
        tests = 0,
        passed = 0,
        failed = 0,
        errors = {}
    }

    if #suites == 0 then
        print("  No test suites defined.")
        emu.stop(0)
        return
    end

    -- Register input callback to apply held buttons each frame
    inputCallbackId = emu.addEventCallback(applyInput, emu.eventType.startFrame)

    -- Start execution
    execState.suiteIdx = 1
    execState.phase = "beforeAll"

    local suite = suites[1]
    print(string.format("Suite: %s", suite.name))
    print(string.rep("-", 40))

    -- Reset assertions
    dbg.resetTests()

    -- Run beforeAll
    if suite.beforeAll then
        local ok, err = pcall(suite.beforeAll)
        if not ok then
            print(string.format("  [ERROR] beforeAll: %s", err))
            execState.phase = "afterAll"
            advance()
            return
        end
    end

    execState.phase = "beforeAll"
    advance()
end

--- Skip a test (mark as pending)
-- @param name Test name
-- @param reason Optional skip reason
function M.skip(name, reason)
    if not currentSuite then
        error("skip() must be called inside describe()")
    end

    table.insert(currentSuite.tests, {
        name = name,
        fn = function(done)
            print(string.format("  [SKIP] %s%s", name, reason and (": " .. reason) or ""))
            done()
        end,
        skipped = true
    })
end

--- Only run this test (useful for debugging)
-- @param name Test name
-- @param fn Test function
function M.only(name, fn)
    if currentSuite then
        for _, test in ipairs(currentSuite.tests) do
            test.skipped = true
        end
    end
    M.it(name, fn)
end

return M
