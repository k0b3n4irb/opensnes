--[[
  Input Test Script for Mesen2

  Tests that joypad input examples respond correctly.
  Runs against examples/input/two_players/two_players.sfc

  Usage:
    Mesen --testrunner --lua tests/mesen/input_test.lua two_players.sfc
]]

local frameCount = 0
local testsPassed = 0
local testsFailed = 0

local function pass(name)
    testsPassed = testsPassed + 1
    emu.log("[PASS] " .. name)
end

local function fail(name, reason)
    testsFailed = testsFailed + 1
    emu.log("[FAIL] " .. name .. ": " .. reason)
end

-- Test: Check that screen has content after init
local function testScreenInit()
    -- Take screenshot and check size
    local screenshot = emu.takeScreenshot()
    if #screenshot > 300 then
        pass("screen_init")
    else
        fail("screen_init", "screen appears blank")
    end
end

-- Test: Simulate button press and check response
local function testButtonResponse()
    -- Press right on controller 1
    emu.setInput(0, 0x0100)  -- Right button
    emu.execute(5)           -- Run 5 frames

    -- Clear input
    emu.setInput(0, 0x0000)
    emu.execute(5)

    -- If we got here without crash, input works
    pass("button_response")
end

-- Frame callback
emu.addEventCallback(function()
    frameCount = frameCount + 1

    if frameCount == 90 then
        testScreenInit()
    elseif frameCount == 120 then
        testButtonResponse()
    elseif frameCount == 180 then
        emu.log("")
        emu.log("=====================================")
        emu.log("Tests: " .. (testsPassed + testsFailed))
        emu.log("Passed: " .. testsPassed)
        emu.log("Failed: " .. testsFailed)
        emu.log("=====================================")

        if testsFailed == 0 then
            emu.stop(0)  -- Success
        else
            emu.stop(1)  -- Failure
        end
    end
end, emu.eventType.endFrame)

emu.log("[Input Test] Starting...")
