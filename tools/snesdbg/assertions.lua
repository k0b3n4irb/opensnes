--[[
  assertions.lua - Test Assertions for snesdbg

  Provides assertion functions for writing tests that verify
  memory values, OAM state, and other conditions.

  Usage:
    local assert = require("assertions")
    assert.setMemory(memModule)
    assert.assertEqual("monster_x", 120, "X should be 120")
    assert.assertNotEqual("monster_x", 0, "X should not be 0")
]]

local M = {}

-- Memory module reference
local mem = nil

-- Test results tracking
local results = {
    passed = 0,
    failed = 0,
    errors = {}
}

--- Set the memory module for reading values
function M.setMemory(memModule)
    mem = memModule
end

--- Reset test results
function M.reset()
    results = {
        passed = 0,
        failed = 0,
        errors = {}
    }
end

--- Get test results
function M.getResults()
    return results
end

--- Record a pass
local function pass(msg)
    results.passed = results.passed + 1
    if msg then
        print("[PASS] " .. msg)
    end
end

--- Record a failure
local function fail(msg, expected, actual)
    results.failed = results.failed + 1
    local errMsg = string.format("[FAIL] %s (expected: %s, actual: %s)",
        msg or "Assertion failed",
        tostring(expected),
        tostring(actual))
    print(errMsg)
    table.insert(results.errors, errMsg)
end

--- Read value from symbol or address
local function readValue(nameOrAddr, size)
    size = size or 2  -- default to 16-bit

    if size == 1 then
        return mem.readByte(nameOrAddr)
    elseif size == 2 then
        return mem.readWord(nameOrAddr)
    else
        return mem.readLong(nameOrAddr)
    end
end

--- Assert that a variable equals an expected value
-- @param nameOrAddr Symbol name or address
-- @param expected Expected value
-- @param msg Optional message
-- @param size Optional size (1, 2, or 3 bytes)
function M.assertEqual(nameOrAddr, expected, msg, size)
    local actual = readValue(nameOrAddr, size)
    local label = type(nameOrAddr) == "string" and nameOrAddr or string.format("$%06X", nameOrAddr)
    local fullMsg = msg or (label .. " should equal " .. tostring(expected))

    if actual == expected then
        pass(fullMsg)
        return true
    else
        fail(fullMsg, expected, actual)
        return false
    end
end

--- Assert that a variable does not equal a value
-- @param nameOrAddr Symbol name or address
-- @param unexpected Value that should NOT be present
-- @param msg Optional message
-- @param size Optional size (1, 2, or 3 bytes)
function M.assertNotEqual(nameOrAddr, unexpected, msg, size)
    local actual = readValue(nameOrAddr, size)
    local label = type(nameOrAddr) == "string" and nameOrAddr or string.format("$%06X", nameOrAddr)
    local fullMsg = msg or (label .. " should not equal " .. tostring(unexpected))

    if actual ~= unexpected then
        pass(fullMsg)
        return true
    else
        fail(fullMsg, "not " .. tostring(unexpected), actual)
        return false
    end
end

--- Assert that a variable is greater than a value
-- @param nameOrAddr Symbol name or address
-- @param threshold Value to compare against
-- @param msg Optional message
-- @param size Optional size
function M.assertGreaterThan(nameOrAddr, threshold, msg, size)
    local actual = readValue(nameOrAddr, size)
    local label = type(nameOrAddr) == "string" and nameOrAddr or string.format("$%06X", nameOrAddr)
    local fullMsg = msg or (label .. " should be > " .. tostring(threshold))

    if actual > threshold then
        pass(fullMsg)
        return true
    else
        fail(fullMsg, "> " .. tostring(threshold), actual)
        return false
    end
end

--- Assert that a variable is less than a value
-- @param nameOrAddr Symbol name or address
-- @param threshold Value to compare against
-- @param msg Optional message
-- @param size Optional size
function M.assertLessThan(nameOrAddr, threshold, msg, size)
    local actual = readValue(nameOrAddr, size)
    local label = type(nameOrAddr) == "string" and nameOrAddr or string.format("$%06X", nameOrAddr)
    local fullMsg = msg or (label .. " should be < " .. tostring(threshold))

    if actual < threshold then
        pass(fullMsg)
        return true
    else
        fail(fullMsg, "< " .. tostring(threshold), actual)
        return false
    end
end

--- Assert that a variable is in a range
-- @param nameOrAddr Symbol name or address
-- @param min Minimum value (inclusive)
-- @param max Maximum value (inclusive)
-- @param msg Optional message
-- @param size Optional size
function M.assertInRange(nameOrAddr, min, max, msg, size)
    local actual = readValue(nameOrAddr, size)
    local label = type(nameOrAddr) == "string" and nameOrAddr or string.format("$%06X", nameOrAddr)
    local fullMsg = msg or (label .. " should be in range [" .. min .. ", " .. max .. "]")

    if actual >= min and actual <= max then
        pass(fullMsg)
        return true
    else
        fail(fullMsg, "[" .. min .. ", " .. max .. "]", actual)
        return false
    end
end

--- Assert that a byte is zero
-- @param nameOrAddr Symbol name or address
-- @param msg Optional message
function M.assertZero(nameOrAddr, msg)
    return M.assertEqual(nameOrAddr, 0, msg, 1)
end

--- Assert that a byte is non-zero
-- @param nameOrAddr Symbol name or address
-- @param msg Optional message
function M.assertNonZero(nameOrAddr, msg)
    return M.assertNotEqual(nameOrAddr, 0, msg, 1)
end

--- Assert that a bit is set
-- @param nameOrAddr Symbol name or address
-- @param bit Bit number (0-7)
-- @param msg Optional message
function M.assertBitSet(nameOrAddr, bit, msg)
    local value = mem.readByte(nameOrAddr)
    local mask = bit.lshift(1, bit)
    local label = type(nameOrAddr) == "string" and nameOrAddr or string.format("$%06X", nameOrAddr)
    local fullMsg = msg or (label .. " bit " .. bit .. " should be set")

    if bit.band(value, mask) ~= 0 then
        pass(fullMsg)
        return true
    else
        fail(fullMsg, "bit " .. bit .. " set", string.format("$%02X", value))
        return false
    end
end

--- Assert that a bit is clear
-- @param nameOrAddr Symbol name or address
-- @param bit Bit number (0-7)
-- @param msg Optional message
function M.assertBitClear(nameOrAddr, bit, msg)
    local value = mem.readByte(nameOrAddr)
    local mask = bit.lshift(1, bit)
    local label = type(nameOrAddr) == "string" and nameOrAddr or string.format("$%06X", nameOrAddr)
    local fullMsg = msg or (label .. " bit " .. bit .. " should be clear")

    if bit.band(value, mask) == 0 then
        pass(fullMsg)
        return true
    else
        fail(fullMsg, "bit " .. bit .. " clear", string.format("$%02X", value))
        return false
    end
end

--- Assert a memory region matches expected bytes
-- @param nameOrAddr Symbol name or address
-- @param expected Array of expected bytes
-- @param msg Optional message
function M.assertBytes(nameOrAddr, expected, msg)
    local label = type(nameOrAddr) == "string" and nameOrAddr or string.format("$%06X", nameOrAddr)
    local fullMsg = msg or (label .. " should match expected bytes")

    local actual = mem.readBytes(nameOrAddr, #expected)
    local mismatch = nil

    for i, v in ipairs(expected) do
        if actual[i] ~= v then
            mismatch = {index = i - 1, expected = v, actual = actual[i]}
            break
        end
    end

    if not mismatch then
        pass(fullMsg)
        return true
    else
        local errMsg = string.format("mismatch at offset $%02X", mismatch.index)
        fail(fullMsg .. " - " .. errMsg,
            string.format("$%02X", mismatch.expected),
            string.format("$%02X", mismatch.actual))
        return false
    end
end

--- Assert condition is true
-- @param condition Boolean condition
-- @param msg Message to display
function M.assertTrue(condition, msg)
    if condition then
        pass(msg or "Condition is true")
        return true
    else
        fail(msg or "Condition should be true", "true", "false")
        return false
    end
end

--- Assert condition is false
-- @param condition Boolean condition
-- @param msg Message to display
function M.assertFalse(condition, msg)
    if not condition then
        pass(msg or "Condition is false")
        return true
    else
        fail(msg or "Condition should be false", "false", "true")
        return false
    end
end

--- Print test summary
function M.printSummary()
    print("")
    print("=== Test Summary ===")
    print(string.format("Passed: %d", results.passed))
    print(string.format("Failed: %d", results.failed))
    print(string.format("Total:  %d", results.passed + results.failed))

    if results.failed > 0 then
        print("")
        print("Failures:")
        for _, err in ipairs(results.errors) do
            print("  " .. err)
        end
    end

    return results.failed == 0
end

return M
