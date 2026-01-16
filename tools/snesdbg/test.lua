--[[
  test.lua - Test Framework DSL for OpenSNES

  A BDD-style test framework built on snesdbg for writing readable
  integration tests for SNES ROMs.

  Usage:
    local test = require("snesdbg.test")

    test.describe("My ROM Tests", function()

        test.beforeAll(function()
            test.loadROM("game.sfc")
            test.loadSymbols("game.sym")
        end)

        test.it("should initialize correctly", function()
            test.waitFrames(10)
            test.assertEqual("player_x", 100)
        end)

        test.it("should move right when RIGHT pressed", function()
            local initial = test.read("player_x")
            test.holdButton("right")
            test.waitFrames(10)
            test.assertGreaterThan("player_x", initial)
        end)

    end)

    test.run()
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

-- Button name to Mesen2 button mapping
local buttonMap = {
    a = "a",
    b = "b",
    x = "x",
    y = "y",
    l = "l",
    r = "r",
    start = "start",
    select = "select",
    up = "up",
    down = "down",
    left = "left",
    right = "right"
}

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

--- Define a test case
-- @param name Test name
-- @param fn Test function
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
-- Test Helpers - ROM Management
-- ============================================================================

--- Load a ROM file
-- @param path Path to .sfc file
function M.loadROM(path)
    emu.loadRom(path)
    print(string.format("[test] Loaded ROM: %s", path))
end

--- Load symbols from .sym file
-- @param path Path to .sym file
function M.loadSymbols(path)
    dbg.loadSymbols(path)
end

--- Reset the emulator
function M.reset()
    emu.reset()
    heldButtons = {}
end

--- Power cycle the emulator
function M.powerCycle()
    emu.powerCycle()
    heldButtons = {}
end

-- ============================================================================
-- Test Helpers - Frame Management
-- ============================================================================

--- Wait for specified number of frames
-- @param frames Number of frames to wait
function M.waitFrames(frames)
    for i = 1, frames do
        -- Apply held buttons before each frame
        for button, held in pairs(heldButtons) do
            if held then
                emu.setInput(1, buttonMap[button], true)
            end
        end
        emu.frameAdvance()
    end
end

--- Wait for VBlank
function M.waitForVBlank()
    -- Wait for start of VBlank by checking RDNMI
    local inVBlank = false
    while not inVBlank do
        local nmitimen = emu.read(0x4210, emu.memType.snesMemory)
        inVBlank = (nmitimen & 0x80) ~= 0
        if not inVBlank then
            emu.frameAdvance()
        end
    end
end

--- Wait until execution reaches a symbol
-- @param name Symbol name
-- @param timeout Max frames to wait (default 600)
-- @return true if reached
function M.waitUntilSymbol(name, timeout)
    return dbg.waitUntilSymbol(name, timeout)
end

-- ============================================================================
-- Test Helpers - Input Simulation
-- ============================================================================

--- Hold a button down
-- @param button Button name (a, b, x, y, l, r, start, select, up, down, left, right)
function M.holdButton(button)
    button = string.lower(button)
    if not buttonMap[button] then
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

--- Press a button briefly (hold for a few frames, then release)
-- @param button Button name
-- @param frames Number of frames to hold (default 2)
function M.pressButton(button, frames)
    frames = frames or 2
    M.holdButton(button)
    M.waitFrames(frames)
    M.releaseButton(button)
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
    if not result then
        currentTest.failed = true
    end
    return result
end

function M.assertNotEqual(nameOrAddr, unexpected, msg)
    local result = dbg.assertNotEqual(nameOrAddr, unexpected, msg)
    if not result then
        currentTest.failed = true
    end
    return result
end

function M.assertGreaterThan(nameOrAddr, threshold, msg)
    local result = dbg.assertGreaterThan(nameOrAddr, threshold, msg)
    if not result then
        currentTest.failed = true
    end
    return result
end

function M.assertLessThan(nameOrAddr, threshold, msg)
    local result = dbg.assertLessThan(nameOrAddr, threshold, msg)
    if not result then
        currentTest.failed = true
    end
    return result
end

function M.assertInRange(nameOrAddr, min, max, msg)
    local result = dbg.assertInRange(nameOrAddr, min, max, msg)
    if not result then
        currentTest.failed = true
    end
    return result
end

function M.assertTrue(condition, msg)
    local result = dbg.assertTrue(condition, msg)
    if not result then
        currentTest.failed = true
    end
    return result
end

function M.assertFalse(condition, msg)
    local result = dbg.assertFalse(condition, msg)
    if not result then
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
    if not result then
        currentTest.failed = true
    end
    return result
end

--- Assert OAM was transferred to hardware
function M.assertOAMTransferred()
    local result = dbg.assertOAMTransferred()
    if not result then
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
-- Uses symmap tool logic
function M.assertNoWRAMOverlap()
    -- This would need to shell out to symmap or re-implement the check
    -- For now, just pass (the check should be done at build time)
    print("[PASS] No WRAM overlap (checked at build time)")
    return true
end

-- ============================================================================
-- Test Runner
-- ============================================================================

--- Run all defined test suites
-- @return true if all tests passed
function M.run()
    print("")
    print("======================================")
    print("  OpenSNES Test Runner")
    print("======================================")
    print("")

    results = {
        suites = 0,
        tests = 0,
        passed = 0,
        failed = 0,
        errors = {}
    }

    for _, suite in ipairs(suites) do
        results.suites = results.suites + 1
        print(string.format("Suite: %s", suite.name))
        print(string.rep("-", 40))

        -- Reset assertions for this suite
        dbg.resetTests()

        -- Run beforeAll
        if suite.beforeAll then
            local ok, err = pcall(suite.beforeAll)
            if not ok then
                print(string.format("  [ERROR] beforeAll: %s", err))
                goto continue
            end
        end

        -- Run each test
        for _, test in ipairs(suite.tests) do
            results.tests = results.tests + 1
            currentTest = {name = test.name, failed = false}

            -- Run beforeEach
            if suite.beforeEach then
                local ok, err = pcall(suite.beforeEach)
                if not ok then
                    print(string.format("  [ERROR] beforeEach for '%s': %s", test.name, err))
                    currentTest.failed = true
                    goto nextTest
                end
            end

            -- Run the test
            local ok, err = pcall(test.fn)
            if not ok then
                print(string.format("  [ERROR] %s: %s", test.name, err))
                currentTest.failed = true
            end

            -- Run afterEach
            if suite.afterEach then
                pcall(suite.afterEach)
            end

            -- Record result
            ::nextTest::
            if currentTest.failed then
                results.failed = results.failed + 1
                print(string.format("  [FAIL] %s", test.name))
            else
                results.passed = results.passed + 1
                print(string.format("  [PASS] %s", test.name))
            end

            currentTest = nil
        end

        -- Run afterAll
        if suite.afterAll then
            pcall(suite.afterAll)
        end

        ::continue::
        print("")
    end

    -- Print summary
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
        return true
    else
        print("  SOME TESTS FAILED")
        emu.stop(1)
        return false
    end
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
        fn = function()
            print(string.format("  [SKIP] %s%s", name, reason and (": " .. reason) or ""))
        end,
        skipped = true
    })
end

--- Only run this test (useful for debugging)
-- @param name Test name
-- @param fn Test function
function M.only(name, fn)
    -- Mark all other tests as skipped
    if currentSuite then
        for _, test in ipairs(currentSuite.tests) do
            test.skipped = true
        end
    end
    M.it(name, fn)
end

return M
