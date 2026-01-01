--[[
  OpenSNES Test Harness - Mesen2 Lua API

  This script is loaded by Mesen2 to run automated tests.
  It monitors test result memory and reports pass/fail status.

  Usage:
    Mesen rom.sfc --testrunner --LoadScript=test_harness.lua

  Author: OpenSNES Team
  License: MIT
]]

local test = {}

--------------------------------------------------------------------------------
-- Constants
--------------------------------------------------------------------------------

-- Memory addresses (must match test_harness.h)
local STATUS_ADDR     = 0x7F0000
local RUN_COUNT_ADDR  = 0x7F0001
local PASS_COUNT_ADDR = 0x7F0003
local FAIL_COUNT_ADDR = 0x7F0005
local MESSAGE_ADDR    = 0x7F0010

-- Status values
local STATUS_RUNNING = 0
local STATUS_PASS    = 1
local STATUS_FAIL    = 2

-- Default timeout (milliseconds)
local DEFAULT_TIMEOUT = 10000

--------------------------------------------------------------------------------
-- Memory Access
--------------------------------------------------------------------------------

function test.read_byte(addr)
    return emu.read(addr, emu.memType.snesMemory)
end

function test.read_word(addr)
    local lo = emu.read(addr, emu.memType.snesMemory)
    local hi = emu.read(addr + 1, emu.memType.snesMemory)
    return lo + (hi * 256)
end

function test.read_string(addr, max_len)
    local str = ""
    for i = 0, max_len - 1 do
        local byte = emu.read(addr + i, emu.memType.snesMemory)
        if byte == 0 then break end
        str = str .. string.char(byte)
    end
    return str
end

--------------------------------------------------------------------------------
-- Test State
--------------------------------------------------------------------------------

function test.get_status()
    return test.read_byte(STATUS_ADDR)
end

function test.get_run_count()
    return test.read_word(RUN_COUNT_ADDR)
end

function test.get_pass_count()
    return test.read_word(PASS_COUNT_ADDR)
end

function test.get_fail_count()
    return test.read_word(FAIL_COUNT_ADDR)
end

function test.get_message()
    return test.read_string(MESSAGE_ADDR, 64)
end

--------------------------------------------------------------------------------
-- Test Execution
--------------------------------------------------------------------------------

function test.init()
    emu.log("[OpenSNES Test] Initializing test runner")
end

function test.run_until_complete(timeout_ms)
    timeout_ms = timeout_ms or DEFAULT_TIMEOUT

    local start_time = os.clock() * 1000
    local check_interval = 100  -- Check every 100 frames

    emu.log("[OpenSNES Test] Waiting for test completion (timeout: " .. timeout_ms .. "ms)")

    while true do
        -- Run emulator for some frames
        for i = 1, check_interval do
            emu.frameAdvance()
        end

        -- Check status
        local status = test.get_status()
        if status ~= STATUS_RUNNING then
            emu.log("[OpenSNES Test] Tests completed with status: " .. status)
            return status
        end

        -- Check timeout
        local elapsed = (os.clock() * 1000) - start_time
        if elapsed > timeout_ms then
            emu.log("[OpenSNES Test] TIMEOUT after " .. elapsed .. "ms")
            return -1  -- Timeout
        end
    end
end

function test.check_results()
    local status = test.get_status()
    local run = test.get_run_count()
    local pass = test.get_pass_count()
    local fail = test.get_fail_count()
    local message = test.get_message()

    emu.log("========================================")
    emu.log("[OpenSNES Test] Results:")
    emu.log("  Tests run:    " .. run)
    emu.log("  Tests passed: " .. pass)
    emu.log("  Tests failed: " .. fail)

    if status == STATUS_PASS then
        emu.log("  Status: PASS")
    elseif status == STATUS_FAIL then
        emu.log("  Status: FAIL")
        if message ~= "" then
            emu.log("  Message: " .. message)
        end
    elseif status == STATUS_RUNNING then
        emu.log("  Status: STILL RUNNING (possible hang)")
    else
        emu.log("  Status: UNKNOWN (" .. status .. ")")
    end
    emu.log("========================================")

    return status == STATUS_PASS
end

function test.exit()
    local status = test.get_status()
    local success = (status == STATUS_PASS)

    if success then
        emu.log("[OpenSNES Test] Exiting with SUCCESS")
    else
        emu.log("[OpenSNES Test] Exiting with FAILURE")
    end

    -- Exit emulator with appropriate code
    emu.stop(success and 0 or 1)
end

--------------------------------------------------------------------------------
-- Main Entry Point (when run as script)
--------------------------------------------------------------------------------

function test.main()
    test.init()
    test.run_until_complete(DEFAULT_TIMEOUT)
    local success = test.check_results()
    test.exit()
end

return test
